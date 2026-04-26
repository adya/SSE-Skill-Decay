#include "DecayTracker.h"
#include "CLIBUtil/distribution.hpp"
#include "CLIBUtil/simpleINI.hpp"
#include "CLIBUtil/string.hpp"
#include "Options.h"

#define Inc(skill) \
	skill = static_cast<Skill>(static_cast<std::underlying_type_t<Skill>>(skill) + 1)

namespace Decay
{
	void DecayTracker::AdvanceTime(RE::Calendar* calendar)
	{
		if (!initialized)
			return;

		float daysPassed = calendar->GetDaysPassed();
		float hoursPassed = (daysPassed - lastDaysPassed) * 24.0;

		if (hoursPassed > trackingRate) {
			lastDaysPassed = daysPassed;
			UpdateSkillUsage(calendar);
		}
	}

	struct PartialDecayConfig
	{
		std::optional<float>                    gracePeriod;
		std::optional<float>                    interval;
		std::optional<int>                      baselineLevelOffset;
		std::optional<int>                      levelOffset;
		std::optional<float>                    difficultyMult;
		std::optional<int>                      difficultyOverride;
		std::optional<float>                    damping;
		std::optional<float>                    legendarySkillDamping;
		std::optional<int>                      levelCap;
		std::optional<float>                    minDaysPerLevel;
		std::optional<float>                    maxDaysPerLevel;
		std::optional<RE::GColor>               decayTint;
		std::optional<RE::GColor>               normalTint;

		void ApplyTo(DecayConfig& config) const
		{
			if (gracePeriod.has_value()) {
				config.gracePeriod = gracePeriod.value();
			}
			if (interval.has_value()) {
				config.interval = interval.value();
			}
			if (baselineLevelOffset.has_value()) {
				config.baselineLevelOffset = baselineLevelOffset.value();
			}
			if (levelOffset.has_value()) {
				config.levelOffset = levelOffset.value();
			}
			if (difficultyMult.has_value()) {
				config.difficultyMult = difficultyMult.value();
			}
			if (difficultyOverride.has_value()) {
				config.difficultyOverride = difficultyOverride.value();
			}
			if (damping.has_value()) {
				config.damping = damping.value();
			}
			if (legendarySkillDamping.has_value()) {
				config.legendarySkillDamping = legendarySkillDamping.value();
			}
			if (levelCap.has_value()) {
				config.levelCap = levelCap.value();
			}
			if (minDaysPerLevel.has_value()) {
				config.minDaysPerLevel = minDaysPerLevel.value();
			}
			if (maxDaysPerLevel.has_value()) {
				config.maxDaysPerLevel = maxDaysPerLevel.value();
			}
			if (decayTint.has_value()) {
				config.decayTint = decayTint.value();
			}
			if (normalTint.has_value()) {
				config.normalTint = normalTint.value();
			}
		}
	};

	void ReadSettings(const CSimpleIniA& ini, const char* section, PartialDecayConfig& config)
	{
		if (!ini.SectionExists(section)) {
			return;
		}

		if (ini.KeyExists(section, "fDecayGracePeriod"))
			config.gracePeriod = ini.GetDoubleValue(section, "fDecayGracePeriod");
		if (ini.KeyExists(section, "fDecayInterval"))
			config.interval = ini.GetDoubleValue(section, "fDecayInterval");
		if (ini.KeyExists(section, "iDecayLevelOffset"))
			config.levelOffset = ini.GetLongValue(section, "iDecayLevelOffset");
		if (ini.KeyExists(section, "iBaselineLevelOffset"))
			config.baselineLevelOffset = ini.GetLongValue(section, "iBaselineLevelOffset");
		if (ini.KeyExists(section, "fDecayXPDamping"))
			config.damping = ini.GetDoubleValue(section, "fDecayXPDamping");
		if (ini.KeyExists(section, "fDecayXPDifficultyMult"))
			config.difficultyMult = ini.GetDoubleValue(section, "fDecayXPDifficultyMult");
		if (ini.KeyExists(section, "iDecayLevelCap"))
			config.levelCap = ini.GetLongValue(section, "iDecayLevelCap");
		if (ini.KeyExists(section, "fLegendarySkillXPDamping"))
			config.legendarySkillDamping = ini.GetDoubleValue(section, "fLegendarySkillXPDamping");
		if (ini.KeyExists(section, "fMinDaysPerLevel"))
			config.minDaysPerLevel = ini.GetDoubleValue(section, "fMinDaysPerLevel");
		if (ini.KeyExists(section, "fMaxDaysPerLevel"))
			config.maxDaysPerLevel = ini.GetDoubleValue(section, "fMaxDaysPerLevel");
		if (ini.KeyExists(section, "iDifficulty"))
			config.difficultyOverride = ini.GetLongValue(section, "iDifficulty");
		std::string color = ini.GetValue(section, "cDecayTint", "");

		if (!color.empty()) {
			config.decayTint = clib_util::string::to_color(color);
		}

		color = ini.GetValue(section, "cTint", "");

		if (!color.empty()) {
			config.normalTint = clib_util::string::to_color(color);
		}
	}

