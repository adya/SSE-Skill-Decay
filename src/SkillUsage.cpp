#include "SkillUsage.h"
#include "Options.h"
#include "RE/A/ActorValueList.h"
#include "RE/P/PlayerCharacter.h"
#include <algorithm>
#include <cassert>

namespace Decay
{
	void SkillUsage::Init(Skill skill, DecayConfig& config)
	{
		this->skill = skill;
		this->decay = std::move(config);

		baselineLevel = Settings::iAVDSkillStart();
		raceSkillBonus = 0;

		for (const auto& boost : Player->GetRace()->data.skillBoosts) {
			const auto skillIndex = boost.skill.underlying() - 6;
			if (skillIndex >= 0 && skillIndex < Skill::kTotal) {
				if (static_cast<Skill>(skillIndex) == skill && raceSkillBonus == 0) {
					raceSkillBonus = boost.bonus;
				}
				if (decay.baselineLevelOffset < 0 && boost.bonus > decay.baselineLevelOffset) {
					decay.baselineLevelOffset = boost.bonus;
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
		auto  level = Player->GetBaseActorValue(AV(skill));
		auto& skillData = Player->skills->data->skills[skill];
		return level > lastKnownLevel || ((skillData.xp - lastKnownXP) > 0.5f);  // 0.5f to make sure that we only count proper XP gains (at least +1)
	}

	void SkillUsage::SetUsed(const RE::Calendar* calendar)
	{
		lastKnownLevel = Player->GetBaseActorValue(AV(skill));
		lastKnownXP = Player->skills->data->skills[skill].xp;
		int legLevel = Player->skills->data->legendaryLevels[skill];
		if (legLevel > lastKnownLegendaryLevel) {
			lastKnownHighestLevel = GetStartingLevel();
		} else {
			lastKnownHighestLevel = max(lastKnownHighestLevel, lastKnownLevel);
		}
		lastKnownLegendaryLevel = legLevel;

		daysPassedWhenLastUsed = calendar->GetDaysPassed();
		isDecaying = false;
	}

	bool SkillUsage::IsStale(const RE::Calendar* calendar) const
	{
		// If already decaying, no need to check further
		if (isDecaying)
			return false;

		auto hoursPassed = (calendar->GetDaysPassed() - daysPassedWhenLastUsed) * 24.0f;
		return hoursPassed >= GetGracePeriod();
	}

	void SkillUsage::MarkDecaying(const RE::Calendar* calendar)
	{
		isDecaying = true;
		daysPassedSinceLastDecay = calendar->GetDaysPassed();
	}

	bool SkillUsage::IsDecaying() const
	{
		return isDecaying && Player->GetBaseActorValue(AV(skill)) > GetDecayCapLevel();  // If it can't decay any further, ignore the isDecaying flag.
	}

	void SkillUsage::Decay(const RE::Calendar* calendar)
	{
		assert(isDecaying);

		auto& skillData = Player->skills->data->skills[skill];

		const float daysPassed = calendar->GetDaysPassed();
		const auto  hoursPassed = (daysPassed - daysPassedSinceLastDecay) * 24.0f;

		float timeDelta = hoursPassed / decay.interval;

		float legendaryDamping = GetLegendaryMult();

		float mult = GetDifficultyMult() / (decay.damping * legendaryDamping);
		float rawDecayXP = CalculateLevelThresholdXP(GetDecayTargetLevel());
		float fullDecayXP = rawDecayXP * mult;

		// We calculate max XP that can be decayed, so that the decay rate won't exeed minDaysPerLevel (e.g. with minDaysPerLevel = 1, it would take at least 1 day to decay 1 level).
		float maxDecayXP = rawDecayXP * decay.minDaysPerLevel;
		// Similarly, we calculate min XP, so that the decay rate won't take ages to decay on higher levels.
		float minDecayXP = rawDecayXP * decay.maxDaysPerLevel;
		float clampedDecayXP = max(minDecayXP, min(maxDecayXP, fullDecayXP));

		float decayXP = clampedDecayXP * timeDelta;

		DecaySkill(skillData, decayXP);

		lastKnownLevel = Player->GetBaseActorValue(AV(skill));
		lastKnownXP = skillData.xp;
		daysPassedSinceLastDecay = daysPassed;
	}

	void SkillUsage::DecaySkill(SkillData& skillData, float& decayXPAmount)
	{
		if (decayXPAmount <= 0.0f)
			return;

		float level = Player->GetBaseActorValue(AV(skill));

		if (skillData.xp >= decayXPAmount) {
			skillData.xp -= decayXPAmount;
			decayXPAmount = 0.0f;
		} else if (level <= GetDecayCapLevel()) {
			// We can't decay any further, so just reset XP.
			skillData.xp = 0.0f;
			decayXPAmount = 0.0f;
		} else {
			decayXPAmount -= skillData.xp;
			const float threshold = CalculateLevelThresholdXP(static_cast<int>(level));
			skillData.xp = max(0, threshold - 1);  // -1 to be safe, so that we won't end up in invalid state where xp == levelThreshold.
			skillData.levelThreshold = threshold;
			Player->ModBaseActorValue(AV(skill), -1);
			// skillData.level is only updated after player confirms level up (in Skills Menu).
			// Before that, skillData.level will remain at the last confirmed level, even if GetBaseAV's level is further.
			if (level == skillData.level) {
				skillData.level -= 1;
			}
			DecaySkill(skillData, decayXPAmount);
		}
	}

	inline int SkillUsage::GetStartingLevel() const
	{
		return baselineLevel + raceSkillBonus;
	}

	inline int SkillUsage::GetDecayTargetLevel() const
	{
		// Level 2 is the smallest we can go to avoid Decay XP equaling zero.
		return max(2, baselineLevel + decay.baselineLevelOffset - raceSkillBonus - decay.levelOffset);
	}

	inline float SkillUsage::GetDifficultyMult() const
	{
		if (std::signbit(decay.difficultyMult)) {
			constexpr float difficultyMults[] = {
				0.5f,   // Novice
				0.75f,  // Apprentice
				1.0f,   // Adept
				1.5f,   // Expert
				2.0f,   // Master
				3.0f    // Legendary
			};
			auto diffIndex = min(Player->difficulty, 5);
			return difficultyMults[diffIndex];
		} else {
			return decay.difficultyMult;
		}
	}

	float SkillUsage::GetGracePeriod() const
	{
		if (std::signbit(decay.gracePeriod)) {
			float level = Player->GetBaseActorValue(AV(skill));
			float target = GetDecayTargetLevel();

			float ratio = target < level ? 1.0f : level / target;

			constexpr float difficultyMults[] = {
				3.0f,   // Novice
				2.0f,   // Apprentice
				1.75f,  // Adept
				1.5f,   // Expert
				1.25f,  // Master
				1.0f    // Legendary
			};

			auto diffIndex = min(Player->difficulty, 5);
			auto diffMult = difficultyMults[diffIndex];

			auto gracePeriodBase = ratio * diffMult * GetLegendaryMult();

			auto days = std::pow(max(1, gracePeriodBase), 0.75f);

			return max(1.0f, days) * 24.0f * GetLegendaryMult();
		} else {
			return decay.gracePeriod;
		}
	}

	float SkillUsage::GetLegendaryMult() const
	{
		return max(1, 1 + (decay.legendarySkillDamping - 1) * Player->skills->data->legendaryLevels[skill]);
	}

	inline int SkillUsage::GetDecayCapLevel() const
	{
		int effectiveLevelCap = decay.levelCap;

		if (effectiveLevelCap == 0) {
			constexpr int difficultyCaps[] = {
				-5,   // Novice
				-10,  // Apprentice
				-15,  // Adept
				-30,  // Expert
				-40,  // Master
				0     // Legendary
			};
			auto diffIndex = min(Player->difficulty, 5);
			effectiveLevelCap = difficultyCaps[diffIndex];
		}

		if (effectiveLevelCap > 0) {
			float level = Player->GetBaseActorValue(AV(skill));
			return level >= effectiveLevelCap ? effectiveLevelCap : GetStartingLevel();
		} else if (effectiveLevelCap < 0) {
			return max(GetStartingLevel(), lastKnownHighestLevel + effectiveLevelCap);
		} else {
			return GetStartingLevel();
		}
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
