#include "Season.h"


namespace Season
{
	Season GetSeason(RE::Calendar::Month month)
	{
		switch (month) {
		case RE::Calendar::Month::kMorningStar:
			return Season::kWinter;
		case RE::Calendar::Month::kSunsDawn:
			return Season::kWinter;
		case RE::Calendar::Month::kFirstSeed:
			return Season::kSpring;
		case RE::Calendar::Month::kRainsHand:
			return Season::kSpring;
		case RE::Calendar::Month::kSecondSeed:
			return Season::kSpring;
		case RE::Calendar::Month::kMidyear:
			return Season::kSummer;
		case RE::Calendar::Month::kSunsHeight:
			return Season::kSummer;
		case RE::Calendar::Month::kLastSeed:
			return Season::kSummer;
		case RE::Calendar::Month::kHearthfire:
			return Season::kAutumn;
		case RE::Calendar::Month::kFrostfall:
			return Season::kAutumn;
		case RE::Calendar::Month::kSunsDusk:
			return Season::kAutumn;
		case RE::Calendar::Month::kEveningStar:
			return Season::kWinter;
		}
		return Season::kSpring;
	}

	Season GetCurrentSeason()
	{
		RE::Calendar::Month month = static_cast<RE::Calendar::Month>(RE::Calendar::GetSingleton()->GetMonth());
		switch (month) {
		case RE::Calendar::Month::kMorningStar:
			return Season::kWinter;
		case RE::Calendar::Month::kSunsDawn:
			return Season::kWinter;
		case RE::Calendar::Month::kFirstSeed:
			return Season::kSpring;
		case RE::Calendar::Month::kRainsHand:
			return Season::kSpring;
		case RE::Calendar::Month::kSecondSeed:
			return Season::kSpring;
		case RE::Calendar::Month::kMidyear:
			return Season::kSummer;
		case RE::Calendar::Month::kSunsHeight:
			return Season::kSummer;
		case RE::Calendar::Month::kLastSeed:
			return Season::kSummer;
		case RE::Calendar::Month::kHearthfire:
			return Season::kAutumn;
		case RE::Calendar::Month::kFrostfall:
			return Season::kAutumn;
		case RE::Calendar::Month::kSunsDusk:
			return Season::kAutumn;
		case RE::Calendar::Month::kEveningStar:
			return Season::kWinter;
		}
		return Season::kSpring;
	}
}
