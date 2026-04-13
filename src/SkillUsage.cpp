#include "SkillUsage.h"
#include "Options.h"
#include "RE/A/ActorValueList.h"
#include "RE/P/PlayerCharacter.h"
#include <algorithm>
#include <cassert>

namespace Decay
{
	void SkillUsage::Init(BaseSkillData* skill, const DecayConfig& config)
	{
		assert(skill);
		this->skill = skill;
		this->decay = config;
	}

	void SkillUsage::Init(BaseSkillData* skill, DecayConfig& config)
	{
		Init(skill, std::move(config));
	}

	void SkillUsage::Revert()
	{
		daysPassedWhenLastUsed = 0;
		lastKnownLevel = -1;
		lastKnownXP = -1;
		lastKnownHighestLevel = -1;
		lastKnownLegendaryLevel = 0;
		isDecaying = false;
		daysPassedSinceLastDecay = 0;
	}

	bool SkillUsage::IsInitialized() const
	{
		return lastKnownLevel >= 0 && lastKnownXP >= 0;
	}

	bool SkillUsage::WasUsed() const
	{
		assert(skill);

		auto level = skill->GetLevel();
		auto xp = skill->GetXP();
		return level > lastKnownLevel || ((xp - lastKnownXP) > 0.5f);  // 0.5f to make sure that we only count proper XP gains (at least +1)
	}

	void SkillUsage::SetUsed(const RE::Calendar* calendar)
	{
		assert(skill);
		lastKnownLevel = skill->GetLevel();
		lastKnownXP = skill->GetXP();
		int legLevel = skill->GetLegendaryLevel();
		if (legLevel > lastKnownLegendaryLevel) {
			lastKnownHighestLevel = GetStartingLevel();
		} else {
			lastKnownHighestLevel = max(lastKnownHighestLevel, lastKnownLevel);
		}
		lastKnownLegendaryLevel = legLevel;

		daysPassedWhenLastUsed = calendar->GetDaysPassed();
		isDecaying = false;
	}

	void SkillUsage::ResetDecay()
	{
		daysPassedWhenLastUsed = RE::Calendar::GetSingleton()->GetDaysPassed();
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
		if (!isDecaying) {
			return false;
		}

		assert(skill);

		auto cap = GetDecayCapLevel();
		auto level = skill->GetLevel();

		// If we're above the cap level, we should be decaying. If we're at the cap level, we can still decay if there's some XP to decay.
		return level == cap ? skill->GetXP() > 0 : level > cap;
	}

	void SkillUsage::Decay(const RE::Calendar* calendar)
	{
		assert(isDecaying);
		assert(skill);

		const float daysPassed = calendar->GetDaysPassed();
		const auto  hoursPassed = (daysPassed - daysPassedSinceLastDecay) * 24.0f;

		float timeDelta = hoursPassed / decay.interval;

		float legendaryDamping = GetLegendaryMult();

		float mult = GetDifficultyMult() / (decay.damping * legendaryDamping);
		float rawDecayXP = skill->CalculateLevelThresholdXP(GetDecayTargetLevel());
		float fullDecayXP = rawDecayXP * mult;

		// We calculate max XP that can be decayed, so that the decay rate won't exeed minDaysPerLevel (e.g. with minDaysPerLevel = 1, it would take at least 1 day to decay 1 level).
		float maxDecayXP = rawDecayXP * decay.minDaysPerLevel;
		// Similarly, we calculate min XP, so that the decay rate won't take ages to decay on higher levels.
		float minDecayXP = rawDecayXP * decay.maxDaysPerLevel;
		float clampedDecayXP = max(minDecayXP, min(maxDecayXP, fullDecayXP));

		float decayXP = clampedDecayXP * timeDelta;

		DecaySkill(decayXP, true);  // standard decay always affects levels, the levelCap in config will control the actual limit.

		lastKnownLevel = skill->GetLevel();
		lastKnownXP = skill->GetXP();
		daysPassedSinceLastDecay = daysPassed;
	}

