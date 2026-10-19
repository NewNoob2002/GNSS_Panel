#include "I18n.h"

namespace {

const char* const texts[] = {
    "PRESS",
    "Keep Press Shutdown\nShort Press Return",
    "Warning: Do Not Shut Down",
    "Warning: Saving System Config",
    "Warning: Saving System Config\nInfo: %s",
    "Shutdown Board",
    "Shutdown LowBattery",
    "Shutdown Forced",
    "Shutdown PushKey",
    "XYZ\nRinex",
    "infinite\n15min\n1hour\n2hour\n4hour\n24hour",
    "TRIMTALK\nTRIMMK3\nTT450S\nTRANSEOT\nSOUTH\nHUACE\nSATEL\nCSS",
    "CUSTOM\n[1]455.05\n[2]456.05\n[3]457.05\n[4]458.05\n[5]459.05\n[6]460.05\n[7]461.05\n[8]462.05\n[9]463.05",
    "Work",
    "Mode\nRadio\nNtrip",
    "GPS",
    "Latitude\nLongitude\nAltitude\n",
    "WIFI",
    "Mode\nStatus\nIp",
    "Battery",
    "Usage\nVoltage\nTemperature\nStatus",
    "Storage",
    "Status\nName\nSize\nType\nInterval",
    "System",
    "Firmware\nRunTime\nErrorCount\nBuild\n",
    "Rover",
    "Base",
    "ON",
    "OFF",
    "Server",
    "Client",
    "N/A",
    "externalPower",
    "notCharge",
    "fastCharge",
    "normalCharge",
    "BATTERY HOT %.1f C - PLACE IN COOL PLACE",
    "Hour",
    "BASE",
    "NONE",
    "SINGLE",
    "FIX",
    "FLOAT",
    "STAR MAP",
    "GPS",
    "BDS",
    "GLONASS",
    "GALILEO",
    "SBAS",
    "QZSS",
    "IRNSS",
};

static_assert(sizeof(texts) / sizeof(texts[0]) == static_cast<size_t>(I18n::TextId::Count));

} // namespace

const I18n::LanguagePack&
I18n::EnglishPack() {
    static const LanguagePack pack = {Language::English, texts};
    return pack;
}
