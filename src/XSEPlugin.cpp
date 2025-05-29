#include <Windows.h>
#include <string>
#include "ActorUpdateHandler.h"

float fHeavyBody = 0.14f;
float fLightBody = 0.07f;
float fHeavyFeet = 0.07f;
float fLightFeet = 0.035f;
float fHeavyHands = 0.05f;
float fLightHands = 0.025f;
float fHeavyHead = 0.04f;
float fLightHead = 0.02f;
float fClothBody = 0.023f;
float fClothFeet = 0.010f;
float fClothHands = 0.007f;
float fClothHead = 0.003f;


float GetPrivateProfileFloat(const char* section, const char* key, float defaultValue, const std::string& filePath)
{
	char buffer[256];
	GetPrivateProfileStringA(section, key, "", buffer, sizeof(buffer), filePath.c_str());

	try {
		return std::stof(buffer);
	} catch (...) {
		return defaultValue;
	}
}

void LoadSwimArmorPenalties(const std::string& configFile)
{
	constexpr auto section = "ArmorSwimPenalty";

	fHeavyBody = GetPrivateProfileFloat(section, "fHeavyBody", fHeavyBody, configFile);
	fLightBody = GetPrivateProfileFloat(section, "fLightBody", fLightBody, configFile);
	fHeavyFeet = GetPrivateProfileFloat(section, "fHeavyFeet", fHeavyFeet, configFile);
	fLightFeet = GetPrivateProfileFloat(section, "fLightFeet", fLightFeet, configFile);
	fHeavyHands = GetPrivateProfileFloat(section, "fHeavyHands", fHeavyHands, configFile);
	fLightHands = GetPrivateProfileFloat(section, "fLightHands", fLightHands, configFile);
	fHeavyHead = GetPrivateProfileFloat(section, "fHeavyHead", fHeavyHead, configFile);
	fLightHead = GetPrivateProfileFloat(section, "fLightHead", fLightHead, configFile);
	fClothBody = GetPrivateProfileFloat(section, "fClothBody", fClothBody, configFile);
	fClothFeet = GetPrivateProfileFloat(section, "fClothFeet", fClothFeet, configFile);
	fClothHands = GetPrivateProfileFloat(section, "fClothHands", fClothHands, configFile);
	fClothHead = GetPrivateProfileFloat(section, "fClothHead", fClothHead, configFile);
}

void Init()
{
	ActorUpdateHandler::InstallHooks();
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();

	
	LoadSwimArmorPenalties("Data\\SKSE\\Plugins\\WadeInWaterRedux.ini");
	logger::info("Loaded plugin & Config: Data/SKSE/Plugins/WadeInWaterRedux.ini");
	SKSE::Init(a_skse);

	Init();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse();
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}


