#pragma once

namespace SkillDecay
{
	constexpr auto PluginName = "SkillDecay";

	/// Available interface versions.
	enum class InterfaceVersion : uint8_t
	{
		kV1
	};

	class API
	{
	public:
		/// <summary>
		/// Checks whether the specified skill is currently decaying.
		/// </summary>
		/// <param name="avSkill">ActorValue of a skill to be checked. The ActorValue must point to a valid skill, otherwise false is returned.</param>
		/// <returns>Flag indicating whether the skill is currently decaying</returns>
		virtual bool IsDecaying(RE::ActorValue avSkill) noexcept = 0;

		/// <summary>
		/// Checks whether the specified skill is currently decaying.
		/// </summary>
		/// <param name="skill">Skill to be checked</param>
		/// <returns>Flag indicating whether the skill is currently decaying</returns>
		virtual bool IsDecaying(RE::PlayerCharacter::PlayerSkills::Data::Skill skill) noexcept = 0;

		/// <summary>
		/// Applies decay to a specified skill, reducing its XP and optionally its levels.
		/// </summary>
		/// <param name="avSkill">The skill to decay, specified as an ActorValue. The ActorValue must point to a valid skill.</param>
		/// <param name="decayXP">The amount of XP to subtract from the skill.</param>
		/// <param name="decayLevels">If true and decayXP is larger than the skill's current XP, skill level will be reduced and remaining decayXP subtracted.</param>
		virtual void DecaySkill(RE::ActorValue avSkill, float decayXP, bool decayLevels) noexcept = 0;

		/// <summary>
		/// Applies decay to a specified skill, reducing its XP and optionally its levels.
		/// </summary>
		/// <param name="skill">The skill to decay.</param>
		/// <param name="decayXP">The amount of XP to subtract from the skill.</param>
		/// <param name="decayLevels">If true and decayXP is larger than the skill's current XP, skill level will be reduced and remaining decayXP subtracted.</param>
		virtual void DecaySkill(RE::PlayerCharacter::PlayerSkills::Data::Skill skill, float decayXP, bool decayLevels) noexcept = 0;

		/// <summary>
		/// Resets decaying state of a specified skill, preventing it from decaying until it is applied again.
		///
		/// If the skill is not currently decaying, this function does nothing.
		/// </summary>
		/// <param name="avSkill">The skill to reset, specified as an ActorValue. The ActorValue must point to a valid skill.</param>
		virtual void ResetDecay(RE::ActorValue avSkill) noexcept = 0;

		/// <summary>
		/// Resets decaying state of a specified skill, preventing it from decaying until it is applied again.
		///
		/// If the skill is not currently decaying, this function does nothing.
		/// </summary>
		/// <param name="skill">The skill to reset.</param>
		virtual void ResetDecay(RE::PlayerCharacter::PlayerSkills::Data::Skill skill) noexcept = 0;
	};

	typedef void* (*_RequestPluginAPI)(const InterfaceVersion interfaceVersion);

	/// <summary>
	/// Request the SkillDecay API interface.
	/// Recommended: Send your request during or after SKSEMessagingInterface::kMessage_PostLoad to make sure the dll has already been loaded
	/// </summary>
	/// <param name="a_interfaceVersion">The interface version to request</param>
	/// <returns>The pointer to the API singleton, or nullptr if request failed</returns>
	[[nodiscard]] inline void* RequestPluginAPI(const InterfaceVersion a_interfaceVersion = InterfaceVersion::kV1)
	{
		const auto pluginHandle = GetModuleHandleA("SkillDecay.dll");
		if (_RequestPluginAPI requestAPIFunction = (_RequestPluginAPI)GetProcAddress(pluginHandle, "RequestPluginAPI"); requestAPIFunction) {
			return requestAPIFunction(a_interfaceVersion);
		}
		return nullptr;
	}
}
