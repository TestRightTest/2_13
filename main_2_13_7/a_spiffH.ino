#include <SPIFFS.h>
#include <FS.h>

uint32_t oldestTestCount = UINT32_MAX;
uint8_t oldestChannelID;

uint32_t MAX_TEST_COUNT = 2600;
