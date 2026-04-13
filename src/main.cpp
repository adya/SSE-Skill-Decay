#include "DecayTracker.h"
#include "Hooks.h"
#include "ModAPI.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:

		SKSE::GetMessagingInterface()->Dispatch(
			Decay::MESSAGE_TYPE::kSkillDecayInterface,
			Decay::DecayInterface::GetSingleton(),
			sizeof(Decay::DecayInterface),
			nullptr);
		Decay::Install();
		Decay::DecayTracker::Register();
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		Decay::DecayTracker::GetInstance().LoadDefaultSkills();
		Decay::DecayTracker::GetInstance().LoadSettings();
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
	case SKSE::MessagingInterface::kNewGame:
		Decay::DecayTracker::GetInstance().LoadSettings();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("SkillDecay");
	v.AuthorName("sasnikol");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "SkillDecay";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_LATEST) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = SKSE::log::log_directory();
	if (!path) {
		SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
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

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse, false);

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}

extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI(const SkillDecay::InterfaceVersion a_interfaceVersion)
{
	const auto api = Decay::DecayInterface::GetSingleton(a_interfaceVersion);

	logger::info("SkillDecay::RequestPluginAPI called, InterfaceVersion {}", static_cast<std::underlying_type<SkillDecay::InterfaceVersion>::type>(a_interfaceVersion));

	switch (a_interfaceVersion) {
	case SkillDecay::InterfaceVersion::kV1:
		logger::info("SkillDecay::RequestPluginAPI returned the API singleton");
		return api;
	}

	logger::info("SkillDecay::RequestPluginAPI requested the wrong interface version");
	return nullptr;
}
