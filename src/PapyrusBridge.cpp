#include "PapyrusBridge.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace logger = SKSE::log;

namespace
{
    bool MarkSpellLearnedIcon(RE::StaticFunctionTag *, RE::TESForm *a_spellOrBook)
    {
        if (!a_spellOrBook)
        {
            logger::warn("MarkSpellLearnedIcon called with null form");
            return false;
        }

        logger::info("MarkSpellLearnedIcon called for form {:08X}", a_spellOrBook->GetFormID());

        // TODO: Implement behavior that mirrors or augments the existing plugin.
        // This function is exposed to Papyrus for high-level quest/script orchestration.
        return true;
    }
}

bool PapyrusBridge::Register(RE::BSScript::IVirtualMachine *a_vm)
{
    if (!a_vm)
    {
        logger::error("Papyrus VM is null");
        return false;
    }

    constexpr auto scriptName = "DIII_SpellLearningBridge";
    a_vm->RegisterFunction("MarkSpellLearnedIcon", scriptName, MarkSpellLearnedIcon);

    logger::info("Registered Papyrus functions for {}", scriptName);
    return true;
}
