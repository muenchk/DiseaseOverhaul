#pragma once

#include "Data.h"

class World
{
private:
	/// <summary>
	/// reference to the players current cell
	/// </summary>
	RE::TESObjectCELL* playerCell = nullptr;
	/// <summary>
	/// reference to the players current world space
	/// </summary>
	RE::TESWorldSpace* playerWorldspace = nullptr;
	/// <summary>
	/// cell id of the cell the player is in
	/// </summary>
	RE::FormID playerCellID = 0;
	/// <summary>
	/// reference to the player
	/// </summary>
	RE::Actor* playerRef = nullptr;


	Data* data = nullptr;

public:
	void Init();

	/// <summary>
	/// returns singleton to the World
	/// </summary>
	/// <returns></returns>
	static World* GetSingleton();

	/// <summary>
	/// Resets world data
	/// </summary>
	void Reset();

	/// <summary>
	/// Player changes the cell
	/// </summary>
	/// <param name="newcell"></param>
	void PlayerChangeCell();
	/// <summary>
	/// Player changes the cell
	/// </summary>
	/// <param name="newcell"></param>
	void PlayerChangeCell(RE::TESObjectCELL* cell);
	/// <summary>
	/// Player changes the cell
	/// </summary>
	/// <param name="newcell"></param>
	void PlayerChangeCell(RE::FormID cellid);
};
