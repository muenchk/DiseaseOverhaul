#include "Disease.h"
#include "Logging.h"

void DiseaseStage::CalcFlags()
{
	// check particle spread
	if (std::get<0>(_spreading[Spreading::kParticle]) != 0) {
		flags = flags | DiseaseFlags::kParticleSpread;
	}
	// check air spread
	if (std::get<0>(_spreading[Spreading::kAir]) != 0) {
		flags = flags | DiseaseFlags::kAirSpread;
	}
	// check intense cold
	if (_spreading[Spreading::kIntenseCold].first != 0) {
		flags = flags | DiseaseFlags::kIntenseCold;
	}
	// check intense heat
	if (_spreading[Spreading::kIntenseHeat].first != 0) {
		flags = flags | DiseaseFlags::kIntenseHeat;
	}
	// check in ashland
	if (_spreading[Spreading::kInAshland].first != 0) {
		flags = flags | DiseaseFlags::kInAshland;
	}
	// check in swamp
	if (_spreading[Spreading::kInSwamp].first != 0) {
		flags = flags | DiseaseFlags::kInSwamp;
	}
	// check in dessert
	if (_spreading[Spreading::kInDessert].first != 0) {
		flags = flags | DiseaseFlags::kInDessert;
	}
	// check in ashstorm
	if (_spreading[Spreading::kInAshstorm].first != 0) {
		flags = flags | DiseaseFlags::kInAshstorm;
	}
	// check in sandstorm
	if (_spreading[Spreading::kInSandstorm].first != 0) {
		flags = flags | DiseaseFlags::kInSandstorm;
	}
	// check in blizzard
	if (_spreading[Spreading::kInBlizzard].first != 0) {
		flags = flags | DiseaseFlags::kInBlizzard;
	}
	// check in rain
	if (_spreading[Spreading::kInRain].first != 0) {
		flags = flags | DiseaseFlags::kInRain;
	}
	// check is windy
	if (_spreading[Spreading::kIsWindy].first != 0) {
		flags = flags | DiseaseFlags::kIsWindy;
	}
	// check is stormy
	if (_spreading[Spreading::kIsStormy].first != 0) {
		flags = flags | DiseaseFlags::kIsStormy;
	}
	// check is cold
	if (_spreading[Spreading::kIsCold].first != 0) {
		flags = flags | DiseaseFlags::kIsCold;
	}
	// check is heat
	if (_spreading[Spreading::kIsHeat].first != 0) {
		flags = flags | DiseaseFlags::kIsHeat;
	}
	// check extreme conditions
	if (_spreading[Spreading::kExtremeConditions].first != 0) {
		flags = flags | DiseaseFlags::kExtremeConditions;
	}
	// check in action physical
	if (_spreading[Spreading::kActionPhysical].first != 0) {
		flags = flags | DiseaseFlags::kActionPhysical;
	}
	// check in action magical
	if (_spreading[Spreading::kActionMagical].first != 0) {
		flags = flags | DiseaseFlags::kActionMagical;
	}
	// check in on hit melee
	if (_spreading[Spreading::kOnHitMelee].first != 0) {
		flags = flags | DiseaseFlags::kOnHitMelee;
	}
	// check in on hit ranged
	if (_spreading[Spreading::kOnHitRanged].first != 0) {
		flags = flags | DiseaseFlags::kOnHitRanged;
	}
	// check in on hit h2h
	if (_spreading[Spreading::kOnHitH2H].first != 0) {
		flags = flags | DiseaseFlags::kOnHitH2H;
	}
	// check in get hit melee
	if (_spreading[Spreading::kGetHitMelee].first != 0) {
		flags = flags | DiseaseFlags::kGetHitMelee;
	}
	// check in get hit h2h
	if (_spreading[Spreading::kGetHitH2H].first != 0) {
		flags = flags | DiseaseFlags::kGetHitH2H;
	}
}

Disease::Disease()
{
}

Disease::~Disease()
{
}

bool Disease::ParticleSpread()
{
	return spreadflags & DiseaseFlags::kParticleSpread;
}

bool Disease::AirSpread()
{
	return spreadflags & DiseaseFlags::kAirSpread;
}