	void ValidateConfig(DecayConfig& config, const DecayConfig& defaults)
	{
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

		config.difficultyOverride = min(config.difficultyOverride, 5);
	}

	void LogSkillConfig(std::string skillName, const DecayConfig& config)
	{
		static constexpr std::string_view difficultyNames[] = { "Novice", "Apprentice", "Adept", "Expert", "Master", "Legendary" };

		logger::info("{:>16} | {:^12} | {:^14} | {:^15} | {:^12} | {:^10} | {:^15} | {:^7} | {:^17} | {:^9} | {:^14} | {:^14}",
			skillName,
			std::signbit(config.gracePeriod) ? "Auto" : std::format("{:.1f}h", config.gracePeriod),
			std::format("{:.1f}h", config.interval),
			config.baselineLevelOffset < 0 ? "Auto" : std::format("{}", config.baselineLevelOffset),
			config.levelOffset,
			config.difficultyOverride < 0 ? "Auto" : difficultyNames[config.difficultyOverride],
			std::signbit(config.difficultyMult) ? "Auto" : std::format("{:.2f}", config.difficultyMult),
			std::format("/{:.2f}", config.damping),
			std::format("+{:.0f}%", (config.legendarySkillDamping - 1) * 100.0f),
			config.levelCap == INT_MAX ? "Auto" : (config.levelCap == 0 ? "Current" : std::format("{}", config.levelCap)),
			std::format("{:.1f}d", config.minDaysPerLevel),
			std::format("{:.1f}d", config.maxDaysPerLevel));
	}

