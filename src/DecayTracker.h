#pragma once


class DecayTracker
{
public:
	static DecayTracker& GetInstance()
	{
		static DecayTracker instance;
		return instance;
	}

	void AdvanceTime(RE::Calendar* calendar);
};
