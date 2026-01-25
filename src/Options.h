#pragma once

namespace Decay::Settings
{
	inline float fEnchantingSkillCostBase()
	{
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fEnchantingSkillCostBase")) {
			return setting->data.f;
		}
		return 0.005f;
	}

	inline float fEnchantingSkillCostScale()
	{
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fEnchantingSkillCostScale")) {
			return setting->data.f;
		}
		return 0.5f;
	}

	inline float fEnchantingSkillCostMult()
	{
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fEnchantingSkillCostMult")) {
			return setting->data.f;
		}
		return 3.0f;
	}

	inline float fEnchantingCostExponent()
	{
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fEnchantingCostExponent")) {
			return setting->data.f;
		}
		return 1.1f;
	}
}
