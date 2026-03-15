#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <atomic>
#include <mutex>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string_view>

#include "DIII_API.h"

namespace logger = SKSE::log;

namespace
{
    constexpr RE::FormID kLastSpellTomeFormID = 0x1A000805;
    constexpr std::string_view kLastSpellTomeRuleName = "lastSpellTome";

    std::atomic_bool g_dataLoaded{false};
    std::atomic_bool g_lastSpellTomeRuleDisabled{false};
    std::once_flag g_lastSpellTomeInitOnce;
    RE::BGSListForm *g_lastSpellTomeList = nullptr;

    void InitializeLastSpellTomeList()
    {
        g_lastSpellTomeList = RE::TESForm::LookupByID<RE::BGSListForm>(kLastSpellTomeFormID);
        if (!g_lastSpellTomeList)
        {
            logger::warn("LastSpellTome form list {:08X} was not found. Disabling '{}' rule.",
                         kLastSpellTomeFormID,
                         kLastSpellTomeRuleName);
            g_lastSpellTomeRuleDisabled.store(true);
            return;
        }

        logger::info("Resolved LastSpellTome form list {:08X}", kLastSpellTomeFormID);
    }

    class LastSpellTomeCondition final : public DIII::ICondition
    {
    public:
        bool Match(RE::InventoryEntryData *a_entry) const override
        {
            if (!a_entry)
            {
                return false;
            }

            if (!g_dataLoaded.load() || g_lastSpellTomeRuleDisabled.load())
            {
                return false;
            }

            std::call_once(g_lastSpellTomeInitOnce, InitializeLastSpellTomeList);
            if (!g_lastSpellTomeList || g_lastSpellTomeRuleDisabled.load())
            {
                return false;
            }

            const auto *object = a_entry->GetObject();
            const auto *book = object ? object->As<RE::TESObjectBOOK>() : nullptr;
            if (!book || !book->TeachesSpell())
            {
                return false;
            }

            return g_lastSpellTomeList->HasForm(book);
        }
    };

    std::unique_ptr<DIII::ICondition> BuildLastSpellTomeCondition(const Json::Value &, RE::FormType a_type)
    {
        if (a_type != RE::FormType::Book)
        {
            return nullptr;
        }

        return std::make_unique<LastSpellTomeCondition>();
    }

    void RegisterDIIIRules(DIII::IAPI *a_api)
    {
        if (!a_api)
        {
            logger::error("DIII API pointer was null");
            return;
        }

        if (a_api->RegisterCondition(kLastSpellTomeRuleName.data(), BuildLastSpellTomeCondition))
        {
            logger::info("Registered DIII condition rule '{}'", kLastSpellTomeRuleName);
        }
        else
        {
            logger::error("Failed to register DIII condition rule '{}'", kLastSpellTomeRuleName);
        }
    }

    void DIIIMessageHandler(SKSE::MessagingInterface::Message *a_msg)
    {
        if (!a_msg || a_msg->type != DIII::kMessage_GetAPI)
        {
            return;
        }

        auto *api = static_cast<DIII::IAPI *>(a_msg->data);
        if (!api)
        {
            logger::error("DIII API message did not include a valid API pointer");
            return;
        }

        logger::info("Received DIII API, version {}", api->GetVersion());
        RegisterDIIIRules(api);
    }
}

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
        g_dataLoaded.store(true);
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
        DIII::ListenForRegistration(DIIIMessageHandler);
    }
    else
    {
        logger::error("Failed to acquire messaging interface");
        return false;
    }

    logger::info("Plugin loaded successfully");
    return true;
}
