#include "tinylates/tinylates.hpp"

#include <sstream>

CTinylatesProp::CTinylatesProp(const std::string& data) : m_type(TL_PROP_STRING), m_data(data) {
    ;
}

CTinylatesProp::CTinylatesProp(const int& data) : m_type(TL_PROP_INT), m_data(data) {
    ;
}

CTinylatesProp::CTinylatesProp(const std::vector<std::string>& data) : m_type(TL_PROP_STR_VECTOR), m_data(data) {
    ;
}

bool CTinylatesProp::truthy() const {
    switch (m_type) {
        case TL_PROP_INT: return std::any_cast<int>(m_data) != 0;
        case TL_PROP_STRING: return !std::any_cast<std::string>(m_data).empty();
        case TL_PROP_STR_VECTOR: return !std::any_cast<std::vector<std::string>>(m_data).empty();
        default: break;
    }
    return false;
}

CTinylates::CTinylates(const std::string& html) : m_html(html) {
    ;
}

void CTinylates::add(const std::string& name, const CTinylatesProp& prop) {
    if (name == "i" || name == "n")
        return;

    if (m_props.contains(name))
        m_props.at(name) = prop;
    else
        m_props.emplace(name, prop);
}

static std::string_view trim(const std::string_view& sv) {
    size_t begin = 0, end = 0;
    for (size_t i = 0; i < sv.length(); ++i) {
        if (!std::isspace(sv[i]))
            break;
        begin++;
    }

    for (size_t i = sv.length() - 1; i > 0; --i) {
        if (!std::isspace(sv[i]))
            break;
        end++;
    }

    return sv.substr(begin, sv.length() - begin - end);
}

std::string CTinylates::parseTagText(const std::string_view& sv) {
    size_t spacePos = sv.find(' ');
    if (spacePos == std::string::npos)
        return "";

    std::string_view name = trim(sv.substr(spacePos));

    if (name.contains('[') && name.contains(']')) {
        size_t openPos  = name.find('[');
        size_t closePos = name.find(']');

        if (closePos < openPos)
            return "";

        std::string_view justName = trim(name.substr(0, openPos));
        std::string_view idx      = name.substr(openPos + 1, closePos - openPos - 1);

        if (!m_props.contains(std::string{justName}))
            return "";

        const auto& PROP = m_props.at(std::string{justName});
        if (PROP.m_type != TL_PROP_STR_VECTOR)
            return "";

        const auto VEC = std::any_cast<std::vector<std::string>>(PROP.m_data);

        if (!m_props.contains(std::string{idx})) {
            // try number
            try {
                const size_t IDX = std::stoull(std::string{idx});
                if (IDX >= VEC.size())
                    return "";

                return VEC.at(IDX);
            } catch (...) { return ""; }
        }

        const auto& PROPIDX = m_props.at(std::string{idx});
        if (PROPIDX.m_type != TL_PROP_INT)
            return "";

        const auto IDX = std::any_cast<int>(PROPIDX.m_data);

        if (IDX < 0 || IDX >= (int)VEC.size())
            return "";

        return VEC.at(IDX);
    }

    if (m_props.contains(std::string{name})) {
        auto& prop = m_props.at(std::string{name});
        if (prop.m_type != TL_PROP_STRING)
            return "";
        return std::any_cast<std::string>(prop.m_data);
    }
    return "";
}

std::string CTinylates::parseTagData(const std::string_view& sv) {
    if (sv.starts_with("tl:text"))
        return parseTagText(sv);
    return "";
}

std::string CTinylates::tagNeedsNext(const std::string_view& sv) {
    if (sv.starts_with("tl:if"))
        return "tl:endif";
    if (sv.starts_with("tl:for"))
        return "tl:endfor";
    return "";
}

bool CTinylates::parseIfTag(const std::string_view& sv) {
    std::string_view cond = trim(sv.substr(5));

    if (!m_props.contains(std::string{cond}))
        return false;

    const auto& PROP = m_props.at(std::string{cond});

    return PROP.truthy();
}

