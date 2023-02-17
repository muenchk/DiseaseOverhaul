#pragma once

/// <summary>
/// class that contains statics that track the execution of specific functions
/// </summary>
class Stats
{
public:
	static inline int MainHandler_Run = 0;
	static inline size_t MainHandler_ActorsHandled = 0;
	static inline uint64_t MainHandler_TimeTaken = 0;
	static inline size_t MainHandler_ActorsHandledTotal = 0;
	static inline std::vector<uint64_t> MainHandler_Particles_Times;
	static inline int DiseaseStats_ProgressDisease = 0;
};
