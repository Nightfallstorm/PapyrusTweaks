#include "ExperimentalHooks.h"
#include "LoggerHooks.h"
#include "ModifyHooks.h"
#include "Papyrus.h"
#include "Settings.h"
#include "VRHooks.h"
#include "Version.h"

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

SKSEPluginInfo(
	.Version = {Project::Version::MAJOR, Project::Version::MINOR, Project::Version::PATCH},
	.Name = Project::NAME,
	.Author = Project::AUTHOR,
	.SupportEmail = "N/A",
	.StructCompatibility = SKSE::StructCompatibility::Independent,
)

extern "C" DLLEXPORT const char* APIENTRY GetPluginVersion()
{
	return Project::Version::NAME.data();
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	// TODO: Incorporate RecursionMonitor
	// TODO: Re-do log level appropriately
	// TODO: Re-evaluate logs to ensure no on-hook logging on release builds
	auto logLevel = spdlog::level::info;
#ifdef _DEBUG
	logLevel = spdlog::level::debug;
#endif

	constexpr auto totalTrampolineSize = ModifyHooks::hookTrampolineSize
	+ LoggerHooks::hookTrampolineSize
	+ VRHooks::hookTrampolineSize
	+ ExperimentalHooks::hookTrampolineSize;

	const auto initInfo = SKSE::InitInfo {
		.log = true,
		.logLevel = logLevel,
		.trampoline = true,
		.trampolineSize = totalTrampolineSize
	};
	SKSE::Init(a_skse, initInfo);

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