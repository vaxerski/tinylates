#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <string_view>
#include <any>

enum eTinylatePropType : unsigned short {
    TL_PROP_INVALID = 0,
    TL_PROP_STRING,
    TL_PROP_INT,
    TL_PROP_STR_VECTOR,
};

class CTinylatesProp {
  public:
    CTinylatesProp(const std::string& data);
    CTinylatesProp(const int& data);
    CTinylatesProp(const std::vector<std::string>& data);

  private:
    eTinylatePropType m_type = TL_PROP_INVALID;
    std::any          m_data;

    bool              truthy() const;

    friend class CTinylates;
};

class CTinylates {
  public:
    CTinylates(const std::string& html);

    void                       add(const std::string& name, const CTinylatesProp& prop);
    void                       setTemplateRoot(const std::string& path);

    std::optional<std::string> render();

  private:
    std::string                                     m_html, m_templateRoot;
    std::unordered_map<std::string, CTinylatesProp> m_props;

    std::string                                     renderInternal(const std::string& input);

    std::string                                     tagNeedsNext(const std::string_view& sv);

    bool                                            parseIfTag(const std::string_view& sv);
    std::vector<std::string>                        parseForTag(const std::string_view& sv);

    std::string                                     parseTagData(const std::string_view& sv);
    std::string                                     parseTagText(const std::string_view& sv);
    std::string                                     parseTagInclude(const std::string_view& sv);
};