#pragma once

#include <string>
#include <unordered_map>

enum class Diseases
{

};

enum class DiseaseType
{
	kNone = 0,
	kMild = 1 << 0,
	kCommon = 1 << 1,
	kDangerous = 1 << 2,
	kFever = 1 << 3,
	kExtreme = 1 << 4,
	kBlight = 1 << 5,
};

enum class Infectivity
{
	kNone = 0,
	kLow = 1 << 0,
	kNormal = 1 << 1,
	kHigher = 1 << 2,
	kHigh = 1 << 3,
	kVeryHigh = 1 << 4,
};

class Disease
{
	typedef uint64_t Spreadingbase;

	struct Spreading
	{
		enum : Spreadingbase
		{
			kNone = 0,
			kOnHitMeele = 1 << 0,  // When an Actor hits another
			kOnHitRanged = 1 << 1,
			kOnHitH2H = 1 << 2,
			kAir = 1 << 3,       // When Actors are in the same room / vicinity
			kParticle = 1 << 4,  // When actors speak to each other
			kIntenseCold = 1 << 5,
			kIntenseHeat = 1 << 6,
		};
	};

	struct CellProperties
	{
	public:
		Spreadingbase SpreadingAdjusts[32];
		uint32_t FormID;

		CellProperties()
		{
		}

		~CellProperties()
		{
		}
	};

	/// <summary>
	/// Name of the disease
	/// </summary>
	std::string _name;
	/// <summary>
	/// number of stages the disease has
	/// </summary>
	int _stages;
	/// <summary>
	/// thresholds for the number of avancement points needed to progress to next stage
	/// </summary>
	std::vector<int> _advancementThresholds;
	/// <summary>
	/// chances to get an advancement point for a tick
	/// </summary>
	std::vector<int> _advancementChances;
	/// <summary>
	/// infectivity per stage, adjusts chancesSpreading during runtime
	/// </summary>
	std::vector<Infectivity> _infectivity;
	/// <summary>
	/// type of the disease
	/// </summary>
	DiseaseType _type;

	/// <summary>
	/// base chances for a spread based on the spread types
	/// </summary>
	Spreadingbase _chancesSpreading[32];


public:

	static inline std::unordered_map<uint32_t, CellProperties*> CellMap;

	static Spreadingbase CalcPossibleInfectionConditions(uint32_t cellid);

	void SetName(std::string value) { _name = value; }
	std::string GetName() { return _name; }
};

class DiseaseInfo
{
	Diseases disease;
	int stage;
	float advPoints;
	int advThreshold;
};
