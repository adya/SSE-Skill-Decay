#include "DecayTracker.h"
#include "CLIBUtil/simpleINI.hpp"
#include "CLIBUtil/string.hpp"
#include "Options.h"

#define Inc(skill) \
	skill = static_cast<Skill>(static_cast<std::underlying_type_t<Skill>>(skill) + 1)

namespace Decay
{
	void DecayTracker::AdvanceTime(RE::Calendar* calendar)
	{
		float daysPassed = calendar->GetDaysPassed();
		float hoursPassed = (daysPassed - lastDaysPassed) * 24.0;

		if (hoursPassed > trackingRate) {
			lastDaysPassed = daysPassed;
			UpdateSkillUsage(calendar);
		}
	}

	void ReadSettings(const CSimpleIniA& ini, const char* section, DecayConfig& config)
	{
		if (ini.SectionExists(section)) {
			config.gracePeriod = ini.GetDoubleValue(section, "fDecayGracePeriod", config.gracePeriod);
			config.interval = ini.GetDoubleValue(section, "fDecayInterval", config.interval);
			config.levelOffset = ini.GetLongValue(section, "iDecayLevelOffset", config.levelOffset);
			config.baselineLevelOffset = ini.GetLongValue(section, "iBaselineLevelOffset", config.baselineLevelOffset);
			config.damping = ini.GetDoubleValue(section, "fDecayXPDamping", config.damping);
			config.difficultyMult = ini.GetDoubleValue(section, "fDecayXPDifficultyMult", config.difficultyMult);
			config.levelCap = ini.GetLongValue(section, "iDecayLevelCap", config.levelCap);
			config.legendarySkillDamping = ini.GetDoubleValue(section, "fLegendarySkillXPDamping", config.legendarySkillDamping);
			config.minDaysPerLevel = ini.GetDoubleValue(section, "fMinDaysPerLevel", config.minDaysPerLevel);
			config.maxDaysPerLevel = ini.GetDoubleValue(section, "fMaxDaysPerLevel", config.maxDaysPerLevel);

			std::string color = ini.GetValue(section, "cDecayTint", "");

			if (!color.empty()) {
				config.decayTint = clib_util::string::to_color(color, config.decayTint);
			}

			color = ini.GetValue(section, "cTint", "");

			if (!color.empty()) {
				config.normalTint = clib_util::string::to_color(color, config.normalTint);
			}

			std::string rawLayers = ini.GetValue(section, "sUILayers", "");
			auto        layers = clib_util::string::split(rawLayers, ",");
			for (auto& layer : layers) {
				clib_util::string::trim(layer);
			}

			if (!layers.empty()) {
				config.uiLayers = std::move(layers);
			}
		}
	}

