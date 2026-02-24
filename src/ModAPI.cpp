#include "ModAPI.h"
#include "DecayTracker.h"

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
			return DecayTracker::GetInstance().IsDecaying(*skill);
		}
		return false;
	}

	bool DecayInterface::IsDecaying(Skill skill) noexcept
	{
		return DecayTracker::GetInstance().IsDecaying(skill);
	}

	void DecayInterface::DecaySkill(RE::ActorValue avSkill, float decayXP, bool decayLevels) noexcept
	{
		if (const auto skill = ToSkill(avSkill)) {
			DecayTracker::GetInstance().DecaySkill(*skill, decayXP, decayLevels);
		}
	}

	void DecayInterface::DecaySkill(Skill skill, float decayXP, bool decayLevels) noexcept
	{
		DecayTracker::GetInstance().DecaySkill(skill, decayXP, decayLevels);
	}
}
