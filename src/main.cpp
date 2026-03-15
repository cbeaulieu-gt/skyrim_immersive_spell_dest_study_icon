#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <string_view>

#include "PapyrusBridge.h"

namespace logger = SKSE::log;

SKSEPluginInfo(
        .Version = REL::Version{0, 1, 0, 0},
        .Name = std::string_view{"DIII Dest Imm Spell Learning Icon"},
        .Author = std::string_view{"Chris"},
        .SupportEmail = std::string_view{""},
        .StructCompatibility = SKSE::StructCompatibility::Independent,
        .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
        .MinimumSKSEVersion = REL::Version{0, 0, 0, 0})

    void SetupLog()
{
    auto logPath = logger::log_directory();
    if (!logPath)
    {
        SKSE::stl::report_and_fail("Unable to find SKSE log directory");
    }

    *logPath /= "DIIIDestImmSpellLearningIcon.log";

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
}

void MessageHandler(SKSE::MessagingInterface::Message *a_msg)
{
    if (!a_msg)
    {
        return;
    }

    if (a_msg->type == SKSE::MessagingInterface::kDataLoaded)
    {
        logger::info("Data loaded event received");
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *a_skse)
{
    SetupLog();
    logger::info("Loading plugin");

    SKSE::Init(a_skse);

    if (const auto messaging = SKSE::GetMessagingInterface())
    {
        messaging->RegisterListener(MessageHandler);
    }
    else
    {
        logger::error("Failed to acquire messaging interface");
        return false;
    }

    if (const auto papyrus = SKSE::GetPapyrusInterface())
    {
        papyrus->Register(PapyrusBridge::Register);
    }
    else
    {
        logger::error("Failed to acquire papyrus interface");
        return false;
    }

    logger::info("Plugin loaded successfully");
    return true;
}
