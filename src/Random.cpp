#include "Random.h"

namespace Random
{
	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
	/// <summary>
	/// trims random numbers to 1 to 100
	/// </summary>
	std::uniform_int_distribution<signed> rand100(1, 100);
	/// <summary>
	/// trims random numbers to 1 to 1000
	/// </summary>
	std::uniform_int_distribution<signed> rand1000(1, 1000);
	/// <summary>
	/// trims random numbers to 1 to 10000
	/// </summary>
	std::uniform_int_distribution<signed> rand10000(1, 10000);
	/// <summary>
	/// trims random numbers to 1 to 3
	/// </summary>
	std::uniform_int_distribution<signed> rand3(1, 3);

}
