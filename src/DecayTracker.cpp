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
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill))
		{
			skillUsages[skill].Init(skill);
		}
	}

	void DecayTracker::UpdateSkillUsage(RE::Calendar* calendar)
	{
		const std::string timestamp = std::format("{} {:.0f}:{}", calendar->GetDayName(), calendar->GetHour(), calendar->GetMinutes());

		logger::info("{:*^50}", " Skill Data ");
		logger::info("[{:<13}] {} {:<11} | {:<5} | {:<9} | {:<8}", timestamp, "D", "Skill", "Level", "Threshold", "XP");

		const auto player = RE::PlayerCharacter::GetSingleton();
		for (auto skill = Skill::kOneHanded; skill < Skill::kTotal; Inc(skill))
		{
			auto& skillData = player->skills->data->skills[skill];
			const auto av = static_cast<RE::ActorValue>(skill + 6);
					
			auto& usage = skillUsages[skill];
			std::string  decayStatus = "-";

			if (!usage.IsInitialized() || usage.WasUsed(skillData)) {
				usage.SetUsed(skillData, calendar);
				decayStatus = "↑";
			} else if (usage.IsDecaying()) {
				usage.Decay(skillData, calendar);
				decayStatus = "↓";
			} else if (usage.IsStale(calendar)) {
				usage.MarkDecaying(calendar);
				decayStatus = "-";
			}

			const auto skillName = RE::ActorValueList::GetActorValueName(av);
			logger::info("[{:<13}] {} {:<11} | {:<5.0f} | {:<9.2f} | {:<8.2f}", timestamp, decayStatus, skillName, player->GetBaseActorValue(av), skillData.levelThreshold, skillData.xp);
		}
		logger::info("");
	}	
}
