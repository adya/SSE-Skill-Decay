#include "Options.h"
#include "CLIBUtil/simpleINI.hpp"
#include "SkillUsage.h"

namespace Decay::Options
{
	/*void LoadSkill(SkillDecay& config, const CSimpleIniA& ini, const char* section) 
	{
		if (ini.SectionExists(section)) {
			logger::info("Loading options for skill decay from section [{}]", section);
		} else {
			config.gracePeriod = ini.GetDoubleValue(section, "fGracePeriod", config.gracePeriod);
			config.decayInterval = ini.GetDoubleValue(section, "fDecayInterval", config.decayInterval);
			config.decayLevelOffset = ini.GetLongValue(section, "iDecayLevelOffset", config.decayLevelOffset);
			config.maxRaceBonus = ini.GetLongValue(section, "iMaxRaceBonus", config.maxRaceBonus);
			config.decayXPDamping = ini.GetDoubleValue(section, "fDecayXPDamping", config.decayXPDamping);
			config.decayXPDifficultyMult = ini.GetDoubleValue(section, "fDecayXPDifficultyMult", config.decayXPDifficultyMult);
			config.decayCap = ini.GetLongValue(section, "iDecayCap", config.decayCap);
			return;
		})
	}*/

	void Load()
	{
		logger::info("{:*^40}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\SkillDecay.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		ini.SetMultiKey(false);
		if (ini.LoadFile(options.string().c_str()) >= 0) {
		} else {
			logger::info(R"(Data\SKSE\Plugins\SkillDecay.ini not found. Default options will be used.)");
			logger::info("");
		}
	}
}
