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

#pragma region Events
	/// <summary>
	/// Number of times the TESHitEvent has been fired
	/// </summary>
	static inline long Events_TESHitEvent = 0;
	/// <summary>
	/// Number of times the TESCombatEvent has been fired
	/// </summary>
	static inline long Events_TESCombatEvent = 0;
	/// <summary>
	/// Number of times the TESDeathEvent has been fired
	/// </summary>
	static inline long Events_TESDeathEvent = 0;
	/// <summary>
	/// Number of times the BGSActorCellEvent has been fired
	/// </summary>
	static inline long Events_BGSActorCellEvent = 0;
	/// <summary>
	/// Number of times the TESCellAttachDetachEvent has been fired
	/// </summary>
	static inline long Events_TESCellAttachDetachEvent = 0;
	/// <summary>
	/// Number of times the TESEquipEvent has been fired
	/// </summary>
	static inline long Events_TESEquipEvent = 0;
	/// <summary>
	/// Number of times the TESFormDeleteEvent has been fired
	/// </summary>
	static inline long Events_TESFormDeleteEvent = 0;
	/// <summary>
	/// Number of times the TESContainerChangedEvent has been fired
	/// </summary>
	static inline long Events_TESContainerChangedEvent = 0;
#pragma endregion
	
#pragma region System
	/// <summary>
	/// Number of bytes written during the last save
	/// </summary>
	static inline long Storage_BytesWrittenLast = 0;
	/// <summary>
	/// Number of bytes read during the last load
	/// </summary>
	static inline long Storage_BytesReadLast = 0;
	/// <summary>
	/// Number of actors save during the last save
	/// </summary>
	static inline long Storage_ActorsSavedLast = 0;
	/// <summary>
	/// Number of actors read during the last load
	/// </summary>
	static inline long Storage_ActorsReadLast = 0;
#pragma endregion
};
