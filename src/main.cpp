#include "Version.h"
#include "api/Papyrus.h"
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

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
	auto logLevel = spdlog::level::info;
#ifdef _DEBUG
	logLevel = spdlog::level::debug;
#endif

	const auto initInfo = SKSE::InitInfo {
		.log = true,
		.logLevel = logLevel,
		.trampoline = true,
		.trampolineSize = hooks::getHookTrampolineSize()
	};
	SKSE::Init(a_skse, initInfo);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	try {
		Settings::GetSingleton()->Load();
	} catch (...) {
		logger::error("Exception caught when loading settings! Default settings will be used");
	}

	hooks::InstallHooks();

	auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);
	return true;
}