void Disease::CalcFlags()
{
	// check particle spread
	if (std::get<0>(_stageInfection->_spreading[Spreading::kParticle]) != 0) {
		spreadflags = spreadflags | DiseaseFlags::kParticleSpread;
		flags = flags | DiseaseFlags::kParticleSpread;
	}
	for (int i = 0; i < _numstages; i++) {
		if (std::get<0>(_stages[i]->_spreading[Spreading::kParticle]) != 0)
			spreadflags = spreadflags | DiseaseFlags::kParticleSpread;
			flags = flags | DiseaseFlags::kParticleSpread;
	}
	// check air spread
	if (std::get<0>(_stageInfection->_spreading[Spreading::kAir]) != 0) {
		spreadflags = spreadflags | DiseaseFlags::kAirSpread;
		flags = flags | DiseaseFlags::kAirSpread;
	}
	for (int i = 0; i < _numstages; i++) {
		if (std::get<0>(_stages[i]->_spreading[Spreading::kAir]) != 0)
			spreadflags = spreadflags | DiseaseFlags::kAirSpread;
			flags = flags | DiseaseFlags::kAirSpread;
	}
	// check intense cold
	if (_stageInfection->_spreading[Spreading::kIntenseCold].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kIntenseCold;
		flags = flags | DiseaseFlags::kIntenseCold;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kIntenseCold].first != 0)
			flags = flags | DiseaseFlags::kIntenseCold;
	}
	// check intense heat
	if (_stageInfection->_spreading[Spreading::kIntenseHeat].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kIntenseHeat;
		flags = flags | DiseaseFlags::kIntenseHeat;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kIntenseHeat].first != 0)
			flags = flags | DiseaseFlags::kIntenseHeat;
	}
	// check in ashland
	if (_stageInfection->_spreading[Spreading::kInAshland].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInAshland;
		flags = flags | DiseaseFlags::kInAshland;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInAshland].first != 0)
			flags = flags | DiseaseFlags::kInAshland;
	}
	// check in swamp
	if (_stageInfection->_spreading[Spreading::kInSwamp].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInSwamp;
		flags = flags | DiseaseFlags::kInSwamp;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInSwamp].first != 0)
			flags = flags | DiseaseFlags::kInSwamp;
	}
	// check in dessert
	if (_stageInfection->_spreading[Spreading::kInDessert].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInDessert;
		flags = flags | DiseaseFlags::kInDessert;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInDessert].first != 0)
			flags = flags | DiseaseFlags::kInDessert;
	}
	// check in ashstorm
	if (_stageInfection->_spreading[Spreading::kInAshstorm].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInAshstorm;
		flags = flags | DiseaseFlags::kInAshstorm;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInAshstorm].first != 0)
			flags = flags | DiseaseFlags::kInAshstorm;
	}
	// check in sandstorm
	if (_stageInfection->_spreading[Spreading::kInSandstorm].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInSandstorm;
		flags = flags | DiseaseFlags::kInSandstorm;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInSandstorm].first != 0)
			flags = flags | DiseaseFlags::kInSandstorm;
	}
	// check in blizzard
	if (_stageInfection->_spreading[Spreading::kInBlizzard].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInBlizzard;
		flags = flags | DiseaseFlags::kInBlizzard;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInBlizzard].first != 0)
			flags = flags | DiseaseFlags::kInBlizzard;
	}
	// check in rain
	if (_stageInfection->_spreading[Spreading::kInRain].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kInRain;
		flags = flags | DiseaseFlags::kInRain;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kInRain].first != 0)
			flags = flags | DiseaseFlags::kInRain;
	}
	// check is windy
	if (_stageInfection->_spreading[Spreading::kIsWindy].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kIsWindy;
		flags = flags | DiseaseFlags::kIsWindy;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kIsWindy].first != 0)
			flags = flags | DiseaseFlags::kIsWindy;
	}
	// check is stormy
	if (_stageInfection->_spreading[Spreading::kIsStormy].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kIsStormy;
		flags = flags | DiseaseFlags::kIsStormy;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kIsStormy].first != 0)
			flags = flags | DiseaseFlags::kIsStormy;
	}
	// check is cold
	if (_stageInfection->_spreading[Spreading::kIsCold].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kIsCold;
		flags = flags | DiseaseFlags::kIsCold;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kIsCold].first != 0)
			flags = flags | DiseaseFlags::kIsCold;
	}
	// check is heat
	if (_stageInfection->_spreading[Spreading::kIsHeat].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kIsHeat;
		flags = flags | DiseaseFlags::kIsHeat;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kIsHeat].first != 0)
			flags = flags | DiseaseFlags::kIsHeat;
	}
	// check extreme conditions
	if (_stageInfection->_spreading[Spreading::kExtremeConditions].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kExtremeConditions;
		flags = flags | DiseaseFlags::kExtremeConditions;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kExtremeConditions].first != 0)
			flags = flags | DiseaseFlags::kExtremeConditions;
	}
	// check in action physical
	if (_stageInfection->_spreading[Spreading::kActionPhysical].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kActionPhysical;
		flags = flags | DiseaseFlags::kActionPhysical;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kActionPhysical].first != 0)
			flags = flags | DiseaseFlags::kActionPhysical;
	}
	// check in action magical
	if (_stageInfection->_spreading[Spreading::kActionMagical].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kActionMagical;
		flags = flags | DiseaseFlags::kActionMagical;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kActionMagical].first != 0)
			flags = flags | DiseaseFlags::kActionMagical;
	}
	// check in on hit melee
	if (_stageInfection->_spreading[Spreading::kOnHitMelee].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kOnHitMelee;
		flags = flags | DiseaseFlags::kOnHitMelee;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kOnHitMelee].first != 0)
			flags = flags | DiseaseFlags::kOnHitMelee;
	}
	// check in on hit ranged
	if (_stageInfection->_spreading[Spreading::kOnHitRanged].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kOnHitRanged;
		flags = flags | DiseaseFlags::kOnHitRanged;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kOnHitRanged].first != 0)
			flags = flags | DiseaseFlags::kOnHitRanged;
	}
	// check in on hit h2h
	if (_stageInfection->_spreading[Spreading::kOnHitH2H].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kOnHitH2H;
		flags = flags | DiseaseFlags::kOnHitH2H;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kOnHitH2H].first != 0)
			flags = flags | DiseaseFlags::kOnHitH2H;
	}
	// check in get hit melee
	if (_stageInfection->_spreading[Spreading::kGetHitMelee].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kGetHitMelee;
		flags = flags | DiseaseFlags::kGetHitMelee;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kGetHitMelee].first != 0)
			flags = flags | DiseaseFlags::kGetHitMelee;
	}
	// check in get hit h2h
	if (_stageInfection->_spreading[Spreading::kGetHitH2H].first != 0) {
		spreadflags = spreadflags | DiseaseFlags::kGetHitH2H;
		flags = flags | DiseaseFlags::kGetHitH2H;
	}
	for (int i = 0; i < _numstages; i++) {
		if (_stages[i]->_spreading[Spreading::kGetHitH2H].first != 0)
			flags = flags | DiseaseFlags::kGetHitH2H;
	}
}
