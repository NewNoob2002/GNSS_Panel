#ifndef I18N_H
#define I18N_H

#include "I18nTextId.h"

#ifndef APP_I18N_DYNAMIC
#define APP_I18N_DYNAMIC 1
#endif

/* Define APP_LANG_RU for a Russian-only/default build. English is the default. */

namespace i18n {

enum class Language {
    English = 0,
    Russian,
};

struct LanguagePack {
    Language language;
    const char* const* texts;
};

Language get_language();
bool set_language(Language language);
const char* text(TextId id);
const char* text(Language language, TextId id);

const LanguagePack& english_pack();
const LanguagePack& russian_pack();

} // namespace i18n

#endif
