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
