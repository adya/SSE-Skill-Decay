#include "Hooks.h"
#include "DecayTracker.h"
#include "Hooking.h"
#include "Options.h"
#include <vector>

namespace Decay
{
	void PrintGFxValueTree(const RE::GFxValue& value, const std::string& path, int depth = 0, std::vector<std::string>* visitedPath = nullptr)
	{
		// Create visited path on first call
		std::vector<std::string> localPath;
		if (!visitedPath) {
			visitedPath = &localPath;
		}

		std::string indent(depth * 2, ' ');

		if (value.IsArray()) {
			logger::info("{}[Array[{}]] {}/", indent, value.GetArraySize(), path);
			for (std::uint32_t i = 0; i < value.GetArraySize(); ++i) {
				RE::GFxValue element;
				if (value.GetElement(i, &element)) {
					std::string elementPath = "[" + std::to_string(i) + "]";
					PrintGFxValueTree(element, elementPath, depth + 1, visitedPath);
				}
			}
		} else if (value.IsObject()) {
			logger::info("{}[Object] {}/", indent, path);
			value.VisitMembers([&](const char* memberName, const RE::GFxValue& memberValue) {
				// Skip known recursive patterns
				if (strcmp(memberName, "ThisInstance") == 0) {
					return;
				}

				// Check if this member name is already in the current path (circular reference)
				if (std::find(visitedPath->begin(), visitedPath->end(), memberName) != visitedPath->end()) {
					std::string memberIndent((depth + 1) * 2, ' ');
					logger::info("{}{}/ (CIRCULAR REFERENCE)", memberIndent, memberName);
					return;
				}

				// Add current member to the path
				visitedPath->push_back(memberName);

				// Print only the member name, not the full path
				PrintGFxValueTree(memberValue, memberName, depth + 1, visitedPath);

				// Remove current member from the path when backtracking
				visitedPath->pop_back();
			});
		} else {
			std::string typeStr = "Unknown";
			if (value.IsString())
				typeStr = "String";
			else if (value.IsNumber())
				typeStr = "Number";
			else if (value.IsBool())
				typeStr = "Bool";
			else if (value.IsUndefined())
				typeStr = "Undefined";
			else if (value.IsNull())
				typeStr = "Null";

			if (value.IsString()) {
				logger::info("{}[{}] {} = {}", indent, typeStr, path, value.GetString());
			} else if (value.IsNumber()) {
				logger::info("{}[{}] {} = {}", indent, typeStr, path, value.GetNumber());
			} else if (value.IsBool()) {
				logger::info("{}[{}] {} = {}", indent, typeStr, path, value.GetBool());
			}
		}
	}

	struct StatsMenu_ProcessMessage
	{
		using Target = RE::StatsMenu;
		static inline constexpr std::size_t index{ 0x4 };

		static inline bool printed = false;

		static RE::UI_MESSAGE_RESULTS thunk(RE::StatsMenu* menu, RE::UIMessage& msg)
		{
			auto result = func(menu, msg);

			if (msg.type == RE::UI_MESSAGE_TYPE::kUpdate) {
				if (auto movie = menu->uiMovie; movie) {
					const std::vector<RE::ActorValue> skillGroup(menu->skillTrees.begin(), menu->skillTrees.end());
					DecayTracker::GetInstance().ApplyTint(movie.get(), skillGroup);

				RE::GFxValue root;
					// .AnimatingSkillTextInstance.SkillText5.ShortBar
					if (movie->GetVariable(&root, "_root.StatsMenuBaseInstance")) {
						if (!printed && DecayTracker::GetInstance().LogStatsMenuTree()) {
						std::string avList;
							for (auto av : skillGroup) {
								if (!avList.empty()) avList += ", ";
								avList += std::to_string(static_cast<int>(av));
							}
							logger::info("=== StatsMenu Actor Values: [{}] ===", avList);
							logger::info("=== GFx Object Tree ===");
							PrintGFxValueTree(root, "_root.StatsMenuBaseInstance");
							logger::info("======================");
							printed = true;
						}
					}
				}
			} else if (msg.type == RE::UI_MESSAGE_TYPE::kHide) {
				printed = false;
			}

			return result;
		}

		static inline void post_hook()
		{
			logger::info("\t\t🪝Installed StatsMenu ProcessMessage hook.");
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct AdvanceTime_Main
	{
		static inline constexpr REL::ID     relocation = RELOCATION_ID(35565, 36564);
		static inline constexpr std::size_t offset = OFFSET(0x24D, 0x266);

		static void thunk(RE::Calendar* calendar, float deltaTime)
		{
			func(calendar, deltaTime);
			DecayTracker::GetInstance().AdvanceTime(calendar);
		}

		static inline void post_hook()
		{
			logger::info("\t\t🪝Installed AdvanceTime Main loop hook.");
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct AdvanceTime_FastTravel
	{
		static inline constexpr REL::ID     relocation = RELOCATION_ID(39373, 40445);
		static inline constexpr std::size_t offset = OFFSET(0x2B1, 0x282);

		static void thunk(RE::Calendar* calendar, float deltaTime)
		{
			func(calendar, deltaTime);
			DecayTracker::GetInstance().AdvanceTime(calendar);
		}

		static inline void post_hook()
		{
			logger::info("\t\t🪝Installed AdvanceTime Fast Travel hook.");
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct AdvanceTime_Sleep
	{
		static inline constexpr REL::ID     relocation = RELOCATION_ID(39410, 40485);
		static inline constexpr std::size_t offset = OFFSET(0x78, 0x78);

		static void thunk(RE::Calendar* calendar, float deltaTime)
		{
			func(calendar, deltaTime);
			DecayTracker::GetInstance().AdvanceTime(calendar);
		}

		static inline void post_hook()
		{
			logger::info("\t\t🪝Installed AdvanceTime Sleep hook.");
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		stl::install_hook<AdvanceTime_Main>();
		stl::install_hook<AdvanceTime_FastTravel>();
		stl::install_hook<AdvanceTime_Sleep>();

		stl::install_hook<StatsMenu_ProcessMessage>();
	}
}
