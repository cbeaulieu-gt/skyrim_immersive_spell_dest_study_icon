#include "PCH.h"
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "DIII_API.h"

namespace logger = SKSE::log;

namespace
{
    constexpr RE::FormID kLastSpellTomeFormID = 0x1A000805;
    constexpr std::string_view kDIIISender = "DynamicInventoryIconInjector";
    constexpr std::string_view kImmSpellLearningRuleName = "currentlyLearningSpell";

    std::atomic_bool g_dataLoaded{false};
    std::atomic_bool g_diiiListenerRegistered{false};
    std::atomic_bool g_lastSpellTomeRuleDisabled{false};
    std::once_flag g_lastSpellTomeInitOnce;
    RE::BGSListForm *g_lastSpellTomeList = nullptr;

    void InitializeLog()
    {
        const auto logDir = SKSE::log::log_directory();
        if (!logDir)
        {
            std::terminate();
        }

        const auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
        const auto logPath = *logDir / fmt::format("{}.log", pluginName);

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto log = std::make_shared<spdlog::logger>("global", std::move(sink));

        spdlog::set_default_logger(std::move(log));
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
    }

    void InitializeLastSpellTomeList()
    {
        g_lastSpellTomeList = RE::TESForm::LookupByID<RE::BGSListForm>(kLastSpellTomeFormID);
        if (!g_lastSpellTomeList)
        {
            logger::warn("LastSpellTome form list {:08X} was not found. Disabling '{}' rule.",
                         kLastSpellTomeFormID,
                         kImmSpellLearningRuleName);
            g_lastSpellTomeRuleDisabled.store(true);
            return;
        }

        logger::info("Resolved LastSpellTome form list {:08X}", kLastSpellTomeFormID);
    }

    RE::TESObjectBOOK *GetBookFromEntry(RE::InventoryEntryData *entry)
    {
        if (!entry)
        {
            return nullptr;
        }

        return entry->GetObject() ? entry->GetObject()->As<RE::TESObjectBOOK>() : nullptr;
    }

    bool TeachesSpell(RE::TESObjectBOOK *book)
    {
        if (!book)
        {
            return false;
        }

        return book->TeachesSpell();
    }

    class LastSpellTomeCondition final : public DIII::ICondition
    {
    public:
        explicit LastSpellTomeCondition(bool expected) : _expected(expected) {}

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

            auto *book = GetBookFromEntry(a_entry);
            if (!TeachesSpell(book))
            {
                return false;
            }

            return g_lastSpellTomeList->HasForm(book);
        }

    private:
        bool _expected;
    };

    bool RegisterStudyingSpellTomeCondition(DIII::IAPI *api, const char *name)
    {
        const bool registered = api->RegisterCondition(
            name,
            [name](const Json::Value &value, RE::FormType type) -> std::unique_ptr<DIII::ICondition>
            {
                if (type != RE::FormType::Book)
                {
                    logger::warn("{} should only be used for Book entries, got form type {}", name, static_cast<std::uint32_t>(type));
                }

                if (!value.isBool())
                {
                    logger::warn("{} expects boolean JSON value", name);
                    return nullptr;
                }

                logger::info("{} condition builder accepted value={}", name, value.asBool());
                return std::make_unique<LastSpellTomeCondition>(value.asBool());
            });

        logger::info("{} registration {}", name, registered ? "succeeded" : "failed");
        return registered;
    }

    void OnDIIIRegistration(SKSE::MessagingInterface::Message *message)
    {
        logger::info("Received DIII registration message");
        if (!message || message->type != DIII::kMessage_GetAPI || !message->data)
        {
            return;
        }

        auto *api = static_cast<DIII::IAPI *>(message->data);
        if (api->GetVersion() < 1)
        {
            logger::warn("DIII API version {} is unsupported", api->GetVersion());
            return;
        }

        logger::info("Connected to DIII API v{}", api->GetVersion());
        RegisterStudyingSpellTomeCondition(api, "currentlyLearningSpell");
    }

    void TryRegisterDIIIListener()
    {
        if (g_diiiListenerRegistered.load())
        {
            return;
        }

        const auto messaging = SKSE::GetMessagingInterface();
        if (!messaging)
        {
            return;
        }

        if (!messaging->RegisterListener(kDIIISender.data(), OnDIIIRegistration))
        {
            logger::warn("DIII sender '{}' is not ready yet; will retry later", kDIIISender);
            return;
        }

        g_diiiListenerRegistered.store(true);
        logger::info("DIII registration listener enabled for sender '{}'", kDIIISender);
    }

    void OnSKSELifecycle(SKSE::MessagingInterface::Message *message)
    {
        if (!message)
        {
            return;
        }

        if (message->type == SKSE::MessagingInterface::kPostPostLoad)
        {
            TryRegisterDIIIListener();
            return;
        }

        if (message->type == SKSE::MessagingInterface::kDataLoaded)
        {
            g_dataLoaded.store(true);
            TryRegisterDIIIListener();
            logger::info("Data loaded event received");
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse)
{
    InitializeLog();
    logger::info("imm_spell_learning_icon_skse init start");

    SKSE::Init(skse);
    logger::info("SKSE initialized");

    if (const auto messaging = SKSE::GetMessagingInterface())
    {
        if (!messaging->RegisterListener(OnSKSELifecycle))
        {
            logger::error("Failed to register SKSE lifecycle listener");
            return false;
        }
        logger::info("Registered SKSE lifecycle listener");
    }
    else
    {
        logger::error("Failed to acquire messaging interface");
        return false;
    }

    logger::info("imm_spell_learning_icon_skse ready");
    return true;
}
