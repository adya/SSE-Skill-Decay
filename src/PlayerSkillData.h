#pragma once

#include "BaseSkillData.h"

namespace Decay
{
	struct PlayerSkillData : BaseSkillData
	{
		constexpr static const char* DEFAULT_SKILL_NAMES[] = {
			"OneHanded", "TwoHanded", "Archery", "Block", "Smithing",
			"HeavyArmor", "LightArmor", "Pickpocket", "Lockpicking",
			"Sneak", "Alchemy", "Speech", "Alteration", "Conjuration",
			"Destruction", "Illusion", "Restoration", "Enchanting"
		};

		constexpr static bool IsDefaultSkill(std::string_view skillId) noexcept
		{
			for (const auto& name : DEFAULT_SKILL_NAMES) {
				if (name == skillId) {
					return true;
				}
			}
			return false;
		}

		Skill               skill;
		RE::ActorValueInfo* avi;

		PlayerSkillData() = delete;
		PlayerSkillData(Skill);

		std::string_view    GetName() const noexcept override;
		int                 GetLevel() const noexcept override;
		float               GetXP() const noexcept override;
		void                SetXP(float xp) noexcept override;
		void                UpdateRaceBonus() noexcept override;
		int                 GetLegendaryLevel() const noexcept override;
		void                ModLevel(int mod) noexcept override;
		RE::ActorValueInfo* GetAVInfo() const noexcept override;
	};
}
