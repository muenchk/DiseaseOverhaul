#include "DataStorage.h"
#include "Logging.h"
#include "Game.h"
#include "Events.h"
#include "Data.h"
#include "Utility.h"
#include "Stats.h"

namespace Storage
{
	bool processing;
	bool CanProcess() { return processing; }
#define EvalProcessing \
	if (processing == false) \
		return;
#define EvalProcessingBool       \
	if (processing == false) \
		return false;
	

	static Data* data = nullptr;

	void ReadData(SKSE::SerializationInterface* a_intfc);
	void WriteData(SKSE::SerializationInterface* a_intfc);
	void RevertData();

	/// <summary>
	/// Callback executed on saving
	/// saves all global data
	/// </summary>
	/// <param name=""></param>
	void SaveGameCallback(SKSE::SerializationInterface* a_intfc)
	{
		LOG_1("{}[DataStorage] [SaveGameCallback]");
		WriteData(a_intfc);
		LOG_1("{}[DataStorage] [SaveGameCallback] end");
	}

	/// <summary>
	/// Callback executed on loading
	/// loads all global data
	/// </summary>
	/// <param name=""></param>
	void LoadGameCallback(SKSE::SerializationInterface* a_intfc)
	{
		LOG_1("{}[DataStorage] [LoadGameCallback]");
		ReadData(a_intfc);

		processing = true;
		LOG_1("{}[DataStorage] [LoadGameCallback] end");
	}

	/// <summary>
	/// Callback executed on reverting to older savegame
	/// deletes all active data and disables processing until load event
	/// </summary>
	/// <param name=""></param>
	void RevertGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[DataStorage] [RevertGameCallback]");
		processing = false;
		RevertData();
		LOG_1("{}[DataStorage] [RevertGameCallback] end");
	}

	void Register()
	{
		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000010, LoadGameCallback);
		loginfo("[DataStorage] [Register] Registered for LoadGameCallback");
		Game::SaveLoad::GetSingleton()->RegisterForRevertCallback(0xFF000020, RevertGameCallback);
		loginfo("[DataStorage] [Register] Registered for RevertGameCallback");
		Game::SaveLoad::GetSingleton()->RegisterForSaveCallback(0xFF000030, SaveGameCallback);
		loginfo("[DataStorage] [Register] Registered for SaveGameCallback");
		data = Data::GetSingleton();
		if (data == nullptr)
			logcritical("[DataStorage] [Register] Cannot access data storage");
	}

	void ReadData(SKSE::SerializationInterface* a_intfc)
	{
		bool preproc = Events::Main::LockProcessing();

		// total number of bytes read
		long size = 0;

		LOG_1("{}[DataStorage] [ReadData] Beginning data load...");

		uint32_t type = 0;
		uint32_t version = 0;
		uint32_t length = 0;

		// for actor info map
		int accounter = 0;
		int acfcounter = 0;
		int acdcounter = 0;

		loginfo("[DataStorage] [ReadData] Beginning data load...");
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			loginfo("[DataStorage] found record with type {} and length {}", type, length);
			switch (type) {
			case 'ACIF':  // ActorInfo
				size += data->ReadActorInfoMap(a_intfc, length, accounter, acdcounter, acfcounter);
				break;
			case 'DAID':  // Deleted Actor
				size += data->ReadDeletedActors(a_intfc, length);
				break;
			case 'EDID':  // Dead Actor
				size += Events::Main::ReadDeadActors(a_intfc, length);
				break;
			}
		}

		Stats::Storage_BytesReadLast = size;
		Stats::Storage_ActorsReadLast = accounter;
		LOG1_1("{}[Data] [ReadActorInfoMap] Read {} ActorInfos", accounter);
		LOG1_1("{}[Data] [ReadActorInfoMap] Read {} dead, deleted or invalid ActorInfos", acdcounter);
		LOG1_1("{}[Data] [ReadActorInfoMap] Failed to read {} ActorInfos", acfcounter);

		loginfo("[DataStorage] [ReadData] Finished loading data");
		if (preproc) {  // if processing was enabled before locking
			Events::Main::UnlockProcessing();
			loginfo("[DataStorage] [ReadData] Enable processing");
		}
	}

	void WriteData(SKSE::SerializationInterface* a_intfc)
	{
		bool preproc = Events::Main::LockProcessing();

		// total number of bytes written
		long size = 0;

		loginfo("[DataStorage] [WriteData] Beginning to write data...");

		data->CleanActorInfos();
		size += data->SaveActorInfoMap(a_intfc);
		size += data->SaveDeletedActors(a_intfc);
		size += Events::Main::SaveDeadActors(a_intfc);

		loginfo("[DataStorage] [WriteData] Finished writing data");

		if (preproc) {  // if processing was enabled before locking
			Events::Main::UnlockProcessing();
			loginfo("[DataStorage] [WriteData] Enable processing");
		}
		Stats::Storage_BytesWrittenLast = size;
	}

	void RevertData()
	{
		bool preproc = Events::Main::LockProcessing();

		LOG_1("{}[DataStorage] [RevertData] Reverting ActorInfo");
		data->DeleteActorInfoMap();

		LOG_1("{}[DataStorage] [RevertData] Finished reverting");

		if (preproc)  // if processing was enabled before locking
			Events::Main::UnlockProcessing();
	}
}