std::vector<std::string> CTinylates::parseForTag(const std::string_view& sv) {
    std::string_view cond = trim(sv.substr(6));

    if (!m_props.contains(std::string{cond}))
        return {};

    const auto& PROP = m_props.at(std::string{cond});

    if (PROP.m_type == TL_PROP_STR_VECTOR)
        return std::any_cast<std::vector<std::string>>(PROP.m_data);

    if (PROP.m_type == TL_PROP_INT) {
        std::vector<std::string> v;
        v.resize(std::any_cast<int>(PROP.m_data));
        return v;
    }

    return {};
}

std::string CTinylates::renderInternal(const std::string& input) {
    std::ostringstream stream;
    size_t             currentPos = 0, lastPos = 0;

    while (true) {
        currentPos = input.find("{{tl:", lastPos);
        if (currentPos == std::string::npos)
            currentPos = input.find("{{ tl:", lastPos);
        if (currentPos == std::string::npos)
            break;

        stream << input.substr(lastPos, currentPos - lastPos);

        // parse the template and return whatever it did
        size_t closingTag = input.find("}}", currentPos);
        if (closingTag == std::string::npos)
            return "";

        const auto TAG  = trim(std::string_view{input}.substr(currentPos + 2, closingTag - currentPos - 2));
        const auto NEXT = tagNeedsNext(TAG);

        if (NEXT.empty()) {
            stream << parseTagData(TAG);

            lastPos    = closingTag + 2;
            currentPos = closingTag + 2;
            continue;
        }

        // tag needs a next tag. Seek its closer and recurse
        int    tagStack  = 1;
        size_t closerPos = closingTag + 2;
        while (tagStack > 0) {
            closerPos = input.find("{{ tl:", closerPos);
            if (closerPos == std::string::npos)
                closerPos = input.find("{{tl:", closerPos);
            if (closerPos == std::string::npos)
                return ""; // unclosed, what do we do?

            size_t endTag = input.find("}}", closerPos + 4);
            if (endTag == std::string::npos)
                return ""; // unclosed again

            const auto NEWTAG = trim(std::string_view{input}.substr(closerPos + 2, endTag - closerPos - 2));

            // TODO: this could be better.
            if (TAG.starts_with("tl:if")) {
                if (NEWTAG.starts_with("tl:if"))
                    tagStack++;
                else if (NEWTAG.starts_with("tl:endif"))
                    tagStack--;
            }

            if (TAG.starts_with("tl:for")) {
                if (NEWTAG.starts_with("tl:for"))
                    tagStack++;
                else if (NEWTAG.starts_with("tl:endfor"))
                    tagStack--;
            }

            if (tagStack > 0)
                closerPos = endTag + 2;
        }

        size_t closerEndPos = input.find("}}", closerPos + 4);
        if (closerEndPos == std::string::npos)
            return "";

        const auto NEWTAG = trim(std::string_view{input}.substr(closerPos + 2, closerEndPos - closerPos - 2));

        if (TAG.starts_with("tl:if")) {
            if (!NEWTAG.starts_with("tl:endif"))
                return ""; // mismatched hierarchy

            lastPos    = closerEndPos + 2;
            currentPos = closerEndPos + 2;

            if (parseIfTag(TAG)) {
                // if passed. Include in stream, but parse first.
                stream << renderInternal(input.substr(closingTag + 2, closerPos - closingTag - 2));
            }

            continue;
        } else if (TAG.starts_with("tl:for")) {
            if (!NEWTAG.starts_with("tl:endfor"))
                return ""; // mismatched hierarchy

            lastPos    = closerEndPos + 2;
            currentPos = closerEndPos + 2;

            const auto VEC = parseForTag(TAG);

            for (size_t i = 0; i < VEC.size(); ++i) {
                m_props.emplace("i", CTinylatesProp((int)i));
                m_props.emplace("n", CTinylatesProp(VEC.at(i)));
                stream << renderInternal(input.substr(closingTag + 2, closerPos - closingTag - 2));
                m_props.erase("i");
                m_props.erase("n");
            }
        }
    }

    stream << input.substr(lastPos, currentPos);

    return stream.str();
}

std::optional<std::string> CTinylates::render() {
    return renderInternal(m_html);
}