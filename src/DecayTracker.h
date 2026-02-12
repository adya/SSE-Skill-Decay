#pragma once
#include "SkillUsage.h"

namespace Decay
{

	class DecayTracker : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static DecayTracker& GetInstance()
		{
			static DecayTracker instance;
			return instance;
		}
		static void Register();

		SkillUsage& operator[](Skill skill) { return skillUsages[skill]; }
		SkillUsage const& operator[](Skill skill) const { return skillUsages[skill]; }

		void AdvanceTime(RE::Calendar* calendar);
		void LoadSettings();

	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	private:

		/// Hours between SkillUsage updates.
		float      trackingRate = 0.16f;
		bool       logSkillUsage = false;
		float      lastDaysPassed;
		SkillUsage skillUsages[Skill::kTotal];

		void UpdateSkillUsage(RE::Calendar*);

		static void Load(SKSE::SerializationInterface*);
		static void Save(SKSE::SerializationInterface*);
		static void Revert(SKSE::SerializationInterface*);
	};
}
