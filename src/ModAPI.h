#pragma once
#include <memory>
#include "../include/SkillDecay_API.h"

namespace Decay
{
	class DecayInterface : public SkillDecay::API
	{
	private:
		SkillDecay::InterfaceVersion version{ SkillDecay::InterfaceVersion::kV1 };
		DecayInterface(SkillDecay::InterfaceVersion version) :
			version(version) {}
		virtual ~DecayInterface() noexcept = default;

	public:
		static DecayInterface* GetSingleton(SkillDecay::InterfaceVersion version) noexcept
		{
			static DecayInterface singleton(version);
			return std::addressof(singleton);
		}

		bool IsDecaying(RE::ActorValue avSkill) noexcept override;
		bool IsDecaying(RE::PlayerCharacter::PlayerSkills::Data::Skill skill) noexcept override;
		void DecaySkill(RE::ActorValue avSkill, float decayXP, bool decayLevels) noexcept override;
		void DecaySkill(RE::PlayerCharacter::PlayerSkills::Data::Skill skill, float decayXP, bool decayLevels) noexcept override;
		void ResetDecay(RE::ActorValue avSkill) noexcept override;
		void ResetDecay(RE::PlayerCharacter::PlayerSkills::Data::Skill skill) noexcept override;
	};
}
