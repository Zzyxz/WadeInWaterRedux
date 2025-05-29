#include "ActorUpdateHandler.h"


extern float fHeavyBody;
extern float fLightBody;
extern float fHeavyFeet;
extern float fLightFeet;
extern float fHeavyHands;
extern float fLightHands;
extern float fHeavyHead;
extern float fLightHead;
extern float fClothBody;
extern float fClothFeet;
extern float fClothHands;
extern float fClothHead;


RE::TESForm* GetFormFromIdentifier(const std::string& identifier)
{
	auto dataHandler = RE::TESDataHandler::GetSingleton();
	auto delimiter = identifier.find('|');
	if (delimiter != std::string::npos) {
		// Handle the case when '|' is present in the identifier
		std::string modName = identifier.substr(0, delimiter);
		std::string modForm = identifier.substr(delimiter + 1);
		uint32_t    formID = std::stoul(modForm, nullptr, 16) & 0xFFFFFF;
		auto*       mod = dataHandler->LookupModByName(modName.c_str());
		if (mod && mod->IsLight())
			formID = std::stoul(modForm, nullptr, 16) & 0xFFF;
		return dataHandler->LookupForm(formID, modName.c_str());
	} else {
		return RE::TESForm::LookupByEditorID(identifier);
	}
	return nullptr;
}

float ActorUpdateHandler::GetWaterMultiplier(RE::Actor* a_actor)
{
	static RE::SpellItem*      WaterSlowdownSmall;
	static RE::SpellItem*      WaterSlowdownLarge;
	static RE::SpellItem*      WaterSlowdownSwim;
	static RE::TESDataHandler* dataHandler;
	if (!dataHandler) {
		dataHandler = RE::TESDataHandler::GetSingleton();
		if (dataHandler) {
			WaterSlowdownSmall = (RE::SpellItem*)GetFormFromIdentifier("WadeInWater.esp|D64");//dataHandler->LookupForm<RE::SpellItem>(0xD64, "WadeInWater.esp");
			WaterSlowdownLarge = (RE::SpellItem*)GetFormFromIdentifier("WadeInWater.esp|D65");  //dataHandler->LookupForm<RE::SpellItem>(0xD65, "WadeInWater.esp");
			WaterSlowdownSwim = (RE::SpellItem*)GetFormFromIdentifier("WadeInWater.esp|D67");
		}
	};

	if (!WaterSlowdownSmall || !WaterSlowdownLarge || !WaterSlowdownSwim)
		return 1.0f;

	auto submergedLevel = GetSubmergedLevel(a_actor);
	auto waterMultiplier = 1.0f;

	if (submergedLevel >= 0.69f) {
		if (!a_actor->HasSpell(WaterSlowdownSwim)) {
			a_actor->RemoveSpell(WaterSlowdownLarge);
			a_actor->RemoveSpell(WaterSlowdownSmall);
			a_actor->AddSpell(WaterSlowdownSwim);
		}

		waterMultiplier = CalculateSwimArmorSlowdown(a_actor, 1.0f - (WaterSlowdownSwim->effects[0]->effectItem.magnitude / 100));

	} else if (submergedLevel >= 0.4f) {
		if (!a_actor->HasSpell(WaterSlowdownLarge)) {
			a_actor->RemoveSpell(WaterSlowdownSwim);
			a_actor->RemoveSpell(WaterSlowdownSmall);
			a_actor->AddSpell(WaterSlowdownLarge);
		}
		waterMultiplier = 1.0f - (std::lerp(WaterSlowdownSmall->effects[0]->effectItem.magnitude, WaterSlowdownLarge->effects[0]->effectItem.magnitude, (submergedLevel - 0.2f) / (1 - 0.49f)) / 100);
	} else if (submergedLevel >= 0.2f) {
		if (!a_actor->HasSpell(WaterSlowdownSmall)) {
			a_actor->RemoveSpell(WaterSlowdownSwim);
			a_actor->RemoveSpell(WaterSlowdownLarge);
			a_actor->AddSpell(WaterSlowdownSmall);
		}
		waterMultiplier = 1.0f - (std::lerp(WaterSlowdownSmall->effects[0]->effectItem.magnitude, WaterSlowdownLarge->effects[0]->effectItem.magnitude, (submergedLevel - 0.2f) / (1 - 0.49f)) / 100);
	} else {
		a_actor->RemoveSpell(WaterSlowdownSwim);
		a_actor->RemoveSpell(WaterSlowdownLarge);
		a_actor->RemoveSpell(WaterSlowdownSmall);
		waterMultiplier = 1.0f - (std::lerp(0, WaterSlowdownSmall->effects[0]->effectItem.magnitude, submergedLevel * 5) / 100);
	}
	
	return waterMultiplier;
}


float ActorUpdateHandler::CalculateSwimArmorSlowdown(RE::Actor* actor, float baseReduction)
{
	if (!actor) {
		return baseReduction;
	}

	float totalReductionFactor = 0.0f;

	struct SlotPenalty
	{
		RE::BIPED_MODEL::BipedObjectSlot slot;
		float*                           heavyPenalty;
		float*                           lightPenalty;
		float*                           clothPenalty;
	};

	std::array<SlotPenalty, 4> slots = { { { RE::BIPED_MODEL::BipedObjectSlot::kBody, &fHeavyBody, &fLightBody, &fClothBody },
		{ RE::BIPED_MODEL::BipedObjectSlot::kFeet, &fHeavyFeet, &fLightFeet, &fClothFeet },
		{ RE::BIPED_MODEL::BipedObjectSlot::kHands, &fHeavyHands, &fLightHands, &fClothHands },
		{ RE::BIPED_MODEL::BipedObjectSlot::kHead, &fHeavyHead, &fLightHead, &fClothHead } } };

	for (const auto& entry : slots) {
		auto armor = actor->GetWornArmor(entry.slot);
		if (!armor) {
			continue;
		}

		auto armo = armor->As<RE::TESObjectARMO>();
		if (!armo) {
			continue;
		}

		switch (armo->GetArmorType()) {
		case RE::BGSBipedObjectForm::ArmorType::kHeavyArmor:
			totalReductionFactor += *entry.heavyPenalty;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kLightArmor:
			totalReductionFactor += *entry.lightPenalty;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kClothing:
			totalReductionFactor += *entry.clothPenalty;
			break;
		default:
			break;
		}
	}

	float finalSwimSpeed = std::clamp(baseReduction - totalReductionFactor, 0.1f, 1.0f);
	//logger::info("waterMultiplier set {}", finalSwimSpeed);
	return finalSwimSpeed;
}
