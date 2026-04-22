#pragma once
#include "CustomSkillData.h"
#include "PlayerSkillData.h"
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

		SkillUsage&       operator[](Skill skill) { return skillUsages[skill]; }
		SkillUsage const& operator[](Skill skill) const { return skillUsages[skill]; }

		void AdvanceTime(RE::Calendar* calendar);
		void LoadSettings();
		void LoadDefaultSkills();

		void ApplyTint(RE::GFxMovieView*, const std::vector<RE::ActorValue>& skillGroup) const;

		void RegisterCustomSkill(
			const std::string&         skillId,
			RE::ActorValue             av,
			RE::ActorValueInfo*        avi,
			RE::TESGlobal*             levelGlobal,
			RE::TESGlobal*             xpGlobal,
			bool                       xpNormalized,
			RE::TESGlobal*             legendaryGlobal,
			std::map<RE::FormID, int>& raceBonuses) noexcept;

		void        UnregisterCustomSkill(const std::string& skillId) noexcept;
		bool        IsCustomSkillRegistered(const std::string& skillId) const noexcept;
		SkillUsage* GetCustomSkillUsage(const std::string& skillId) noexcept;

		bool LogStatsMenuTree() const noexcept { return logStatsMenuTree; }

	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	private:
		/// Hours between SkillUsage updates.
		float trackingRate = 0.016f;  // once every in-game minute by default
		bool  logSkillUsage = false;
		bool  logStatsMenuTree = false;
		float lastDaysPassed = 0;
		bool  initialized = false;

		DecayConfig      skillConfigs[Skill::kTotal];
		SkillUsage       skillUsages[Skill::kTotal];
		PlayerSkillData* defaultSkills[Skill::kTotal];

		std::map<std::string, DecayConfig>      customSkillConfigs{};
		std::map<std::string, SkillUsage>       customSkillUsages{};
		std::map<std::string, CustomSkillData*> customSkills{};

		void UpdateSkillUsage(RE::Calendar*);

		static void Load(SKSE::SerializationInterface*);
		static void Save(SKSE::SerializationInterface*);
		static void Revert(SKSE::SerializationInterface*);
	};
}
