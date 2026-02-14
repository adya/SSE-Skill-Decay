#include "Hooks.h"
#include "DecayTracker.h"
#include "Hooking.h"
#include "Options.h"
// TODO: Figure out path
// TODO: Look at Immersive HUD SKSE for code that prints available paths.
// _root.StatsMenuBaseInstance.AnimatingSkillTextInstance.SkillText[0-17].ShortBar
namespace Decay
{
	struct StatsMenu_PostDisplay
	{
		using Target = RE::StatsMenu;
		static inline constexpr std::size_t index{ 0x6 };

		static RE::UI_MESSAGE_RESULTS thunk(RE::StatsMenu* menu, RE::UIMessage& a_message)
		{
			if (auto movie = menu->uiMovie; movie) {
				logger::info("StatsMenu movie is valid");
				
				RE::GFxValue root;
				if (movie->GetVariable(&root, "_root")) {
					logger::info("Got _root successfully");
					
					// Try both possible names from the TODO comment
					const char* possibleNames[] = {
						"StatsMenuInstance",
						"StatsMenuBaseInstance",
						"statsMenuInstance",
						"statsMenuBaseInstance"
					};
					
					RE::GFxValue statsMenuInstance;
					bool found = false;
					const char* foundName = nullptr;
					
					for (const char* name : possibleNames) {
						if (root.GetMember(name, &statsMenuInstance)) {
							logger::info("Found stats menu instance with name: {}", name);
							foundName = name;
							found = true;
							break;
						}
					}
					
					if (!found) {
						logger::warn("Failed to find StatsMenu instance with any known name");
						return func(menu, a_message);
					}
					
					logger::info("Got {} successfully", foundName);
					
					RE::GFxValue topPlayerInfo;
					if (statsMenuInstance.GetMember("TopPlayerInfo", &topPlayerInfo)) {
						logger::info("Got TopPlayerInfo successfully");
						
						RE::GFxValue animate;
						if (topPlayerInfo.GetMember("animate", &animate)) {
							logger::info("Got animate successfully, type: {}", static_cast<int>(animate.GetType()));
							
							RE::GRenderer::Cxform colorTransform;
							if (animate.GetCxform(&colorTransform)) {
								logger::info("Got Cxform successfully");
								
								colorTransform.matrix[0][0] = 1.0f;
								colorTransform.matrix[1][0] = 0.7f;
								colorTransform.matrix[2][0] = 0.7f;
								colorTransform.matrix[3][0] = 1.0f;
								
								colorTransform.matrix[0][1] = 0.1f;
								colorTransform.matrix[1][1] = 0.0f;
								colorTransform.matrix[2][1] = 0.0f;
								colorTransform.matrix[3][1] = 0.0f;
								
								if (animate.SetCxform(colorTransform)) {
									logger::info("Successfully applied color transform!");
								} else {
									logger::warn("SetCxform failed");
								}
							} else {
								logger::warn("GetCxform failed - trying to invoke AS colorTransform instead");
								
								RE::GFxValue transform;
								if (animate.GetMember("transform", &transform)) {
									logger::info("Got transform object");
									
									RE::GFxValue colorTransformObj;
									RE::GFxValue args[8];
									args[0].SetNumber(1.0);
									args[1].SetNumber(0.7);
									args[2].SetNumber(0.7);
									args[3].SetNumber(1.0);
									args[4].SetNumber(25.0);
									args[5].SetNumber(0.0);
									args[6].SetNumber(0.0);
									args[7].SetNumber(0.0);
									
									movie->CreateObject(&colorTransformObj, "flash.geom.ColorTransform", args, 8);
									logger::info("Created ColorTransform object, type: {}", static_cast<int>(colorTransformObj.GetType()));
									
									if (transform.SetMember("colorTransform", colorTransformObj)) {
										logger::info("Successfully set colorTransform!");
									} else {
										logger::warn("Failed to set colorTransform member");
									}
								} else {
									logger::warn("Failed to get transform member");
								}
							}
						} else {
							logger::warn("Failed to get animate member");
						}
					} else {
						logger::warn("Failed to get TopPlayerInfo member");
					}
				} else {
					logger::warn("Failed to get _root");
				}
			}

			return func(menu, a_message);
		}

		static inline void post_hook()
		{
			logger::info("\t\t🪝Installed ShouldBackgroundClone hook.");
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

		stl::install_hook<StatsMenu_PostDisplay>();
	}
}
