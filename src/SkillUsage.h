#include "RE/C/Calendar.h"
#include "RE/P/PlayerCharacter.h"

namespace Decay
{
	struct SkillUsage
	{
		void Init(Skill skill);
		void Revert();

		/// Checks whether this SkillUsage has received at least one SetUsed() call.
		bool IsInitialized() const;

		bool WasUsed() const;
		void SetUsed(const RE::Calendar* calendar);

		bool IsStale(const RE::Calendar* calendar) const;

		void MarkDecaying(const RE::Calendar* calendar);
		bool IsDecaying() const { return isDecaying; }
		void Decay( const RE::Calendar*);

		int GetStartingLevel() const { return baselineLevel + raceSkillBonus; }

	private:
		Skill skill = Skill::kTotal;  // unless loaded properly, this SkillUsage is invalid and should not be used.

		// Days Passed when it was last used
		float daysPassedWhenLastUsed = 0;
		int   lastKnownLevel = -1;
		float lastKnownXP = -1;

		bool  isDecaying = false;
		float daysPassedSinceLastDecay = 0;

		/// Time interval in hours before a skill is considered stale (starting to decay).
		float decayGracePeriod = 24.0f;

		/// Time interval in hours that it takes to fully decay XP of GetDecayTargetLevel().
		float decayInterval = 24.0f;

		/// Starting level of the skill.
		int baselineLevel = 15;

		/// A value that determines upper limit for target decay level.
		/// By default, this value should be max race skill bonus among skillBoosts provided by race.
		int baselineLevelBonus = 10;

		/// Bonus that Player's race provides to the skill.
		/// Together with baselineLevel is used to calculate XP decay rate for the skill.
		/// Also, used to prevent decaying below (baselineLevel + raceSkillBonus).
		int raceSkillBonus = 0;

		/// Additional offset to the target decay level. Larger offset means slower decay.
		int decayLevelOffset = 0;

		/// Multiplier to the XP decay rate.
		float decayXPMult = 1.0f;

		/// Subtracts decayXPAmount recursively, decreasing skill level as needed.
		void DecaySkill(RE::ActorValue, SkillData& skillData, float& decayXPAmount);

		int GetDecayTargetLevel() const;

		float CalculateLevelThresholdXP(int level) const;


		friend bool Write(SKSE::SerializationInterface*, const SkillUsage&);
		friend bool Read(SKSE::SerializationInterface*, SkillUsage&);
	};

}
