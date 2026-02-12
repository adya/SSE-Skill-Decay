#include "DecayTracker.h"
#include "Options.h"
#include "CLIBUtil/simpleINI.hpp"

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

	void DecayTracker::LoadSettings()
	{
		logger::info("{:*^30}", " OPTIONS ");
		std::filesystem::path options = R"(Data\SKSE\Plugins\SkillDecay.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		ini.SetMultiKey(false);

		DecayConfig configs[Skill::kTotal] = {
			DecayConfig(),      // One-Handed
			DecayConfig(),      // Two-Handed
			DecayConfig(),      // Archery
			DecayConfig(),      // Block
			DecayConfig(2),     // Smithing
			DecayConfig(),      // Heavy Armor
			DecayConfig(),      // Light Armor
			DecayConfig(2),     // Pickpocket
			DecayConfig(2),     // Lockpicking
			DecayConfig(1.5f),  // Sneaking
			DecayConfig(),      // Alchemy
			DecayConfig(),      // Speech
			DecayConfig(),      // Alteration
			DecayConfig(),      // Conjuration
			DecayConfig(),      // Destruction
			DecayConfig(),      // Illusion
			DecayConfig(),      // Restoration
			DecayConfig(1.25f)  // Enchanting
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
			if (trackingRate <= 0) {
				trackingRate = defaultTrackingRate;
			}

			for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
				DecayConfig& config = configs[skill];
				DecayConfig  defaults = config;
				const char*  section = sections[skill].c_str();

				// Load global overwrites for all skills first.
				config.gracePeriod = ini.GetDoubleValue("", "fDecayGracePeriod", config.gracePeriod);
				config.interval = ini.GetDoubleValue("", "fDecayInterval", config.interval);
				config.levelOffset = ini.GetLongValue("", "iDecayLevelOffset", config.levelOffset);
				config.baselineLevelOffset = ini.GetLongValue("", "iBaselineLevelOffset", config.baselineLevelOffset);
				config.damping = ini.GetDoubleValue("", "fDecayXPDamping", config.damping);
				config.difficultyMult = ini.GetDoubleValue("", "fDecayXPDifficultyMult", config.difficultyMult);
				config.levelCap = ini.GetLongValue("", "iDecayLevelCap", config.levelCap);
				config.legendarySkillDamping = ini.GetDoubleValue("", "fLegendarySkillXPDamping", config.legendarySkillDamping);
				config.minDaysPerLevel = ini.GetDoubleValue("", "fMinDaysPerLevel", config.minDaysPerLevel);
				config.maxDaysPerLevel = ini.GetDoubleValue("", "fMaxDaysPerLevel", config.maxDaysPerLevel);

				// Then apply skill-specific settings, if they exist.
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
				}

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
				std::format("{:.1f}d", configs[skill].maxDaysPerLevel)
			);
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

		logger::info("{:*^65}", " Skill Data ");
		logger::info("[{:^13}] {} | {:^11} | {:^11} | {:^9} | {:^8}", timestamp, "D", "Skill", "Level [Cap]", "Threshold", "XP");

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
			std::string levelInfo = std::format("{:^3.0f}[{:^2}]", Player->GetBaseActorValue(AV(skill)), usage.GetDecayCapLevel());
			logger::info("[{:^13}] {} | {:^11} | {:^11} | {:^9.2f} | {:^8.2f}", timestamp, decayStatus, SkillName(skill), levelInfo, skillData.levelThreshold, skillData.xp);
		}
		logger::info("");
	}
}

// Serialization
namespace Decay {
	
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

		const auto& tracker = GetInstance();

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
