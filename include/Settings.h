#pragma once

#include "Json2Settings.h"


class Settings : public Json2Settings::Settings
{
public:
	Settings() = delete;
	static bool loadSettings(bool a_dumpParse = false);


	static bSetting	useThreshold;
	static iSetting	costThreshold;

private:
	static inline constexpr char FILE_NAME[] = "Data\\SKSE\\Plugins\\MumsTheWord.json";
};
