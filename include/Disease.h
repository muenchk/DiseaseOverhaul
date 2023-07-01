#pragma once

#include <string>
#include <unordered_map>

typedef uint32_t EnumType;

typedef float Magnitude;

struct Diseases
{
	enum Disease : EnumType
	{
		// blight
		kAshWoeBlight = 0,
		kBlackHeartBlight = 1,
		kAshChancre = 2,
		kChantraxBlight = 3,
		// common
		kAstralVapors = 4,
		kAtaxia = 5,
		kBloodLung = 6,
		kBloodRot = 7,
		kBrainRot = 8,
		kCollywobbles = 9,
		kDroops = 10,
		kGreenspore = 11,
		kHelljoint = 12,
		kRedRage = 13,
		kRockjoint = 14,
		kSerpigniousDementia = 15,
		kSwampRot = 16,
		kWitchesPox = 17,
		// mild
		kBrownRot = 18,
		kChrondiasis = 19,
		kDampworm = 20,
		kFeeblelimb = 21,
		kRattles = 22,
		kRustchancre = 23,
		kShakes = 24,
		kSwampFever = 25,
		kWither = 26,
		kWitlessPox = 27,
		kYellowTick = 28,
		// fever
		kBoneBreakFever = 29,
		kBrainFever = 30,
		kTyphoidFever = 31,
		kWizardFever = 32,
		kYellowFever = 33,
		// dangerous
		kCalironsCurse = 34,
		kChills = 35,
		kRedDeath = 36,
		kStomachRot = 37,
		kWitbane = 38,
		kWoundRot = 39,
		// extreme
		kCholera = 40,
		kLeprosy = 41,
		kPlague = 42,

		// special
		kSanguinareVampirism = 43,

		kMaxValue = 44,
	};
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

enum class DiseaseStatus
{
	kProgressing = 0,
	kRegressing = 1,
	kInfection = 2,
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

struct PermanentModifiers
{
	enum PermanentModifier : EnumType
	{
		kNone = 0,
		kPotionOfCureCommonDisease = 1 << 0,
		kPotionOfStrongCureCommonDisease = 1 << 1,
		kPotionOfCureBlight = 1 << 2,
		kSpellOfCureBlight = 1 << 3,
		kPotionOfCureFever = 1 << 4,
		kPotionOfStrongCureFever = 1 << 5,
		kPotionOfCureCholera = 1 << 6,
		kPotionOfCureLeprosy = 1 << 7,
		kPotionOfCurePlague = 1 << 8,
		kPotionOfCureSanguinareVampirism = 1 << 9,
		kShrine = 1 << 10,
		kPotionOfCureStrongDisease = 1 << 11,

		kMaxValue = 12,
	};
};

typedef uint64_t Spreadingbase;

struct Spreading
{
	enum : Spreadingbase
	{
		kMinValue = 1,

		kNone = 0,
		kOnHitMelee = 1,        // When an actor is melee attacked by another
		kOnHitRanged = 2,       // when an actor is range attacked by another
		kOnHitH2H = 3,          // when an actor is attacked with fists by another
		kGetHitMelee = 4,       // when an actor hits another with a melee weapon
		kGetHitH2H = 5,         // when an actor hits another with fists
		kAir = 6,               // When Actors are in the same room / vicinity
		kParticle = 7,          // When actors speak to each other
		kIntenseCold = 8,       // when an actor is in an environment of intense cold (blizzard, on ice)
		kIntenseHeat = 9,       // when an actor is in an environment of intense heat (dessert)
		kInAshland = 10,        // when an actor is wandering ashlands (solstheim lower)
		kInSwamp =  11,     // when an actor is wandering a swamp (hjaalmarsch)
		kInDessert = 12,
		kInAshstorm = 13,  // when an actor is in an ashstorm
		kInSandstorm = 14, // when an actor is in a sandstorm
		kInBlizzard = 15,	// when an actor is in a blizzard
		kInRain = 16,   // when the weather is rainy
		kIsWindy = 17,       // when the weather is windy
		kIsStormy = 18,       // when the weather is stormy
		kIsCold = 19,     // when the weather is cold
		kIsHeat = 20,       // when the weather is hot
		kExtremeConditions = 21,
		kActionPhysical = 22,
		kActionMagical = 23,
		kActionVoice = 24,

