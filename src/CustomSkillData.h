#pragma once
#include "Options.h"
#include "../include/SkillDecay_API.h"
#include "BaseSkillData.h"

namespace Decay
{
	struct CustomSkillData : BaseSkillData
	{
		std::string skillId;
		float improveMult;
		float improveOffset;
		
		RE::TESGlobal* levelGlobal;
		RE::TESGlobal* xpGlobal;
		bool xpNormalized;
		RE::TESGlobal* legendaryGlobal;

		RE::ActorValueInfo* avi;
		
		std::map<RE::FormID, int> raceBonuses;
		
		SkillDecay::CustomSkillDecayCallback callback;
		void* userData;
		
		std::string_view    GetName() const noexcept override;
		int                 GetLevel() const noexcept override;
		float               GetXP() const noexcept override;
		void                SetXP(float xp) noexcept override;
		int               GetRaceBonus() noexcept override;
		int                 GetLegendaryLevel() const noexcept override;
		void                ModLevel(int mod) noexcept override;
		RE::ActorValueInfo* GetAVInfo() const noexcept override;
	};
}
