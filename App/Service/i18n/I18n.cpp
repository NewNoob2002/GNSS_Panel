#include "I18n.h"
#include <cstdio>
#include <cstring>

namespace {

#if defined(APP_LANG_RU)
i18n::Language current_language = i18n::Language::Russian;
#else
i18n::Language current_language = i18n::Language::English;
#endif

const i18n::LanguagePack& get_pack(const i18n::Language language) {
    switch (language) {
        case i18n::Language::Russian:
            return i18n::russian_pack();
        case i18n::Language::English:
        default:
            return i18n::english_pack();
    }
}

} // namespace

namespace i18n {

Language get_language() {
    return current_language;
}

bool set_language(const Language language) {
    if (language != Language::English && language != Language::Russian)
        return false;

    if (language == current_language) {
        return false;
    }

#if APP_I18N_DYNAMIC
    current_language = language;
    return true;
#else
    return false;
#endif
}

const char* text(const TextId id) {
    return text(current_language, id);
}

const char* text(const Language language, const TextId id) {
    const size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(TextId::Count)) {
        return "";
    }

    const char* entry = get_pack(language).texts[index];
    if (entry != nullptr) {
        return entry;
    }

    entry = english_pack().texts[index];
    return entry != nullptr ? entry : "";
}

void option_text(TextId id, unsigned index, char* buffer, std::size_t size) {
    if (!buffer || !size)
        return;
    const char* entry = text(id);
    for (unsigned line = 0; line < index && entry; ++line) {
        entry = std::strchr(entry, '\n');
        entry = entry ? entry + 1 : nullptr;
    }
    const char* end = entry ? std::strchr(entry, '\n') : nullptr;
    const int length = entry ? static_cast<int>(end ? end - entry : std::strlen(entry)) : 0;
    std::snprintf(buffer, size, "%.*s", length, entry ? entry : "");
}

} // namespace i18n