	void DecayTracker::LoadSettings()
	{
		logger::info("{:*^30}", " OPTIONS ");
		std::filesystem::path options = R"(Data\SKSE\Plugins\SkillDecay.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		ini.SetMultiKey(false);

		// These are valid indices for instances present in each SkillText's ShortBar.
		// SkillText0: 94-97 // Enchanting
		// SkillText1: 100-103 // Smithing
		// SkillText2: 106-109 // Heavy Armor
		// SkillText3: 112-115 // Block
		// SkillText4: 118-121 // Two-Handed
		// SkillText5: 124-127 // One-Handed
		// SkillText6: 130-133 // Archery
		// SkillText7: 136-139 // Light Armor
		// SkillText8: 142-145 // Sneaking
		// SkillText9: 148-151 // Lockpicking
		// SkillText10: 154-157 // Pickpocket
		// SkillText11: 160-163 // Speech
		// SkillText12: 166-169 // Alchemy
		// SkillText13: 172-175 // Illusion
		// SkillText14: 178-181 // Conjuration
		// SkillText15: 184-187 // Destruction
		// SkillText16: 190-193 // Restoration
		// SkillText17: 196-199 // Alteration

		// By default we target the primary color of the bar as well as the background. Other instances control "reflection" and "shadow" effects applied to the bar.
		DecayConfig configs[Skill::kTotal] = {
			/* One-Handed */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText5.ShortBar.instance124", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText5.ShortBar.instance126" }),
			/* Two-Handed */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText4.ShortBar.instance118", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText4.ShortBar.instance120" }),
			/* Archery */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText6.ShortBar.instance130", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText6.ShortBar.instance132" }),
			/* Block */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText3.ShortBar.instance112", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText3.ShortBar.instance114" }),
			/* Smithing */ DecayConfig(2, { "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText1.ShortBar.instance100", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText1.ShortBar.instance102" }),
			/* Heavy Armor */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText2.ShortBar.instance106", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText2.ShortBar.instance108" }),
			/* Light Armor */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText7.ShortBar.instance136", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText7.ShortBar.instance138" }),
			/* Pickpocket */ DecayConfig(2, { "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText10.ShortBar.instance154", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText10.ShortBar.instance156" }),
			/* Lockpicking */ DecayConfig(2, { "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText9.ShortBar.instance148", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText9.ShortBar.instance150" }),
			/* Sneaking */ DecayConfig(1.5f, { "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText8.ShortBar.instance142", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText8.ShortBar.instance144" }),
			/* Alchemy */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText12.ShortBar.instance166", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText12.ShortBar.instance168" }),
			/* Speech */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText11.ShortBar.instance160", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText11.ShortBar.instance162" }),
			/* Alteration */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText17.ShortBar.instance196", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText17.ShortBar.instance198" }),
			/* Conjuration */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText14.ShortBar.instance178", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText14.ShortBar.instance180" }),
			/* Destruction */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText15.ShortBar.instance184", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText15.ShortBar.instance186" }),
			/* Illusion */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText13.ShortBar.instance172", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText13.ShortBar.instance174" }),
			/* Restoration */ DecayConfig({ "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText16.ShortBar.instance190", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText16.ShortBar.instance192" }),
			/* Enchanting */ DecayConfig(1.25f, { "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText0.ShortBar.instance94", "_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText0.ShortBar.instance96" })
		};

		const std::string sections[Skill::kTotal] = {
			"OneHanded",
			"TwoHanded",
			"Archery",
			"Block",
			"Smithing",
			"HeavyArmor",
			"LightArmor",
			"Pickpocket",
			"Lockpicking",
			"Sneaking",
			"Alchemy",
			"Speech",
			"Alteration",
			"Conjuration",
			"Destruction",
			"Illusion",
			"Restoration",
			"Enchanting"
		};

		if (ini.LoadFile(options.string().c_str()) >= 0) {
			float defaultTrackingRate = trackingRate;
			trackingRate = ini.GetDoubleValue("", "fTrackingRate", trackingRate);
			logSkillUsage = ini.GetBoolValue("", "bLogSkillUsage", logSkillUsage);
			if (trackingRate <= 0) {
				trackingRate = defaultTrackingRate;
			}

			for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
				DecayConfig& config = configs[skill];
				DecayConfig  defaults = config;
				const char*  section = sections[skill].c_str();

				// We load settings in 3 passes:
				// 1) Load default values for all the skills
				// 2) Load skill-specific custom values
				// 3) Load forced global values that all skills will use.

				// Load global overwrites for all skills first.
				ReadSettings(ini, "", config);

				// Then apply skill-specific settings, if they exist.
				ReadSettings(ini, section, config);

				// Finally, apply another global overwrites that are supposed to affect all skills.
				ReadSettings(ini, "All", config);

				// Lastly we want to validate input
				if (config.gracePeriod < 0) {
					config.gracePeriod = defaults.gracePeriod;
				}

				if (config.interval <= 0) {
					config.interval = defaults.interval;
				}

				if (config.damping <= 0) {
					config.damping = defaults.damping;
				}

				if (config.legendarySkillDamping < 1) {
					config.legendarySkillDamping = defaults.legendarySkillDamping;
				}

				if (config.minDaysPerLevel < 0) {
					config.minDaysPerLevel = defaults.minDaysPerLevel;
				}

				if (config.maxDaysPerLevel < 0) {
					config.maxDaysPerLevel = defaults.maxDaysPerLevel;
				} else if (config.maxDaysPerLevel < config.minDaysPerLevel) {
					config.maxDaysPerLevel = config.minDaysPerLevel + config.maxDaysPerLevel;
				}
			}
		} else {
			logger::info(R"(Data\SKSE\Plugins\SkillDecay.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("{}", logSkillUsage ? "Logging Skill Usage enabled" : "Logging Skill Usage disabled");
		auto formattedRate = trackingRate < 1.0f ? std::format("{:.2f} in-game minutes", trackingRate * 60.0f) : std::format("{:.2f} in-game hours", trackingRate);
		logger::info("Tracking Rate: once every {}", formattedRate);

		logger::info("{:>11} | {:^12} | {:^14} | {:^15} | {:^12} | {:^15} | {:^7} | {:^17} | {:^9} | {:^14} | {:^14}",
			"Skill", "Grace Period", "Decay Duration", "Baseline Offset", "Extra Offset", "Difficulty Mult", "Damping", "Legendary Damping", "Decay Cap", "Min Decay Days", "Max Decay Days");
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			skillUsages[skill].Init(skill, configs[skill]);
			const auto name = SkillName(skill);
			logger::info("{:>11} | {:^12} | {:^14} | {:^15} | {:^12} | {:^15} | {:^7} | {:^17} | {:^9} | {:^14} | {:^14}",
				name,
				std::format("{:.1f}h", configs[skill].gracePeriod),
				std::format("{:.1f}h", configs[skill].interval),
				configs[skill].baselineLevelOffset < 0 ? "Auto" : std::format("{}", configs[skill].baselineLevelOffset),
				configs[skill].levelOffset,
				std::signbit(configs[skill].difficultyMult) ? "Auto" : std::format("{:.2f}", configs[skill].difficultyMult),
				std::format("/{:.2f}", configs[skill].damping),
				std::format("+{:.0f}%", (configs[skill].legendarySkillDamping - 1) * 100.0f),
				configs[skill].levelCap == 0 ? "Base" : std::format("{}", configs[skill].levelCap),
				std::format("{:.1f}d", configs[skill].minDaysPerLevel),
				std::format("{:.1f}d", configs[skill].maxDaysPerLevel));
		}
	}

	void DecayTracker::ApplyTint(RE::GFxMovieView* movie) const
	{
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			const auto& usage = skillUsages[skill];
			const auto& config = usage.GetConfig();
			if (usage.IsDecaying()) {
				//auto r = config.decayTint.colorData.channels.red;
				//auto g = config.decayTint.colorData.channels.green;
				//auto b = config.decayTint.colorData.channels.blue;
				//auto a = config.decayTint.colorData.channels.alpha;
				//logger::info("Applying tint to {}:", SkillName(skill));
				//logger::info("    RGBA: ({}, {}, {}, {})", r, g, b, a);

				for (const auto& path : config.uiLayers) {
					movie->SetColorTint(path.c_str(), config.decayTint);
					//if (movie->SetColorTint(path.c_str(), config.decayTint)) {
					//	logger::info("    Layer: {}", path);
					//} else {
					//	logger::warn("    Failed to apply tint to layer: {}", path);
					//}
				}
			} else if (config.normalTint.colorData.channels.alpha > 0) {
				for (const auto& path : config.uiLayers) {
					movie->SetColorTint(path.c_str(), config.normalTint);
				}
			}
		}
	}

	RE::BSEventNotifyControl DecayTracker::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		// We need to reload settings, specifically, Racial Skill Bonuses after RaceMenu is closed, since player might've changed race.
		if (event->menuName == RE::RaceSexMenu::MENU_NAME && !event->opening) {
			LoadSettings();
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void DecayTracker::UpdateSkillUsage(RE::Calendar* calendar)
	{
		const std::string timestamp = std::format("{} {:.0f}:{}", calendar->GetDayName(), calendar->GetHour(), calendar->GetMinutes());

		if (logSkillUsage) {
			logger::info("{:*^65}", " Skill Data ");
			logger::info("[{:^13}] {} | {:^11} | {:^11} | {:^9} | {:^8}", timestamp, "D", "Skill", "Level [Cap]", "Threshold", "XP");
		}

		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			auto&       usage = skillUsages[skill];
			const auto& skillData = Player->skills->data->skills[skill];

			std::string decayStatus = "-";

			if (!usage.IsInitialized() || usage.WasUsed()) {
				usage.SetUsed(calendar);
				decayStatus = "↑";
			} else if (usage.IsDecaying()) {
				usage.Decay(calendar);
				decayStatus = "↓";
			} else if (usage.IsStale(calendar)) {
				usage.MarkDecaying(calendar);
				decayStatus = "-";
			}
			if (logSkillUsage) {
				std::string levelInfo = std::format("{:^3.0f}[{:^2}]", Player->GetBaseActorValue(AV(skill)), usage.GetDecayCapLevel());
				logger::info("[{:^13}] {} | {:^11} | {:^11} | {:^9.2f} | {:^8.2f}", timestamp, decayStatus, SkillName(skill), levelInfo, skillData.levelThreshold, skillData.xp);
			}
		}
		if (logSkillUsage) {
			logger::info("");
		}
	}
}

