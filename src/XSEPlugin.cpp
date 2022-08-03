
#include "Hooks.h"
#include "HUDManager.h"

#include "Settings.cpp"

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			if (!SmoothCamAPI::RegisterInterfaceLoaderCallback(SKSE::GetMessagingInterface(),
					[](void* interfaceInstance, SmoothCamAPI::InterfaceVersion interfaceVersion) {
						if (interfaceVersion == SmoothCamAPI::InterfaceVersion::V3) {
							HUDManager::GetSingleton()->g_SmoothCam = reinterpret_cast<SmoothCamAPI::IVSmoothCam3*>(interfaceInstance);
							logger::info("Obtained SmoothCam API");
						} else {
							logger::error("Unable to acquire requested SmoothCam API interface version");
						}
					})) {
				logger::warn("SmoothCamAPI::RegisterInterfaceLoaderCallback reported an error");
			}

			HUDManager::GetSingleton()->g_TDM = reinterpret_cast<TDM_API::IVTDM2*>(TDM_API::RequestPluginAPI(TDM_API::InterfaceVersion::V2));
			if (HUDManager::GetSingleton()->g_TDM) 
				logger::info("Obtained TDM API");
			else
				logger::info("Unable to acquire TDM API");

			HUDManager::GetSingleton()->g_BTPS = reinterpret_cast<BTPS_API_decl::API_V0*>(BTPS_API_decl::RequestPluginAPI_V0());
			if (HUDManager::GetSingleton()->g_BTPS)
				logger::info("Obtained BTPS API");
			else
				logger::info("Unable to acquire BTPS API");

			if (HUDManager::GetSingleton()->g_DetectionMeter = LoadLibrary(L"Data/SKSE/Plugins/MaxsuDetectionMeter.dll"))
				logger::info("Obtained Detection Meter DLL");
			else
				logger::info("Unable to acquire Detection Meter DLL");

			break;


		case SKSE::MessagingInterface::kPostPostLoad:
			if (!SmoothCamAPI::RequestInterface(
					SKSE::GetMessagingInterface(),
					SmoothCamAPI::InterfaceVersion::V3))
				logger::warn("SmoothCamAPI::RequestInterface reported an error");
			break;

		case SKSE::MessagingInterface::kPreLoadGame:
			Settings::GetSingleton()->LoadSettings();
			break;

	}
}


void Init()
{
	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
	Hooks::Install();
	Settings::GetSingleton()->LoadSettings();
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

	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	Init();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName("PluginName");
	v.PluginVersion({ 1, 0, 0, 0 });
	v.UsesAddressLibrary(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
