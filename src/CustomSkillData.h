#pragma once
#include "BaseSkillData.h"
#include "Options.h"

namespace Decay
{
	struct CustomSkillData : BaseSkillData
	{
		std::string         skillId;
		RE::ActorValueInfo* avi;

		RE::TESGlobal* levelGlobal;
		RE::TESGlobal* xpGlobal;
		bool           xpNormalized;
		RE::TESGlobal* legendaryGlobal;

		std::map<RE::FormID, int> raceBonuses;

		CustomSkillData() = delete;
		CustomSkillData(std::string skillId, RE::ActorValueInfo* avi, RE::TESGlobal* level, RE::TESGlobal* xp, bool xpNormalized, RE::TESGlobal* legendary, std::map<RE::FormID, int>& raceBonuses) :
			skillId(std::move(skillId)),
			avi(avi),
			levelGlobal(level),
			xpGlobal(xp),
			xpNormalized(xpNormalized),
			legendaryGlobal(legendary),
			raceBonuses(std::move(raceBonuses)){};

		std::string_view    GetName() const noexcept override;
		int                 GetLevel() const noexcept override;
		float               GetLevelThreshold() const noexcept override;
		float               GetXP() const noexcept override;
		void                SetXP(float xp) noexcept override;
		void                UpdateRaceBonus() noexcept override;
		int                 GetLegendaryLevel() const noexcept override;
		void                ModLevel(int mod) noexcept override;
		RE::ActorValueInfo* GetAVInfo() const noexcept override;
	};
}