// Serialization
namespace Decay
{

	namespace details
	{
		template <typename T>
		bool Write(SKSE::SerializationInterface* a_interface, const T& data)
		{
			return a_interface->WriteRecordData(&data, sizeof(T));
		}

		template <>
		bool Write(SKSE::SerializationInterface* a_interface, const std::string& data)
		{
			const std::size_t size = data.length();
			return a_interface->WriteRecordData(size) && a_interface->WriteRecordData(data.data(), static_cast<std::uint32_t>(size));
		}

		template <typename T>
		bool Read(SKSE::SerializationInterface* a_interface, T& result)
		{
			return a_interface->ReadRecordData(&result, sizeof(T));
		}

		template <>
		bool Read(SKSE::SerializationInterface* a_interface, std::string& result)
		{
			std::size_t size = 0;
			if (!a_interface->ReadRecordData(size)) {
				return false;
			}
			if (size > 0) {
				result.resize(size);
				if (!a_interface->ReadRecordData(result.data(), static_cast<std::uint32_t>(size))) {
					return false;
				}
			} else {
				result = "";
			}
			return true;
		}
	}

	constexpr std::uint32_t serializationKey = 'SKDC';
	constexpr std::uint32_t skillUsageRecordType = 'SKUS';
	constexpr std::uint32_t skillUsageVersion = 1;

