#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
using namespace std::literals;

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif

#include "Version.h"


using SkillData = RE::PlayerCharacter::PlayerSkills::Data::SkillData;
using Skill = RE::PlayerCharacter::PlayerSkills::Data::Skill;

#define Player RE::PlayerCharacter::GetSingleton()
#define AV(skill) static_cast<RE::ActorValue>(skill + 6)
#define SkillName(skill) RE::ActorValueList::GetActorValueName(AV(skill))
