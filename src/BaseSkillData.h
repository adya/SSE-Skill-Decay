#pragma once
#include "Options.h"

namespace Decay
{
	struct BaseSkillData
	{
		/// Gets name of the associated Skill.
		virtual std::string_view GetName() const noexcept = 0;

		/// Gets level of the associated Skill.
		virtual int GetLevel() const noexcept = 0;

		/// Gets XP of the associated Skill.
		virtual float GetXP() const noexcept = 0;

		/// Sets XP of the associated Skill.
		virtual void SetXP(float xp) noexcept = 0;

		/// Starting level of the skill.
		inline int GetBaselineLevel() const noexcept { return Settings::iAVDSkillStart(); }

		/// Bonus that Player's race provides to the skill.
		/// Together with baselineLevel is used to calculate XP decay rate for the skill.
		/// Also, used to prevent decaying below (baselineLevel + raceSkillBonus).
		int GetRaceBonus() noexcept { return raceBonus; }

		/// Updates race bonus for the skill.
		/// Should be called after Player's race is determined:
		/// - When a game is loaded
		/// - After RaceMenu closes, in case Race has changed.
		virtual void UpdateRaceBonus() noexcept = 0;

		virtual int GetLegendaryLevel() const noexcept = 0;

		/// Modifies level of the associated Skill by the specified amount. Positive mod increases level, negative mod decreases level.
		virtual void ModLevel(int mod) noexcept = 0;

		/// Gets ActorValueInfo of the associated Skill.
		virtual RE::ActorValueInfo* GetAVInfo() const noexcept = 0;

		inline float CalculateLevelThresholdXP(int level) const
		{
			if (const auto avi = GetAVInfo(); avi) {
				const auto mult = avi->skill->improveMult;
				const auto offset = avi->skill->improveOffset;
				const auto curve = Settings::fSkillUseCurve();

				return mult * std::pow(level - 1.0f, curve) + offset;
			}

			return 0.0f;
		}

		protected:
			int raceBonus = 0;
	};
}
