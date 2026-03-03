#include "PlayerSkillData.h"

namespace Decay
{
	PlayerSkillData::PlayerSkillData(Skill skill) :
		skill(skill),
		avi(RE::ActorValueList::GetActorValueInfo(AV(skill)))
	{}

	std::string_view PlayerSkillData::GetName() const noexcept
	{
		return SkillName(skill);
	}

	int PlayerSkillData::GetLevel() const noexcept
	{
		return Player->GetActorValue(AV(skill));
	}

	float PlayerSkillData::GetXP() const noexcept
	{
		return Player->skills->data->skills[skill].xp;
	}

	void PlayerSkillData::SetXP(float xp) noexcept
	{
		Player->skills->data->skills[skill].xp = xp;
	}

	int PlayerSkillData::GetRaceBonus() noexcept
	{
		for (const auto& boost : Player->GetRace()->data.skillBoosts) {
			const auto skillIndex = boost.skill.underlying() - 6;
			if (skillIndex >= 0 && skillIndex < Skill::kTotal) {
				if (static_cast<Skill>(skillIndex) == skill) {
					return boost.bonus;
				}
			}
		}

		return 0;
	}

	int PlayerSkillData::GetLegendaryLevel() const noexcept
	{
		return Player->skills->data->legendaryLevels[skill];
	}

	void PlayerSkillData::ModLevel(int mod) noexcept
	{
		auto& skillData = Player->skills->data->skills[skill];
		auto  level = Player->GetBaseActorValue(AV(skill));

		Player->ModBaseActorValue(AV(skill), mod);
		// skillData.level is only updated after player confirms level up (in Skills Menu).
		// Before that, skillData.level will remain at the last confirmed level, even if GetBaseAV's level is further.
		if (level == skillData.level) {
			skillData.level += mod;
			skillData.levelThreshold = CalculateLevelThresholdXP(skillData.level);
		}
	}

	RE::ActorValueInfo* PlayerSkillData::GetAVInfo() const noexcept
	{
		return avi;
	}
}
