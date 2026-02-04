#include "DecayTracker.h"
#include "Options.h"

#define Inc(skill) \
	skill = static_cast<Skill>(static_cast<std::underlying_type_t<Skill>>(skill) + 1)
	
namespace Decay
{
	void DecayTracker::AdvanceTime(RE::Calendar* calendar)
	{
		float daysPassed = calendar->GetDaysPassed();
		float hoursPassed = (daysPassed - lastDaysPassed) * 24.0;

		if (hoursPassed > Settings::fDecayTrackerRate()) {
			lastDaysPassed = daysPassed;
			UpdateSkillUsage(calendar);
		}
	}

	void DecayTracker::LoadSettings()
	{
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill)) {
			skillUsages[skill].Init(skill);
		}
	}

	RE::BSEventNotifyControl DecayTracker::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (event->menuName == RE::RaceSexMenu::MENU_NAME && !event->opening) {
			LoadSettings();
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void DecayTracker::UpdateSkillUsage(RE::Calendar* calendar)
	{
		const std::string timestamp = std::format("{} {:.0f}:{}", calendar->GetDayName(), calendar->GetHour(), calendar->GetMinutes());

		logger::info("{:*^65}", " Skill Data ");
		logger::info("[{:^13}] {} | {:^11} | {:^7} | {:^9} | {:^8}", timestamp, "D", "Skill", "Level", "Threshold", "XP");

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
			logger::info("[{:^13}] {} | {:^11} | {:^3.0f}[{:^2}] | {:^9.2f} | {:^8.2f}", timestamp, decayStatus, SkillName(skill), Player->GetBaseActorValue(AV(skill)), usage.GetStartingLevel(), skillData.levelThreshold, skillData.xp);
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
			details::Write(a_interface, skill.isDecaying) &&
			details::Write(a_interface, skill.daysPassedSinceLastDecay);
	}

	bool Read(SKSE::SerializationInterface* a_interface, SkillUsage& skill)
	{
		return details::Read(a_interface, skill.daysPassedWhenLastUsed) &&
			details::Read(a_interface, skill.lastKnownLevel) &&
			details::Read(a_interface, skill.lastKnownXP) &&
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

		tracker.LoadSettings();
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
