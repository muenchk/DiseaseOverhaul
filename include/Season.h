#pragma once

namespace Season
{

	enum class Season
	{
		kSpring,
		kSummer,
		kWinter,
		kAutumn
	};

	Season GetSeason(RE::Calendar::Month month);

	Season GetCurrentSeason();
}
