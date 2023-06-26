#pragma once


namespace Random
{
	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	extern std::mt19937 rand;
	/// <summary>
	/// trims random numbers to 1 to 100
	/// </summary>
	extern std::uniform_int_distribution<signed> rand100;
	/// <summary>
	/// trims random numbers to 1 to 1000
	/// </summary>
	extern std::uniform_int_distribution<signed> rand1000;
	/// <summary>
	/// trims random numbers to 1 to 10000
	/// </summary>
	extern std::uniform_int_distribution<signed> rand10000;
	/// <summary>
	/// trims random numbers to 1 to 3
	/// </summary>
	extern std::uniform_int_distribution<signed> rand3;
}
