#pragma once
#include "SkillUsage.h"

namespace Decay
{

	class DecayTracker
	{
	public:
		static DecayTracker& GetInstance()
		{
			static DecayTracker instance;
			return instance;
		}

		void AdvanceTime(RE::Calendar* calendar);
		void LoadSettings();

	private:

		float lastDaysPassed;
		SkillUsage skillUsages[Skill::kTotal];

		void UpdateSkillUsage(RE::Calendar*);
	};
}
