#include "Random.h"

namespace Random
{
	std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<signed> rand100(1, 100);
	std::uniform_int_distribution<signed> rand3(1, 3);
}
