#include "settings.h"


bool Settings::loadSettings(bool a_dumpParse)
{
	return Json2Settings::Settings::loadSettings(FILE_NAME, a_dumpParse);
}


decltype(Settings::useThreshold)	Settings::useThreshold("useThreshold", true, true);
decltype(Settings::costThreshold)	Settings::costThreshold("costThreshold", 500, true);
