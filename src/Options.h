#pragma once

namespace Decay::Settings
{
	inline int iAVDSkillStart()
	{
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("iAVDSkillStart")) {
			return setting->data.i;
		}
		return 15;
	}

	inline float fSkillUseCurve()
	{
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fSkillUseCurve")) {
			return setting->data.f;
		}
		return 1.95f;
	}
}

namespace Decay::Options
{
	struct SkillDecay
	{
		float gracePeriod = 24.0f;
		float decayInterval = 24.0f;

		int decayLevelOffset = 0;
		int maxRaceBonus = 10;

		/// Damping multiplier to the XP decay rate. Larger multiplier means slower decay.
		/// Must be positive. Applied as a 1/decayXPDamping.
		float decayXPDamping = 1.0f;

		/// Multiplier to the XP decay rate. Larger multiplier means faster decay.
		/// Negative values represent the automatic scaling based on the game's difficulty setting. 
		/// 0 disables decay for the skill.
		float decayXPDifficultyMult = -0.0f;

		/// The lower cap for decay.
		/// Positive values represent absolute minimum level that Skill can decay to.
		/// Negative values represent a relative offset from the highest level achieved in this skill
		/// 0 means no cap, the skill will decay to the baseline level with which the player started.
		int decayCap = 0;
	};

	void Load();
}
