#pragma

#include "Disease.h"

class DiseaseInfo
{
public:
	/// <summary>
	/// the disease information is contained about
	/// </summary>
	Diseases::Disease disease;
	/// <summary>
	/// the current stage of the disease
	/// </summary>
	int stage = 0;
	/// <summary>
	/// the current number of advancement points towards the next higher / lower stage
	/// </summary>
	float advPoints = 0.0f;
	/// <summary>
	/// defines the timepoint in gamedayspassed when an actor may advance to the next stage at earliest
	/// all excessive points will be carried over and may cause jumps in stages
	/// </summary>
	float earliestAdvancement = 0.0f;
	/// <summary>
	/// whether the disease is advancing, regressing or in the stage of infection
	/// </summary>
	DiseaseStatus status = DiseaseStatus::kInfection;

	/// <summary>
	/// the gamedays until the actor is immune to the disease
	/// this happens after a disease was overcome
	/// until this time, the actor may not gain any infection points
	/// </summary>
	float immuneUntil = 0.0f;

	/// <summary>
	/// sum of permanent modifiers applied to an actor.
	/// such as potions, shrines, spells
	/// </summary>
	float permanentModifiersPoints = 0.0f;
	/// <summary>
	/// the modifiers already applied to the actor
	/// </summary>
	EnumType permanentModifiers = PermanentModifiers::kNone;

private:
	static inline const uint32_t version = 0x01000001;
	static inline const uint32_t valid_ver1 =   0x53fed832;
	static inline const uint32_t invalid_ver1 = 0x748392ed;

public:
	static void WriteData(std::shared_ptr<DiseaseInfo> dinfo, unsigned char* buffer, int offset);
	static std::shared_ptr<DiseaseInfo> ReadData(unsigned char* buffer, int offset);
	static int GetDataSize(std::shared_ptr<DiseaseInfo> dinfo);
};
