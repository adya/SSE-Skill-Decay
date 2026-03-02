#pragma once

#include "BaseSkillData.h"

namespace Decay
{
	struct PlayerSkillData : BaseSkillData
	{
		//namespace
		//{
		//	const char* DEFAULT_SKILL_NAMES[] = {
		//		"OneHanded", "TwoHanded", "Archery", "Block", "Smithing",
		//		"HeavyArmor", "LightArmor", "Pickpocket", "Lockpicking",
		//		"Sneak", "Alchemy", "Speech", "Alteration", "Conjuration",
		//		"Destruction", "Illusion", "Restoration", "Enchanting"
		//	};
		//}

		Skill               skill;
		RE::ActorValueInfo* avi;
		int               raceBonus;

		PlayerSkillData(Skill);

		std::string_view    GetName() const noexcept override;
		int                 GetLevel() const noexcept override;
		float               GetXP() const noexcept override;
		void                SetXP(float xp) noexcept override;
		int                 GetRaceBonus() noexcept override;
		int                 GetLegendaryLevel() const noexcept override;
		void                ModLevel(int mod) noexcept override;
		RE::ActorValueInfo* GetAVInfo() const noexcept override;
	};
}