		kMaxValue = 25,
	};
};

struct CellTypes
{
	enum CellType : EnumType
	{
		kNone = 0,
		// cell is dessert
		kDessert = 1 << 0,
		// cell has ice
		kIceland = 1 << 1,
		// cell is ashland
		kAshland = 1 << 2,
		// cell is swamp
		kSwamp = 1 << 3,
		// cell is snowy
		kSnow = 1 << 4,
		// cell is cold in winter
		kWinterCold = 1 << 5,
		// cell is hot in winter
		kWinterHot = 1 << 6,
		// cell is hot in summer
		kSummerHot = 1 << 7,
		// cell is cold in summer
		kSummerCold = 1 << 8,
		// always cold
		kCold = 1 << 9,
		// always hot
		kHeat = 1 << 10,
	};
};

struct CellDiseaseProperties
{
public:
	uint32_t formID = 0;
	EnumType type = CellTypes::kNone;

	CellDiseaseProperties()
	{
	}

	~CellDiseaseProperties()
	{
	}
};

struct TextureTypes
{
	enum TextureType : EnumType
	{
		kNone = 0,

		kDessert = 1 << 0,
		kIceland = 1 << 1,
		kAshland = 1 << 2,
		kSwamp = 1 << 3,
		kSnow = 1 << 4,

		kCold = 1 << 10,
		kHeat = 1 << 11,
	};
};

struct TextureDiseaseProperties
{
public:
	uint32_t formID = 0;
	EnumType type = TextureTypes::kNone;
};

struct WeatherTypes
{
	enum WeatherType : EnumType
	{
		kNone = 0,
		kAshstorm = 1 << 0,
		kSandstorm = 1 << 1,
		kBlizzard = 1 << 2,
		kRain = 1 << 3,
		kThunderstorm = 1 << 4,
		kSnow = 1 << 5,
		kHeatWave = 1 << 6,

		kCold = 1 << 10,
		kHeat = 1 << 11,
		kWindy = 1 << 12,
		kStormy = 1 << 13,
	};
};

struct WeatherDiseaseProperties
{
public:
	uint32_t formID = 0;
	EnumType type = WeatherTypes::kNone;
};

struct DiseaseEndEvents
{
	enum DiseaseEndEvent : EnumType
	{
		kNone = 0,					// nothing happens, i.e. infinity
		kDie100 = 1 << 0,			// dies 100%
		kDie75 = 1 << 1,			// dies with 75% probability
		kDie50 = 1 << 2,			// dies with 50% probability
		kDie25 = 1 << 3,			// dies with 25% probability
		kNoRegression = 1 << 4,		// the actor does not enter regression
		kRegression = 1 << 5, 		// the actor enters regression
	};
};

enum class DiseaseEffect
{
	kNone = 0,
};

struct DiseaseFlags
{
	enum DiseaseFlag : EnumType
	{
		kNone = 0,
		kAirSpread = 1 << 0,
		kParticleSpread = 1 << 1,
		kIntenseCold = 1 << 2,
		kIntenseHeat = 1 << 3,
		kInAshland = 1 << 4,
		kInSwamp = 1 << 5,
		kInDessert = 1 << 6,
		kInAshstorm = 1 << 7,
		kInSandstorm = 1 << 8,
		kInBlizzard = 1 << 9,
		kInRain = 1 << 10,
		kIsWindy = 1 << 11,
		kIsStormy = 1 << 12,
		kIsCold = 1 << 13,
		kIsHeat = 1 << 14,
		kExtremeConditions = 1 << 15,
		kActionPhysical = 1 << 16,
		kActionMagical = 1 << 17,
		kOnHitMelee = 1 << 18,
		kOnHitRanged = 1 << 19,
		kOnHitH2H = 1 << 20,
		kGetHitMelee = 1 << 21,
		kGetHitH2H = 1 << 22,
		kActionVoice = 1 << 23,
	};
};

class DiseaseStage
{
public:
	/// <summary>
	/// thresholds for the number of avancement points needed to progress to next stage
	/// </summary>
	float _advancementThreshold = 1;
	/// <summary>
	/// time a disease needs to progress before advancement
	/// </summary>
	float _advancementTime = 3;
	/// <summary>
	/// precalculated for the stage adjusted spreading values summing stage effects and base effects
	/// </summary>
	std::pair<float /*chance*/, float /*points*/> _spreading[Spreading::kMaxValue];
	/// <summary>
	/// infectivity in stage, adjusts _spreading during runtime
	/// </summary>
	Infectivity _infectivity = Infectivity::kLow;
	/// <summary>
	/// game spell that contains the diseases effect in the stage
	/// </summary>
	RE::SpellItem* effect = nullptr;
	/// <summary>
	/// Specifier of the stage
	/// </summary>
	std::string _specifier;

