#pragma once
#include "../include/SkillDecay_API.h"
#include <memory>

namespace Decay
{
	enum MESSAGE_TYPE : std::uint32_t
	{
		kSkillDecayInterface,
	};

	class DecayInterface : public SkillDecay::API
	{
	private:
		SkillDecay::InterfaceVersion version{ SkillDecay::InterfaceVersion::kV1 };
		DecayInterface(SkillDecay::InterfaceVersion version) :
			version(version) {}
		virtual ~DecayInterface() noexcept = default;

	public:
		static DecayInterface* GetSingleton(SkillDecay::InterfaceVersion version = SkillDecay::InterfaceVersion::kV1) noexcept
		{
			static DecayInterface singleton(version);
			return std::addressof(singleton);
		}

		bool IsDecaying(RE::ActorValue avSkill) noexcept override;
		bool IsDecaying(RE::PlayerCharacter::PlayerSkills::Data::Skill skill) noexcept override;
		bool IsDecaying(const char* skillId) noexcept override;

		void DecaySkill(RE::ActorValue avSkill, float decayXP, bool decayLevels) noexcept override;
		void DecaySkill(RE::PlayerCharacter::PlayerSkills::Data::Skill skill, float decayXP, bool decayLevels) noexcept override;
		void DecaySkill(const char* skillId, float decayXP, bool decayLevels) noexcept override;

		void ResetDecay(RE::ActorValue avSkill) noexcept override;
		void ResetDecay(RE::PlayerCharacter::PlayerSkills::Data::Skill skill) noexcept override;
		void ResetDecay(const char* skillId) noexcept override;

		bool RegisterCustomSkill(
			const char*                       skillId,
			RE::ActorValueInfo*               avi,
			RE::TESGlobal*                    levelGlobal,
			RE::TESGlobal*                    xpGlobal,
			bool                              xpNormalized,
			RE::TESGlobal*                    legendaryGlobal,
			const std::pair<RE::FormID, int>* raceBonuses,
			size_t                            raceBonusesCount) noexcept override;

		void UnregisterCustomSkill(const char* skillId) noexcept override;
		bool IsCustomSkillRegistered(const char* skillId) noexcept override;
	};
}