	void DecayTracker::LoadDefaultSkills()
	{
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			defaultSkills[skill] = new PlayerSkillData(skill);
		}
	}

	void DecayTracker::LoadSettings()
	{
		logger::info("{:*^30}", " OPTIONS ");

		auto files = clib_util::distribution::get_configs_paths(R"(Data\SKSE\Plugins\SkillDecay)");

		std::map<std::string, PartialDecayConfig> configs{};
		PartialDecayConfig                        defaultConfig{};    // no sections
		PartialDecayConfig                        overwriteConfig{};  // [All]

		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			configs[PlayerSkillData::DEFAULT_SKILL_NAMES[skill]] = PartialDecayConfig();
		}

		for (const auto& [skillId, skillData] : customSkills) {
			if (!configs.contains(skillId)) {
				configs[skillId] = PartialDecayConfig();
			}
		}

		if (files.empty()) {
			logger::info(R"(No configs found in Data\SKSE\Plugins\SkillDecay\ folder. Default options will be used.)");
		}

		std::filesystem::path legacyConfigPath = R"(Data\SKSE\Plugins\SkillDecay.ini)";
		if (std::filesystem::exists(legacyConfigPath)) {
			files.push_back(legacyConfigPath);  // Support legacy config.
		}

		logger::info("{} matching inis found", files.size());

		for (const auto& path : files) {
			logger::info("\tINI : {}", path.string());

			CSimpleIniA ini{};
			ini.SetUnicode();
			ini.SetMultiKey(false);

			if (ini.LoadFile(path.c_str()) >= 0) {
				float defaultTrackingRate = trackingRate;
				trackingRate = ini.GetDoubleValue("", "fTrackingRate", trackingRate);
				logSkillUsage = ini.GetBoolValue("", "bLogSkillUsage", logSkillUsage);
				logStatsMenuTree = ini.GetBoolValue("", "bLogStatsMenuTree", logStatsMenuTree);
				if (trackingRate <= 0) {
					trackingRate = defaultTrackingRate;
				}

				// Load default and overwrite global configs that will affect all skills.
				ReadSettings(ini, "", defaultConfig);
				ReadSettings(ini, "All", overwriteConfig);

				for (auto& [section, config] : configs) {
					ReadSettings(ini, section.c_str(), config);
				}
			}
		}

		DecayConfig defaultConfigs[Skill::kTotal] = {
			/* One-Handed */ DecayConfig(),
			/* Two-Handed */ DecayConfig(),
			/* Archery */ DecayConfig(),
			/* Block */ DecayConfig(),
			/* Smithing */ DecayConfig(2),
			/* Heavy Armor */ DecayConfig(),
			/* Light Armor */ DecayConfig(),
			/* Pickpocket */ DecayConfig(2),
			/* Lockpicking */ DecayConfig(2),
			/* Sneaking */ DecayConfig(1.5f),
			/* Alchemy */ DecayConfig(),
			/* Speech */ DecayConfig(),
			/* Alteration */ DecayConfig(),
			/* Conjuration */ DecayConfig(),
			/* Destruction */ DecayConfig(),
			/* Illusion */ DecayConfig(),
			/* Restoration */ DecayConfig(),
			/* Enchanting */ DecayConfig(1.25f)
		};

		logger::info("{}", logSkillUsage ? "Logging Skill Usage enabled" : "Logging Skill Usage disabled");
		auto formattedRate = trackingRate < 1.0f ? std::format("{:.2f} in-game minutes", trackingRate * 60.0f) : std::format("{:.2f} in-game hours", trackingRate);
		logger::info("Tracking Rate: once every {}", formattedRate);

		logger::info("{:>16} | {:^12} | {:^14} | {:^15} | {:^12} | {:^10} | {:^15} | {:^7} | {:^17} | {:^9} | {:^14} | {:^14}",
			"Skill", "Grace Period", "Decay Duration", "Baseline Offset", "Extra Offset", "Difficulty", "Difficulty Mult", "Damping", "Legendary Damping", "Decay Cap", "Min Decay Days", "Max Decay Days");
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			auto config = defaultConfigs[skill];
			defaultConfig.ApplyTo(config);
			configs[PlayerSkillData::DEFAULT_SKILL_NAMES[skill]].ApplyTo(config);
			configs.erase(PlayerSkillData::DEFAULT_SKILL_NAMES[skill]);
			overwriteConfig.ApplyTo(config);

			ValidateConfig(config, defaultConfigs[skill]);

			skillConfigs[skill] = config;
			skillUsages[skill].Init(defaultSkills[skill], config);
			LogSkillConfig(SkillName(skill), config);
		}

		if (!configs.empty()) {
			logger::info("{:->16} | {:-^12} | {:-^14} | {:-^15} | {:-^12} | {:-^10} | {:-^15} | {:-^7} | {:-^17} | {:-^9} | {:-^14} | {:-^14}",
				"", "", "", "", "", "", "", "", "", "", "", "");
		}

		DecayConfig empty{};
		for (const auto& [skillId, customConfig] : configs) {
			DecayConfig config{};
			defaultConfig.ApplyTo(config);
			customConfig.ApplyTo(config);
			overwriteConfig.ApplyTo(config);

			ValidateConfig(config, empty);

			customSkillConfigs[skillId] = config;
			LogSkillConfig(skillId, config);
		}

		for (const auto& [skillId, skillData] : customSkills) {
			customSkillUsages[skillId].Init(skillData, customSkillConfigs[skillId]);
		}

		initialized = true;
	}

	void DecayTracker::ApplyTint(RE::GFxMovieView* movie, const std::vector<RE::ActorValue>& skillGroup) const
	{
		if (skillGroup.empty())
			return;

		// Build AV -> SkillUsage lookup covering both default and custom skills.
		std::map<RE::ActorValue, const SkillUsage*> avToUsage;
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			avToUsage[AV(skill)] = &skillUsages[skill];
		}
		for (const auto& [skillId, skillData] : customSkills) {
			auto usageIt = customSkillUsages.find(skillId);
			if (usageIt != customSkillUsages.end()) {
				avToUsage[skillData->av] = &usageIt->second;
			}
		}

		// The Nth entry in skillGroup corresponds to SkillTextN in the GFx tree.
		// For index N: path1 = SkillTextN.ShortBar.instance{94+N*6}
		//              path2 = SkillTextN.ShortBar.instance{94+N*6+2}
		for (std::size_t n = 0; n < skillGroup.size(); ++n) {
			auto avIt = avToUsage.find(skillGroup[n]);
			if (avIt == avToUsage.end())
				continue;
			const auto& usage  = *avIt->second;
			const auto& config = usage.GetConfig();

			const auto instanceBase = 94 + n * 6;
			const auto path1 = std::format("_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText{}.ShortBar.instance{}", n, instanceBase);
			const auto path2 = std::format("_root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText{}.ShortBar.instance{}", n, instanceBase + 2);

			if (usage.IsDecaying()) {
				if (config.decayTint.colorData.channels.alpha > 0) {
					movie->SetColorTint(path1.c_str(), config.decayTint);
					movie->SetColorTint(path2.c_str(), config.decayTint);
				}
			} else if (config.normalTint.colorData.channels.alpha > 0) {
				movie->SetColorTint(path1.c_str(), config.normalTint);
				movie->SetColorTint(path2.c_str(), config.normalTint);
			}
		}
	}

	void DecayTracker::RegisterCustomSkill(const std::string& skillId, RE::ActorValue av, RE::ActorValueInfo* avi, RE::TESGlobal* levelGlobal, RE::TESGlobal* xpGlobal, bool xpNormalized, RE::TESGlobal* legendaryGlobal, std::map<RE::FormID, int>& raceBonuses) noexcept
	{
		assert(!PlayerSkillData::IsDefaultSkill(skillId));

		auto skill = new CustomSkillData(skillId, av, avi, levelGlobal, xpGlobal, xpNormalized, legendaryGlobal, raceBonuses);
		skill->UpdateRaceBonus();
		customSkills[skillId] = skill;
	}

	void DecayTracker::UnregisterCustomSkill(const std::string& skillId) noexcept
	{
		auto it = customSkills.find(skillId);
		if (it != customSkills.end()) {
			delete it->second;
			customSkills.erase(it);
			customSkillUsages.erase(skillId);
			customSkillConfigs.erase(skillId);
		}
	}

	bool DecayTracker::IsCustomSkillRegistered(const std::string& skillId) const noexcept
	{
		return customSkills.find(skillId) != customSkills.end();
	}

	SkillUsage* DecayTracker::GetCustomSkillUsage(const std::string& skillId) noexcept
	{
		auto it = customSkillUsages.find(skillId);
		if (it != customSkillUsages.end()) {
			return &it->second;
		}
		return nullptr;
	}

	RE::BSEventNotifyControl DecayTracker::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (event->menuName == RE::RaceSexMenu::MENU_NAME && !event->opening) {
			for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
				defaultSkills[skill]->UpdateRaceBonus();
				skillUsages[skill].UpdateBaselineLevel();
			}
			for (const auto& [skillId, skillData] : customSkills) {
				skillData->UpdateRaceBonus();
				customSkillUsages[skillId].UpdateBaselineLevel();
				customSkillUsages[skillId].Init(skillData, customSkillConfigs[skillId]);
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void DecayTracker::UpdateSkillUsage(RE::Calendar* calendar)
	{
		const std::string timestamp = std::format("{} {:.0f}:{}", calendar->GetDayName(), calendar->GetHour(), calendar->GetMinutes());

		if (logSkillUsage) {
			logger::info("{:*^65}", " Skill Data ");
			logger::info("[{:^13}] {} | {:^16} | {:^11} | {:^9} | {:^8}", timestamp, "D", "Skill", "Level [Cap]", "Threshold", "XP");
		}

		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			auto&       usage = skillUsages[skill];

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
				std::string levelInfo = std::format("{:^3}[{:^2}]", usage.GetSkill()->GetLevel(), usage.GetDecayCapLevel());
				logger::info("[{:^13}] {} | {:^16} | {:^11} | {:^9.2f} | {:^8.2f}", timestamp, decayStatus, SkillName(skill), levelInfo, usage.GetSkill()->GetLevelThreshold(), usage.GetSkill()->GetXP());
			}
		}

		if (!customSkillUsages.empty() && logSkillUsage) {
			logger::info("[{:^13}] - | {:-^16} | {:-^11} | {:-^9} | {:-^8}",
				timestamp, "", "", "", "");
		}

		for (auto& [skillId, usage] : customSkillUsages) {
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
				std::string levelInfo = std::format("{:^3}[{:^2}]", usage.GetSkill()->GetLevel(), usage.GetDecayCapLevel());
				 logger::info("[{:^13}] {} | {:^16} | {:^11} | {:^9.2f} | {:^8.2f}", timestamp, decayStatus, skillId, levelInfo, usage.GetSkill()->GetLevelThreshold(), usage.GetSkill()->GetXP());
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
	constexpr std::uint32_t customSkillUsageRecordType = 'CSKU';
	constexpr std::uint32_t skillUsageVersion = 1;

	bool Write(SKSE::SerializationInterface* a_interface, const SkillUsage& skill)
	{
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

		auto& tracker = GetInstance();
		tracker.lastDaysPassed = 0.0f;

		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			tracker[skill].Init(tracker.defaultSkills[skill], tracker[skill].GetConfig());
		}

		Skill skill = Skill::kOneHanded;
		while (interface->GetNextRecordInfo(type, version, length)) {
			if (type == skillUsageRecordType) {
				if (skill >= Skill::kTotal) {
					logger::warn("More default skill records than expected, skipping.");
					continue;
				}
				switch (version) {
				case 1:
					if (Read(interface, tracker[skill])) {
						logger::info("Loaded usage for {}", SkillName(skill));
					} else {
						tracker[skill].SetUsed(RE::Calendar::GetSingleton());
						logger::error("Failed to load usage for '{}'. SkillUsage will be reset.", SkillName(skill));
					}
					break;
				default:
					logger::error("Unsupported SkillUsage version: {} for '{}'. SkillUsage will be reset.", version, SkillName(skill));
					break;
				}
				Inc(skill);
			} else if (type == customSkillUsageRecordType) {
				switch (version) {
				case 1: {
					std::string skillId;
					SkillUsage  usage;
					if (details::Read(interface, skillId) && Read(interface, usage)) {
						auto customIt = tracker.customSkills.find(skillId);
						if (customIt != tracker.customSkills.end()) {
							tracker.customSkillUsages[skillId] = usage;
							logger::info("Loaded usage for custom skill '{}'", skillId);
						} else {
							logger::info("Discarding saved usage for unregistered custom skill '{}'.", skillId);
						}
					} else {
						logger::error("Failed to load usage for custom skill. Record will be skipped.");
					}
					break;
				}
				default:
					logger::error("Unsupported custom SkillUsage version: {}. Record will be skipped.", version);
					break;
				}
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
					logger::info("Saved usage for '{}'", SkillName(skill));
				} else {
					logger::error("Failed to save usage for '{}'", SkillName(skill));
				}
			}
		}

		for (const auto& [skillId, usage] : tracker.customSkillUsages) {
			if (interface->OpenRecord(customSkillUsageRecordType, skillUsageVersion)) {
				if (details::Write(interface, skillId) && Write(interface, usage)) {
					logger::info("Saved usage for '{}'", skillId);
				} else {
					logger::error("Failed to save usage for '{}'", skillId);
				}
			}
		}
	}

	void DecayTracker::Revert(SKSE::SerializationInterface*)
	{
		logger::info("{:*^30}", " REVERTING ");
		auto& tracker = GetInstance();
		tracker.lastDaysPassed = 0.0f;
		for (Skill skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			tracker[skill].Revert();
			logger::info("Reverted usage for '{}'", SkillName(skill));
		}
		for (auto& [skillId, usage] : tracker.customSkillUsages) {
			usage.Revert();
			logger::info("Reverted usage for '{}'", skillId);
		}
	}
}
