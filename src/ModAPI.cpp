#include "ModAPI.h"
#include "DecayTracker.h"
#include "CustomSkillData.h"

namespace Decay
{
	std::optional<Skill> ToSkill(RE::ActorValue avSkill) noexcept
	{
		if (avSkill < RE::ActorValue::kOneHanded || avSkill > RE::ActorValue::kEnchanting) {
			return std::nullopt;
		}
		auto rawSkill = static_cast<RE::ActorValue>(static_cast<std::underlying_type_t<RE::ActorValue>>(avSkill) - 6);
		return static_cast<Skill>(rawSkill);
	}

	bool DecayInterface::IsDecaying(RE::ActorValue avSkill) noexcept
	{
		if (const auto skill = ToSkill(avSkill)) {
			return DecayTracker::GetInstance()[*skill].IsDecaying();
		}
		return false;
	}

	bool DecayInterface::IsDecaying(Skill skill) noexcept
	{
		return DecayTracker::GetInstance()[skill].IsDecaying();
	}

	void DecayInterface::DecaySkill(RE::ActorValue avSkill, float decayXP, bool decayLevels) noexcept
	{
		if (const auto skill = ToSkill(avSkill)) {
			DecayTracker::GetInstance()[*skill].DecaySkill(Player->skills->data->skills[*skill], decayXP, decayLevels);
		}
	}

	void DecayInterface::DecaySkill(Skill skill, float decayXP, bool decayLevels) noexcept
	{
		DecayTracker::GetInstance()[skill].DecaySkill(Player->skills->data->skills[skill], decayXP, decayLevels);
	}

	void DecayInterface::ResetDecay(RE::ActorValue avSkill) noexcept
	{
		if (const auto skill = ToSkill(avSkill)) {
			DecayTracker::GetInstance()[*skill].ResetDecay();
		}
	}

	void DecayInterface::ResetDecay(Skill skill) noexcept
	{
		DecayTracker::GetInstance()[skill].ResetDecay();
	}

	bool DecayInterface::RegisterCustomSkill(
		const char*                       skillId,
		RE::ActorValueInfo*               avi,
		RE::TESGlobal*                    levelGlobal,
		RE::TESGlobal*                    xpGlobal,
		bool                              xpNormalized,
		RE::TESGlobal*                    legendaryGlobal,
		const std::pair<RE::FormID, int>* raceBonuses,
		size_t                            raceBonusesCount) noexcept
	{
		if (!skillId || skillId[0] == '\0') {
			return false;
		}

		if (!levelGlobal || !xpGlobal) {
			logger::error("Failed to register custom skill '{}': Level and XP globals are required.", skillId);
			return false;
		}
		if (!avi || !avi->skill) {
			logger::error("Failed to register custom skill '{}': ActorValueInfo with valid skill info is required.", skillId);
			return false;
		}

		if (PlayerSkillData::IsDefaultSkill(skillId)) {
			logger::error("Cannot register custom skill '{}' because it conflicts with default skill names.", skillId);
			return false;
		}

		auto& tracker = DecayTracker::GetInstance();

		std::map<RE::FormID, int> raceBonusesMap{};

		if (raceBonuses && raceBonusesCount > 0) {
			for (size_t i = 0; i < raceBonusesCount; ++i) {
				raceBonusesMap[raceBonuses[i].first] = raceBonuses[i].second;
			}
		}
		
		tracker.RegisterCustomSkill(
			skillId, avi,
			levelGlobal, xpGlobal, xpNormalized, legendaryGlobal,
			raceBonusesMap);

		logger::info("Registered custom skill '{}':", skillId);
		logger::info("\tUses {} XP values;", xpNormalized ? "normalized" : "absolute");
		logger::info("\tImproveMult: {:.3f}; ImproveOffset: {:.3f}", avi->skill->improveMult, avi->skill->improveOffset);
		logger::info("\t{} legendary level bonuses;", legendaryGlobal ? "Supports" : "Doesn't support");
		logger::info("\t{} race bonuses;", raceBonusesCount > 0 ? "Supports" : "Doesn't support");
		return true;
	}
}