	std::vector<std::pair<DiseaseEffect, Magnitude>> _effects;

	/// <summary>
	/// flags reprensenting valid spread types for this disease stage
	/// </summary>
	EnumType flags = DiseaseFlags::kNone;

	void CalcFlags();
};

class Disease
{
private:
	/// <summary>
	/// Flags containing all possible spreading types for this disease
	/// </summary>
	EnumType flags = DiseaseFlags::kNone;

	/// <summary>
	/// flags representing valid infection types for this disease
	/// </summary>
	EnumType spreadflags = DiseaseFlags::kNone;

public:

	Disease();
	~Disease();

	/// <summary>
	/// Name of the disease
	/// </summary>
	std::string _name;
	/// <summary>
	/// number of stages the disease has
	/// </summary>
	int _numstages;
	/// <summary>
	/// dedicated stage for infection
	/// </summary>
	std::shared_ptr<DiseaseStage> _stageInfection;
	/// <summary>
	/// actual disease stages, according to numstages
	/// </summary>
	std::shared_ptr<DiseaseStage>* _stages;
	/// <summary>
	/// pairs of chance / point values that determine how an infection is spread // these are the base chances for infection / progression
	/// </summary>
	//std::pair<float /*chance*/, float /*points*/> _spreading[Spreading::kMaxValue];
	/// <summary>
	/// type of the disease
	/// </summary>
	DiseaseType _type;
	/// <summary>
	/// The disease
	/// </summary>
	Diseases::Disease _disease;


	/// <summary>
	/// the base points gained per tick while progressing / regressing
	/// </summary>
	float _baseProgressionPoints = 3.0f;
	/// <summary>
	/// the base infection points / hour that a disease in infection stage looses per hour
	/// </summary>
	float _baseInfectionReductionPoints = 1.0f;
	/// <summary>
	/// basic chance for an npc to be infected with the disease in percent
	/// </summary>
	int _baseInfectionChance = 5;  // in percent
	/// <summary>
	/// the permanent modifiers that are allowed for this disease
	/// </summary>
	uint32_t _validModifiers = PermanentModifiers::kNone;

	/// <summary>
	/// how long an actor gains immunity once they overcome the disease
	/// </summary>
	float immunityTime;


	// effects at the end of the last disease stage
	/// <summary>
	/// game spell that contains effects applied to an actor upon the end
	/// </summary>
	RE::SpellItem* endeffect = nullptr;
	/// <summary>
	/// events happening upon the end
	/// </summary>
	EnumType endevents = DiseaseEndEvents::kNone;

	/// <summary>
	/// effect that is permanently applied from the beginning of progression to the end of regression
	/// </summary>
	RE::SpellItem* permeffect = nullptr;

	/// <summary>
	/// returns [true] if the disease can spread via particles, [false] otherwise
	/// </summary>
	/// <returns></returns>
	bool ParticleSpread();
	/// <summary>
	/// returns [true] if the disease can spread via air, [false] otherwise
	/// </summary>
	/// <returns></returns>
	bool AirSpread();

	/// <summary>
	/// Calculates static disease flags.
	/// Use after each change to spreading values etc.
	/// </summary>
	void CalcFlags();

public:

	void SetName(std::string value) { _name = value; }
	std::string GetName() { return _name; }
};
