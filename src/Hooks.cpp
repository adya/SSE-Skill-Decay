#include "Hooks.h"
#include "DecayTracker.h"
#include "Hooking.h"
#include "Options.h"

namespace Decay
{
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
	}
}