	void SkillUsage::DecaySkill(float& decayXPAmount, bool decayLevels)
	{
		if (decayXPAmount <= 0.0f)
			return;

		if (!skill) {
			decayXPAmount = 0.0f;
			return;
		}

		float level = skill->GetLevel();
		float xp = skill->GetXP();

		if (xp >= decayXPAmount) {
			skill->SetXP(xp - decayXPAmount);
			decayXPAmount = 0.0f;
		} else if (!decayLevels || level <= GetDecayCapLevel()) {
			// We can't decay any further, so just reset XP.
			skill->SetXP(0.0f);
			decayXPAmount = 0.0f;
		} else {
			decayXPAmount -= xp;
			const float threshold = skill->CalculateLevelThresholdXP(level);
			skill->SetXP(max(0, threshold - 1));  // -1 to be safe, so that we won't end up in invalid state where xp == levelThreshold.
			skill->ModLevel(-1);

			DecaySkill(decayXPAmount, decayLevels);
		}
	}

	inline int SkillUsage::GetStartingLevel() const
	{
		assert(skill);
		return skill->GetBaselineLevel() + skill->GetRaceBonus();
	}

	inline int SkillUsage::GetDecayTargetLevel() const
	{
		assert(skill);
		// Level 2 is the smallest we can go to avoid Decay XP equaling zero.
		return max(2, skill->GetBaselineLevel() + decay.baselineLevelOffset - skill->GetRaceBonus() - decay.levelOffset);
	}

	inline float SkillUsage::GetDifficultyMult() const
	{
		if (std::signbit(decay.difficultyMult)) {
			constexpr float difficultyMults[] = {
				1.0f,   // Novice
				1.25f,  // Apprentice
				1.5f,   // Adept
				1.75f,  // Expert
				2.0f,   // Master
				3.0f    // Legendary
			};
			return difficultyMults[GetDifficulty()];
		} else {
			return decay.difficultyMult;
		}
	}

	float SkillUsage::GetGracePeriod() const
	{
		if (std::signbit(decay.gracePeriod)) {
			assert(skill);
			float level = skill->GetLevel();
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

			auto diffMult = difficultyMults[GetDifficulty()];

			auto gracePeriodBase = ratio * diffMult * GetLegendaryMult();

			auto days = std::pow(max(1, gracePeriodBase), 0.75f);

			return max(1.0f, days) * 24.0f * GetLegendaryMult();
		} else {
			return decay.gracePeriod;
		}
	}

	float SkillUsage::GetLegendaryMult() const
	{
		assert(skill);
		return max(1, 1 + (decay.legendarySkillDamping - 1) * skill->GetLegendaryLevel());
	}

	int SkillUsage::GetDifficulty() const
	{
		if (decay.difficultyOverride >= 0) {
			return decay.difficultyOverride;
		} else {
			return Player->difficulty;
		}
	}

	inline int SkillUsage::GetDecayCapLevel() const
	{
		int effectiveLevelCap = decay.levelCap;

		if (decay.levelCap == INT_MAX) {
			constexpr int difficultyCaps[] = {
				-5,   // Novice
				-10,  // Apprentice
				-15,  // Adept
				-30,  // Expert
				-40,  // Master
				-500  // Legendary
			};
			effectiveLevelCap = difficultyCaps[GetDifficulty()];
		}

		if (effectiveLevelCap > 0) {
			assert(skill);
			float level = skill->GetLevel();
			return level >= effectiveLevelCap ? effectiveLevelCap : GetStartingLevel();
		} else if (effectiveLevelCap <= 0) {
			return max(GetStartingLevel(), lastKnownHighestLevel + effectiveLevelCap);
		} else {
			return GetStartingLevel();
		}
	}
	void SkillUsage::UpdateBaselineLevel()
	{
		if (!Player || !Player->GetRace()) {
			return;
		}

		for (const auto& boost : Player->GetRace()->data.skillBoosts) {
			if (decay.baselineLevelOffset < 0 && boost.bonus > decay.baselineLevelOffset) {
				decay.baselineLevelOffset = boost.bonus;
			}
		}
	}
}
