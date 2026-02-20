#include "RE/C/Calendar.h"

namespace Decay
{
	struct DecayConfig
	{
		/// Time interval in hours before a skill is considered stale (starting to decay).
		/// Negative value represents automatic scaling of grace period based on Skill level.
		/// Specifically, how far from the starting level the Player has progressed. The further the progression, the longer grace period is granted.
		float gracePeriod = -0.0f;

		/// Time interval in hours that it takes to fully decay XP of GetDecayTargetLevel().
		float interval = 24.0f;

		/// An offset to the baseline level, that is used to calculate decay rate.
		///
		/// Ideally, this value should correspond to the max racial skill bonus among skillBoosts provided by Player's race.
		/// However, it can be customized for a skill, if needed.
		///
		/// Positive values represent custom racial bonus to the target decay level.
		/// Negative values represent automatic value based on Player's racial skill bonuses.
		/// 0 would mean that skill doesn't use racial skill bonus.
		int baselineLevelOffset = -1;

		/// Additional offset to the target decay level. Larger offset means slower decay.
		///
		/// This value should be less than starting level. Otherwise, the decay level would be clamped to 2.
		int levelOffset = 0;

		/// Multiplier to the XP decay rate. Larger multiplier means faster decay.
		/// Negative values represent automatic scaling based on the game's difficulty setting.
		/// 0 disables decay for the skill.
		float difficultyMult = -0.0f;

		/// If non-negative, this difficulty will be used instead of Player's actual difficulty for calculating decay rate.
		int difficultyOverride = -1;

		/// Damping multiplier to the XP decay rate. Larger multiplier means slower decay.
		/// Must be positive. Applied as a 1/damping.
		float damping = 1.0f;

		/// Additional damping to slow down decay with each legendary level in this skill.
		/// This damping also applies to the grace period, so it will take longer for the skill to start decaying with each legendary level.
		///
		/// For each additional legendary level, fraction of this damping value is added.
		/// e.g. with default 1.15f, each legendary level adds 0.15f to the damping.
		float legendarySkillDamping = 1.15f;

		/// The lower cap for decay.
		/// Skill cannot decay below the starting level.
		///
		/// Positive values represent absolute minimum level that Skill can decay to.
		/// Negative values represent a relative offset from the highest level achieved in this skill
		/// 0 representes automatic scaling of level cap based on the difficulty.
		int levelCap = 0;

		/// The smallest number of days it should take to decay a skill by 1 level.
		/// This value is used to clamp maximum allowed decay XP, to prevent too rapid decays on smaller levels.
		float minDaysPerLevel = 1.0f;

		/// The largest number of days it should take to decay a skill by 1 level.
		/// This value is used to clamp minimum allowed decay XP, to prevent too slow decays on higher levels.
		float maxDaysPerLevel = 14.0f;

		/// Paths to the skill level meter UI elements for each skill, used for applying color tint when decaying.
		std::vector<std::string> uiLayers;

		/// Color of the tint to be applied to the skill level meter UI elements when the skill is decaying.
		RE::GColor decayTint = { 255, 60, 0, 200 };

		/// Color of the tint to be applied to the skill level meter UI elements when the skill is not decaying.
		/// Provided just for fun if some people would like to recolor their skill meters.
		RE::GColor normalTint = { 0, 0, 0, 0 };

		DecayConfig() = default;
		DecayConfig(std::vector<std::string> layers) :
			uiLayers(std::move(layers))
		{}
		DecayConfig(float damping, std::vector<std::string> layers) :
			damping(damping),
			uiLayers(std::move(layers))
		{}
	};

	struct SkillUsage
	{
		void Init(Skill skill, DecayConfig& config);
		void Revert();

		/// Checks whether this SkillUsage has received at least one SetUsed() call.
		bool IsInitialized() const;

		bool WasUsed() const;
		void SetUsed(const RE::Calendar* calendar);

		bool IsStale(const RE::Calendar* calendar) const;

		void MarkDecaying(const RE::Calendar* calendar);
		bool IsDecaying() const;
		void Decay(const RE::Calendar*);

		int GetDecayCapLevel() const;

		const DecayConfig& GetConfig() const { return decay; }

	private:
		Skill skill = Skill::kTotal;  // unless loaded properly, this SkillUsage is invalid and should not be used.

		/// Days Passed when the skill was last used.
		float daysPassedWhenLastUsed = 0;

		int   lastKnownLevel = -1;
		float lastKnownXP = -1;

		int lastKnownLegendaryLevel = -1;

		/// Highest level achieved in this skill, used for relative decay cap when decayCap is negative.
		int lastKnownHighestLevel = -1;

		bool  isDecaying = false;
		float daysPassedSinceLastDecay = 0;

		/// Starting level of the skill.
		int baselineLevel = 15;

		/// Bonus that Player's race provides to the skill.
		/// Together with baselineLevel is used to calculate XP decay rate for the skill.
		/// Also, used to prevent decaying below (baselineLevel + raceSkillBonus).
		int raceSkillBonus = 0;

		DecayConfig decay;

		/// Subtracts decayXPAmount recursively, decreasing skill level as needed.
		void DecaySkill(SkillData& skillData, float& decayXPAmount);

		int   GetStartingLevel() const;
		int   GetDecayTargetLevel() const;
		float GetDifficultyMult() const;

		float GetGracePeriod() const;

		float GetLegendaryMult() const;

		int GetDifficulty() const;

		float CalculateLevelThresholdXP(int level) const;

		friend bool Write(SKSE::SerializationInterface*, const SkillUsage&);
		friend bool Read(SKSE::SerializationInterface*, SkillUsage&);
	};
}
