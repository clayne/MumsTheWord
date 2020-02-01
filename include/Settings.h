#pragma once

#include "Json2Settings.h"


class Settings
{
public:
	using bSetting = Json2Settings::bSetting;
	using iSetting = Json2Settings::iSetting;


	Settings() = delete;

	static bool LoadSettings(bool a_dumpParse = false);


	static bSetting useThreshold;
	static iSetting costThreshold;

private:
	static inline constexpr char FILE_NAME[] = "Data\\SKSE\\Plugins\\MumsTheWord.json";
};
