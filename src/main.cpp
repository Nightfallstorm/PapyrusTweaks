#include "ExperimentalHooks.h"
#include "LoggerHooks.h"
#include "ModifyHooks.h"
#include "Papyrus.h"
#include "Settings.h"
#include "VRHooks.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPreLoadGame:
	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPostLoad:
	case SKSE::MessagingInterface::kPostLoadGame:
	case SKSE::MessagingInterface::kPostPostLoad:
	default:
		break;
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName(Version::PROJECT);
	v.AuthorName("Nightfallstorm");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST_AE });
	v.UsesNoStructs(true);

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();

	return true;
}

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		//stl::report_and_fail("Failed to find standard logging directory"sv); // Doesn't work in VR
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	try {
		Settings::GetSingleton()->Load();
	} catch (...) {
		logger::error("Exception caught when loading settings! Default settings will be used");
	}

	ModifyHooks::InstallHooks();
	LoggerHooks::InstallHooks();
	VRHooks::InstallHooks();
	ExperimentalHooks::InstallHooks();

	auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);
	return true;
}
