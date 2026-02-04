#include "SkillUsage.h"
#include "Options.h"
#include <cassert>
#include <algorithm>
#include "RE/A/ActorValueList.h"
#include "RE/P/PlayerCharacter.h"

namespace Decay
{
	void SkillUsage::Init(Skill skill)
	{
		this->skill = skill;
		decayGracePeriod = Settings::fDecayGracePeriodHours(skill);
		decayInterval = Settings::fDecayIntervalHours(skill);
		baselineLevel = Settings::iAVDSkillStart();
		baselineLevelBonus = Settings::iMaxRaceBonus();

		decayLevelOffset = Settings::iDecayLevelOffset(skill);
		decayXPMult = Settings::fDecayXPMult(skill);

		raceSkillBonus = 0; // default to 0, in case there is value from previous race (when changing races through RaceMenu)
		for (const auto& boost : Player->GetRace()->data.skillBoosts) {
			const auto skillIndex = boost.skill.underlying() - 6;
			if (skillIndex >= 0 && skillIndex < Skill::kTotal) {
				if (static_cast<Skill>(skillIndex) == skill) {
					raceSkillBonus = boost.bonus;
					break;
				}
			}
		}
	}

	void SkillUsage::Revert()
	{
		daysPassedWhenLastUsed = 0;
		lastKnownLevel = -1;
		lastKnownXP = -1;
		isDecaying = false;
		daysPassedSinceLastDecay = 0;
	}

	bool SkillUsage::IsInitialized() const
	{
		return lastKnownLevel >= 0 && lastKnownXP >= 0;
	}	

	bool SkillUsage::WasUsed() const
	{
		auto level = Player->GetBaseActorValue(AV(skill));
		auto& skillData = Player->skills->data->skills[skill];
		return level > lastKnownLevel || ((skillData.xp - lastKnownXP) > 0.5f);  // 0.5f to make sure that we only count proper XP gains (at least +1)
	}

	void SkillUsage::SetUsed(const RE::Calendar* calendar)
	{
		lastKnownLevel = Player->GetBaseActorValue(AV(skill));
		lastKnownXP = Player->skills->data->skills[skill].xp;
		daysPassedWhenLastUsed = calendar->GetDaysPassed();
		isDecaying = false;
	}

	bool SkillUsage::IsStale(const RE::Calendar* calendar) const
	{
		// If already decaying, no need to check further
		if (isDecaying)
			return false;

		auto hoursPassed = (calendar->GetDaysPassed() - daysPassedWhenLastUsed) * 24.0f;
		return hoursPassed >= decayInterval;
	}

	void SkillUsage::MarkDecaying(const RE::Calendar* calendar)
	{
		isDecaying = true;
		daysPassedSinceLastDecay = calendar->GetDaysPassed();
	}

	void SkillUsage::Decay(const RE::Calendar* calendar)
	{
		assert(isDecaying);

		auto& skillData = Player->skills->data->skills[skill];

		const float daysPassed = calendar->GetDaysPassed();
		const auto  hoursPassed = (daysPassed - daysPassedSinceLastDecay) * 24.0f;

		const auto av = static_cast<RE::ActorValue>(skill + 6);

		float decayXP = hoursPassed * CalculateLevelThresholdXP(GetDecayTargetLevel()) / decayInterval;

		DecaySkill(av, skillData, decayXP);

		lastKnownLevel = Player->GetBaseActorValue(AV(skill));
		lastKnownXP = skillData.xp;
		daysPassedSinceLastDecay = daysPassed;
	}

	void SkillUsage::DecaySkill(RE::ActorValue av, SkillData& skillData, float& decayXPAmount)
	{
		if (decayXPAmount <= 0.0f)
			return;

		float level = Player->GetBaseActorValue(av);
		
		if (skillData.xp >= decayXPAmount) {
			skillData.xp -= decayXPAmount;
			decayXPAmount = 0.0f;
		} else if (level <= GetStartingLevel()) {
			// We can't decay any further, so just reset XP.
			skillData.xp = 0.0f;
			decayXPAmount = 0.0f;
		} else {
			decayXPAmount -= skillData.xp;
			const float threshold = CalculateLevelThresholdXP(static_cast<int>(level));
			skillData.xp = max(0, threshold - 1);  // -1 to be safe, so that we won't end up in invalid state where xp == levelThreshold.
			skillData.levelThreshold = threshold;
			Player->ModBaseActorValue(av, -1);
			// skillData.level is only updated after player confirms level up (in Skills Menu).
			// Before that, skillData.level will remain at the last confirmed level, even if GetBaseAV's level is further.
			if (level == skillData.level) {
				skillData.level -= 1;
			}
			DecaySkill(av, skillData, decayXPAmount);
		}
	}
	inline int SkillUsage::GetDecayTargetLevel() const
	{
		// Level 2 is the smallest we can go to avoid Decay XP equaling zero.
		return max(2, baselineLevel + baselineLevelBonus - raceSkillBonus - decayLevelOffset);
	}
	inline float SkillUsage::CalculateLevelThresholdXP(int level) const
	{
		if (skill == Skill::kTotal)
			return 0.0f;

		const auto av = static_cast<RE::ActorValue>(skill + 6);
		const auto avi = RE::ActorValueList::GetActorValueInfo(av);

		const auto mult = avi->skill->improveMult;
		const auto offset = avi->skill->improveOffset;
		const auto curve = Settings::fSkillUseCurve();

		return mult * std::pow(level - 1.0f, curve) + offset;
	}
}