	bool Write(SKSE::SerializationInterface* a_interface, const SkillUsage& skill)
	{
		if (!a_interface->OpenRecord(skillUsageRecordType, skillUsageVersion)) {
			return false;
		}

		return details::Write(a_interface, skill.daysPassedWhenLastUsed) &&
		       details::Write(a_interface, skill.lastKnownLevel) &&
		       details::Write(a_interface, skill.lastKnownXP) &&
		       details::Write(a_interface, skill.lastKnownLegendaryLevel) &&
		       details::Write(a_interface, skill.lastKnownHighestLevel) &&
		       details::Write(a_interface, skill.isDecaying) &&
		       details::Write(a_interface, skill.daysPassedSinceLastDecay);
	}

	bool Read(SKSE::SerializationInterface* a_interface, SkillUsage& skill)
	{
		return details::Read(a_interface, skill.daysPassedWhenLastUsed) &&
		       details::Read(a_interface, skill.lastKnownLevel) &&
		       details::Read(a_interface, skill.lastKnownXP) &&
		       details::Read(a_interface, skill.lastKnownLegendaryLevel) &&
		       details::Read(a_interface, skill.lastKnownHighestLevel) &&
		       details::Read(a_interface, skill.isDecaying) &&
		       details::Read(a_interface, skill.daysPassedSinceLastDecay);
	}

	void DecayTracker::Register()
	{
		const auto serializationInterface = SKSE::GetSerializationInterface();
		serializationInterface->SetUniqueID(serializationKey);
		serializationInterface->SetSaveCallback(Save);
		serializationInterface->SetLoadCallback(Load);
		serializationInterface->SetRevertCallback(Revert);

		if (const auto ui = RE::UI::GetSingleton()) {
			ui->AddEventSink(&GetInstance());
		}
	}

	void DecayTracker::Load(SKSE::SerializationInterface* interface)
	{
		logger::info("{:*^30}", " LOADING ");

		std::uint32_t type, version, length;
		Skill         skill = Skill::kOneHanded;

		auto& tracker = GetInstance();
		tracker.lastDaysPassed = 0.0f;

		while (skill < Skill::kTotal && interface->GetNextRecordInfo(type, version, length)) {
			if (type == skillUsageRecordType) {
				switch (version) {
				case 1:
					if (Read(interface, tracker[skill])) {
						logger::info("Loaded usage for {}", SkillName(skill));
					} else {
						logger::error("Failed to load usage for {}. SkillUsage will be reset.", SkillName(skill));
					}
					break;
				default:
					logger::error("Unsupported SkillUsage version: {} for {}. SkillUsage will be reset.", version, SkillName(skill));
					break;
				}
				Inc(skill);
			}
		}
	}

	void DecayTracker::Save(SKSE::SerializationInterface* interface)
	{
		logger::info("{:*^30}", " SAVING ");

		auto& tracker = GetInstance();

		// Before saving, we want to make sure that skill usage is at the most recent state.
		// This avoid situations when player gains XP or levels up a skill and immediately saves. This would
		tracker.UpdateSkillUsage(RE::Calendar::GetSingleton());

		for (Skill skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			if (interface->OpenRecord(skillUsageRecordType, skillUsageVersion)) {
				if (Write(interface, tracker[skill])) {
					logger::info("Saved usage for {}", SkillName(skill));
				} else {
					logger::error("Failed to save usage for {}", SkillName(skill));
				}
			}
		}
	}

	void DecayTracker::Revert(SKSE::SerializationInterface*)
	{
		logger::info("{:*^30}", " REVERTING ");
		GetInstance().lastDaysPassed = 0.0f;
		auto& tracker = GetInstance();
		for (Skill skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			tracker[skill].Revert();
			logger::info("Reverted usage for {}", SkillName(skill));
		}
	}
}
