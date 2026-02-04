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

namespace Decay::Settings
{

	inline float fDifficultyMult()
	{
		// TODO: Replace with actual configurable value if needed
		// Should support "auto" where the value is taken from the game's difficulty setting
		return 1.0f;
	}

	inline float fDecayTrackerRate() 
	{
		// TODO: Replace with actual configurable value if needed
		return 1.0f;
	}

	/// Returns the number of in-game hours before skill decay starts
	inline float fDecayGracePeriodHours(Skill skill)
	{
		// TODO: Replace with actual configurable value if needed
		return 24.0f;
	}

	/// Returns the number of in-game hours it takes to fully apply single decay step.
	inline float fDecayIntervalHours(Skill skill)
	{
		// TODO: Replace with actual configurable value if needed
		return 24.0f;
	}

	inline int iDecayLevelOffset(Skill skill)
	{
		// TODO: Replace with actual configurable value if needed
		return 0;
	}

	inline int iMaxRaceBonus()
	{
		// TODO: Replace with actual configurable value if needed
		return 10;
	}

	inline float fDecayXPMult(Skill skill)
	{
		// TODO: Replace with actual configurable value if needed
		return 1.0f;
	}
}
