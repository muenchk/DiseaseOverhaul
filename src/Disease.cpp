#include "Disease.h"
#include "Logging.h"

bool Disease::ParticleSpread()
{
	return flags & DiseaseFlags::kParticleSpread;
}

bool Disease::AirSpread()
{
	return flags & DiseaseFlags::kAirSpread;
}

void Disease::CalcFlags()
{
	// check particle spread
	if (std::get<0>(_spreading[Spreading::kParticle]) != 0)
		flags = flags | DiseaseFlags::kParticleSpread;
	for (int i = 0; i < _numstages; i++) {
		if (std::get<0>(_stages[i]->_spreading[Spreading::kParticle]) != 0)
			flags = flags | DiseaseFlags::kParticleSpread;
	}
	// check air spread
	if (std::get<0>(_spreading[Spreading::kAir]) != 0)
		flags = flags | DiseaseFlags::kAirSpread;
	for (int i = 0; i < _numstages; i++) {
		if (std::get<0>(_stages[i]->_spreading[Spreading::kAir]) != 0)
			flags = flags | DiseaseFlags::kAirSpread;
	}
}
