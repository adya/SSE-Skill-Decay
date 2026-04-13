#include "CustomSkillData.h"

namespace Decay
{
	std::string_view CustomSkillData::GetName() const noexcept
	{
		return skillId;
	}

	int CustomSkillData::GetLevel() const noexcept
	{
		return levelGlobal ? static_cast<int>(levelGlobal->value) : 0;
	}

	float CustomSkillData::GetXP() const noexcept
	{
		if (!xpGlobal)
			return 0.0f;

		float xp = xpGlobal->value;
		if (xpNormalized) {
			float threshold = CalculateLevelThresholdXP(GetLevel());
			return xp * threshold;
		}
		return xp;
	}

	void CustomSkillData::SetXP(float xp) noexcept
	{
		if (!xpGlobal)
			return;

		xp = max(0.0f, xp);

		if (xpNormalized) {
			if (float threshold = CalculateLevelThresholdXP(GetLevel()); threshold > 0) {
				xpGlobal->value = xp / threshold;
			}
		} else {
			xpGlobal->value = xp;
		}
	}

	void CustomSkillData::UpdateRaceBonus() noexcept
	{
		raceBonus = raceBonuses[Player->GetRace()->formID];
	}

	int CustomSkillData::GetLegendaryLevel() const noexcept
	{
		return legendaryGlobal ? static_cast<int>(legendaryGlobal->value) : 0;
	}

	void CustomSkillData::ModLevel(int mod) noexcept
	{
		if (!levelGlobal)
			return;

		levelGlobal->value = max(0, levelGlobal->value + mod);
	}

	RE::ActorValueInfo* CustomSkillData::GetAVInfo() const noexcept
	{
		return avi;
	}
}
