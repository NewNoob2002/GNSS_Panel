#ifndef CORE_LOG_H
#define CORE_LOG_H

#include <HardwareSerial.h>

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#define LOG_LEVEL LOG_LEVEL_DEBUG

#define LOG_ERROR(msg) if(LOG_LEVEL >= LOG_LEVEL_ERROR) { Serial.printf("[ERROR] %s\n", msg); }
#define LOG_WARNING(msg) if(LOG_LEVEL >= LOG_LEVEL_WARNING) { Serial.printf("[WARNING] %s\n", msg); }
#define LOG_INFO(msg) if(LOG_LEVEL >= LOG_LEVEL_INFO) { Serial.printf("[INFO] %s\n", msg); }
#define LOG_DEBUG(msg) if(LOG_LEVEL >= LOG_LEVEL_DEBUG) { Serial.printf("[DEBUG] %s\n", msg); }

#endif

