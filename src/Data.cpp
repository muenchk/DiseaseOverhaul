#include "ActorManipulation.h"
#include "BufferOperations.h"
#include "Data.h"
#include "Events.h"
#include "Logging.h"
#include "UtilityAlch.h"
#include "Season.h"
#include "Stats.h"




void Data::Init()
{
	datahandler = RE::TESDataHandler::GetSingleton();
}

Data* Data::GetSingleton()
{
	static Data singleton;
	return std::addressof(singleton);
}

std::binary_semaphore lockdata{ 1 };

std::shared_ptr<ActorInfo> Data::CreateActorInfo(RE::Actor* actor)
{
	std::shared_ptr<ActorInfo> acinfo = std::make_shared<ActorInfo>(actor);
	if (acinfo->IsValid()) {
		validActors.insert(acinfo->GetFormID());
		actorinfoMap.insert_or_assign(acinfo->GetFormID(), acinfo);
		Distribution::CalcRule(acinfo);
	}
	LOG1_4("{}[Data] [CreateActorInfo] {}", Utility::PrintForm(acinfo));
	return acinfo;
}

std::shared_ptr<ActorInfo> Data::CreateActorInfoNew()
{
	LOG_4("{}[Data] [CreateActorInfoNew]");
	return std::make_shared<ActorInfo>();
}

std::shared_ptr<ActorInfo> Data::CreateActorInfoEmpty()
{
	std::shared_ptr<ActorInfo> empty = std::make_shared<ActorInfo>(true);  // blocks resetting this instance
	empty->SetInvalid();
	empty->SetDead();
	LOG_4("{}[Data] [CreateActorInfoEmpty]");
	return empty;
}

void Data::RegisterActorInfo(std::shared_ptr<ActorInfo> acinfo)
{
	if (acinfo->IsValid()) {
		validActors.insert(acinfo->GetFormID());
		actorinfoMap.insert_or_assign(acinfo->GetFormID(), acinfo);
		Distribution::CalcRule(acinfo);
	}
}

void Data::DeleteActorInfo(RE::FormID formid)
{
	validActors.erase(formid);
	actorinfoMap.erase(formid);
}

std::shared_ptr<ActorInfo> Data::FindActor(RE::Actor* actor)
{
	if (Utility::ValidateActor(actor) == false)
		return CreateActorInfoEmpty();  // worst case, should not be necessary here
	lockdata.acquire();
	// check whether the actor was deleted before
	if (deletedActors.contains(actor->GetFormID())) {
		// create dummy ActorInfo
		std::shared_ptr<ActorInfo> acinfo = CreateActorInfoEmpty();
		lockdata.release();
		return acinfo;
	}
	// if there already is an valid object for the actor return it
	if (validActors.contains(actor->GetFormID())) {
		// find the object
		auto itr = actorinfoMap.find(actor->GetFormID());
		if (itr != actorinfoMap.end()) {
			// found it, check it for validity and dead status
			if (itr->second->IsValid()) {
				lockdata.release();
				return itr->second;
			}
			// else go to next point
		}
	}
	// not found or not valid
	// create new object. This will override existing objects for a formid as long as they are invalid or deleted
	// as checked above
	std::shared_ptr<ActorInfo> acinfo = CreateActorInfo(actor);
	lockdata.release();
	return acinfo;
}

std::shared_ptr<ActorInfo> Data::FindActorExisting(RE::Actor* actor)
{
	if (Utility::ValidateActor(actor) == false)
		return CreateActorInfoEmpty();  // worst case, should not be necessary here
	lockdata.acquire();
	// check whether the actor was deleted before
	if (deletedActors.contains(actor->GetFormID())) {
		// create dummy ActorInfo
		std::shared_ptr<ActorInfo> acinfo = CreateActorInfoEmpty();
		lockdata.release();
		return acinfo;
	}
	// if there already is an valid object for the actor return it
	if (validActors.contains(actor->GetFormID())) {
		// find the object
		auto itr = actorinfoMap.find(actor->GetFormID());
		if (itr != actorinfoMap.end()) {
			// found it, check it for validity and deleted status
			if (itr->second->IsValid()) {
				lockdata.release();
				return itr->second;
			}
			// else go to next point
		}
	}
	// if there is no valid actorinfo object present, return an empty one and do not create a new one
	std::shared_ptr<ActorInfo> acinfo = CreateActorInfoEmpty();
	lockdata.release();
	return acinfo;
}

std::shared_ptr<ActorInfo> Data::FindActor(RE::FormID actorid)
{
	RE::Actor* actor = RE::TESForm::LookupByID<RE::Actor>(actorid);
	if (actor)
		return FindActor(actor);
	else {
		// create dummy ActorInfo
		std::shared_ptr<ActorInfo> acinfo = CreateActorInfoEmpty();
		return acinfo;
	}
}

bool Data::UpdateActorInfo(std::shared_ptr<ActorInfo> acinfo)
{
	acinfo->Update();
	// if actorinfo is marked deleted, or expired, delete it
	if (acinfo->IsExpired()) {
		validActors.erase(acinfo->GetFormID());
		DeleteActorInfo(acinfo->GetFormID());
		return false;
	}
	// if it is invalid, don't delete it just yet, we may need it again
	if (!acinfo->IsValid()) {
		return false;
	}
	return true;
}

void Data::DeleteActor(RE::FormID actorid)
{
	// if we delete the object itself, we may have problems when someone tries to access the deleted object
	// so just flag it as invalid and move it to the list of empty actor refs
	lockdata.acquire();
	auto itr = actorinfoMap.find(actorid);
	if (itr != actorinfoMap.end()) {
		std::shared_ptr<ActorInfo> acinfo = itr->second;
		acinfo->SetInvalid();
		acinfo->SetDead();
		// save deleted actors, so we do not create new actorinfos for these
		deletedActors.insert(actorid);
		DeleteActorInfo(actorid);
	}
	lockdata.release();
}

void Data::CleanActorInfos()
{
	lockdata.acquire();
	std::vector<uint32_t> keys;
	auto proc = [&keys](uint32_t key, std::shared_ptr<ActorInfo>& acinfo) {
		acinfo->Update();
		if (acinfo->IsExpired())
			keys.push_back(key);
	};
	for (auto& [key, val] : actorinfoMap) {
		proc(key, val);
	}
	for (auto& key : keys) {
		actorinfoMap.erase(key);
	}
	lockdata.release();
}

void Data::ResetActorInfoMap()
{
	lockdata.acquire();
	auto itr = actorinfoMap.begin();
	while (itr != actorinfoMap.end()) {
		itr->second->citems.Reset();
		itr++;
	}
	lockdata.release();
}

long Data::SaveDeletedActors(SKSE::SerializationInterface* a_intfc)
{
	lockdata.acquire();
	LOG_1("{}[Data] [SaveDeletedActors] Writing Deleted Actors");
	LOG1_1("{}[Data] [SaveDeletedActors] {} actors to write", deletedActors.size());

	long size = 0;
	long successfulwritten = 0;

	for (auto& actorid : deletedActors) {
		uint32_t formid = Utility::Mods::GetIndexLessFormID(actorid);
		std::string pluginname = Utility::Mods::GetPluginNameFromID(actorid);
		if (a_intfc->OpenRecord('DODA', 0)) {
			// get entry length
			int length = 4 + Buffer::CalcStringLength(pluginname);
			// save written bytes number
			size += length;
			// create buffer
			unsigned char* buffer = new unsigned char[length + 1];
			if (buffer == nullptr) {
				logwarn("[DataStorage] [WriteData] failed to write Deleted Actor record: buffer null");
				continue;
			}
			// fill buffer
			int offset = 0;
			Buffer::Write(actorid, buffer, offset);
			Buffer::Write(pluginname, buffer, offset);
			// write record
			a_intfc->WriteRecordData(buffer, length);
			delete[] buffer;
			successfulwritten++;
		}
	}
	LOG_1("{}[Data] [SaveDeletedActors] Writing Deleted Actors finished.");
	lockdata.release();

	return size;
}

long Data::ReadDeletedActors(SKSE::SerializationInterface* a_intfc, uint32_t length)
{
	long size = 0;
	// get map lock
	lockdata.acquire();

	LOG_1("{}[Data] [ReadDeletedActors] Reading Deleted Actor...");
	unsigned char* buffer = new unsigned char[length];
	a_intfc->ReadRecordData(buffer, length);
	if (length >= 12) {
		int offset = 0;
		uint32_t formid = Buffer::ReadUInt32(buffer, offset);
		std::string pluginname = Buffer::ReadString(buffer, offset);
		RE::TESForm* form = RE::TESDataHandler::GetSingleton()->LookupForm(formid, pluginname);
		if (form)
			deletedActors.insert(form->GetFormID());
	}
	delete[] buffer;
	// release lock
	lockdata.release();

	return size;
}

long Data::SaveActorInfoMap(SKSE::SerializationInterface* a_intfc)
{
	lockdata.acquire();
	LOG_1("{}[Data] [SaveActorInfoMap] Writing ActorInfo");
	LOG1_1("{}[Data] [SaveActorInfoMap] {} records to write", actorinfoMap.size());

	long size = 0;
	long successfulwritten = 0;

	// transform second values of map into a vector and operate on the vector instead
	std::vector<std::weak_ptr<ActorInfo>> acvec;
	// write map data to vector
	std::transform(
		actorinfoMap.begin(),
		actorinfoMap.end(),
		std::back_inserter(acvec),
		[](auto& kv) { return kv.second; });
	// iterate over the vector entries
	for (int i = 0; i < acvec.size(); i++) {
		//LOG1_3("{}[Data] [SaveActorInfoMap] {} Writing ActorInfo if begin", i);
		if (std::shared_ptr<ActorInfo> acinfo = acvec[i].lock()) {
			if (acinfo->IsValid()) {
				//LOG1_3("{}[Data] [SaveActorInfoMap] {} Writing ActorInfo if valid", i);
				if (acinfo->GetActor() != nullptr) {
					//LOG1_3("{}[Data] [SaveActorInfoMap] {} Writing ActorInfo if actor not null", i);
					if ((acinfo->GetFormFlags() & RE::TESForm::RecordFlags::kDeleted) == 0) {
						//LOG1_3("{}[Data] [SaveActorInfoMap] {} Writing ActorInfo if actor not deleted", i);
						if (acinfo->GetActor()->GetFormID() != 0) {
							//LOG1_3("{}[Data] [SaveActorInfoMap] {} Writing ActorInfo if id not 0", i);
							if (acinfo->IsDead()) {
								loginfo("[Data] [SaveActorInfoMap] {} Cannot write {}: actor is dead", i, acinfo->GetName());
							} else {
								//LOG1_3("{}[Data] [SaveActorInfoMap] {} Writing ActorInfo if actor not dead", i);
								LOG2_3("{}[Data] [SaveActorInfoMap] Writing {}, number {}", acinfo->GetName(), i);
								if (a_intfc->OpenRecord('DOAC', ActorInfo::GetVersion())) {
									//LOG_3("{}[Data] [SaveActorInfoMap] \tget data size");
									// get entry length
									int length = acinfo->GetDataSize();
									if (length == 0) {
										logwarn("[Data] [WriteData] failed to write ActorInfo record: record length 0");
										continue;
									}
									// save written bytes nu´mber
									size += length;
									//LOG_3("{}[Data] [SaveActorInfoMap] \tcreate buffer");
									// create buffer
									unsigned char* buffer = new unsigned char[length + 1];
									if (buffer == nullptr) {
										logwarn("[DataStorage] [WriteData] failed to write ActorInfo record: buffer null");
										continue;
									}
									//LOG_3("{}[Data] [SaveActorInfoMap] \twrite data to buffer");
									// fill buffer
									if (acinfo->WriteData(buffer, 0) == false) {
										logwarn("[Data] [SaveActorInfoMap] failed to write ActorInfo record: Writing of ActorInfo failed");
										delete[] buffer;
										continue;
									}
									//LOG_3("{}[Data] [SaveActorInfoMap] \twrite record");
									// write record
									a_intfc->WriteRecordData(buffer, length);
									//LOG_3("{}[Data] [SaveActorInfoMap] \tDelete buffer");
									delete[] buffer;
									successfulwritten++;
								} else if (acinfo == nullptr) {
									logwarn("[Data] [SaveActorInfoMap] failed to write ActorInfo record: ActorInfo invalidated");
								} else if (acinfo->GetActor() == nullptr) {
									logwarn("[Data] [SaveActorInfoMap] failed to write ActorInfo record: actor invalidated");
								} else if (acinfo->GetActor()->GetFormID() == 0) {
									logwarn("[Data] [SaveActorInfoMap] failed to write ActorInfo record: formid invalid");
								} else if (acinfo->IsDead() == true) {
									logwarn("[Data] [SaveActorInfoMap] failed to write ActorInfo record: actor died");
								} else {
									logwarn("[Data] [SaveActorInfoMap] failed to write ActorInfo record: unknown reason");
								}
							}
						} else
							logwarn("[Data] [SaveActorInfoMap] {} Cannot write {}: formid is 0", i, acinfo->GetName());
					} else
						logwarn("[Data] [SaveActorInfoMap] {} Cannot write {}: actor deleted", i, acinfo->GetName());
				} else
					logwarn("[Data] [SaveActorInfoMap] {} Cannot write {}: actor null", i, acinfo->GetName());
			} else
				logwarn("[Data] [SaveActorInfoMap] {} Cannot write {}: ActorInfo invalid", i, acinfo->GetName());
		} else
			logwarn("[Data] [SaveActorInfoMap] {} Cannot write {}: ActorInfo is nullptr", i, acinfo->GetName());
	}

	lockdata.release();
	Stats::Storage_ActorsSavedLast = successfulwritten;
	return size;
}

long Data::ReadActorInfoMap(SKSE::SerializationInterface* a_intfc, uint32_t length, int& accounter, int& acdcounter, int& acfcounter)
{
	long size = 0;

	// get map lock
	lockdata.acquire();

	LOG_1("{}[Data] [ReadActorInfoMap] Reading ActorInfo...");
	unsigned char* buffer = new unsigned char[length];
	a_intfc->ReadRecordData(buffer, length);
	std::shared_ptr<ActorInfo> acinfo = CreateActorInfoNew();
	if (acinfo->ReadData(buffer, 0, length) == false) {
		acfcounter++;
		logwarn("[Data] [ReadActorInfoMap] Couldn't read ActorInfo");
	} else if (acinfo->IsValid() == false) {
		acdcounter++;
		logwarn("[Data] [ReadActorInfoMap] actor invalid {}", acinfo->GetName());
	} else if ((acinfo->GetFormFlags() & RE::TESForm::RecordFlags::kDeleted)) {
		acdcounter++;
		logwarn("[Data] [ReadActorInfoMap] actor deleted {}", acinfo->GetName());
	} else if (acinfo->IsDead()) {
		acdcounter++;
		logwarn("[Data] [ReadActorInfoMap] actor dead {}", acinfo->GetName());
		Events::Main::SetDead(acinfo->GetActor());
	} else {
		accounter++;
		RegisterActorInfo(acinfo);
		LOG1_3("{}[Data] [ReadActorInfoMap] read ActorInfo. actor: {}", Utility::PrintForm(acinfo));
	}
	delete[] buffer;
	// release lock
	lockdata.release();

	return size;
}

void Data::DeleteActorInfoMap()
{
	lockdata.acquire();
	validActors.clear();
	actorinfoMap.clear();
	deletedActors.clear();
	lockdata.release();
}


RE::TESForm* Data::FindForm(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second;
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form;
	}
	return nullptr;
}

RE::EffectSetting* Data::FindMagicEffect(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second->As<RE::EffectSetting>();
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form->As<RE::EffectSetting>();
	}
	return nullptr;
}

RE::BGSPerk* Data::FindPerk(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second->As<RE::BGSPerk>();
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form->As<RE::BGSPerk>();
	}
	return nullptr;
}

RE::AlchemyItem* Data::FindAlchemyItem(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second->As<RE::AlchemyItem>();
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form->As<RE::AlchemyItem>();
	}
	return nullptr;
}

RE::SpellItem* Data::FindSpell(uint32_t formid, std::string pluginname)
{
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		return form->As<RE::SpellItem>();
	}
	return nullptr;
}

void Data::DeleteFormCustom(RE::FormID formid)
{
	lockdata.acquire();
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end()) {
		customItemFormMap.erase(formid);
	}
	lockdata.release();
}

std::shared_ptr<CellInfo> Data::FindCell(RE::TESObjectCELL* cell)
{
	// use this if we get an invalid object, and need to return something nonetheless. since its not in the map
	// no one will ever try to delete it
	static std::shared_ptr<CellInfo> statcinfo = std::make_shared<CellInfo>();
	if (cell == nullptr)
		return statcinfo;
	std::shared_ptr<CellInfo> cinfo = nullptr;
	auto itr = cellinfoMap.find(cell->GetFormID());
	if (itr == cellinfoMap.end()) {
		cinfo = std::make_shared<CellInfo>();
		// calc and assign flags
	} else if (itr->second == nullptr || itr->second->cell == nullptr || itr->second->seasoncalc != Season::GetCurrentSeason()) {
		cellinfoMap.erase(cell->GetFormID());
		cinfo = std::make_shared<CellInfo>();
		// calc and assign flags
	} else {
		cinfo = itr->second;
		return cinfo;
	}
	// calc and assign flags
	// the other pathes have already exited the function by now
	cinfo->cell = cell;
	Season::Season season = Season::GetCurrentSeason();
	cinfo->seasoncalc = season;
	auto iter = cellMap.find(cell->GetFormID());
	if (iter != cellMap.end()) {
		if (iter->second) {
			if (iter->second->type & CellTypes::kAshland)
				cinfo->celltype |= CellInfoType::kAshland;
			else if (iter->second->type & CellTypes::kCold)
				cinfo->celltype |= CellInfoType::kCold;
			else if (iter->second->type & CellTypes::kDessert) {
				cinfo->celltype |= CellInfoType::kDessert;
				cinfo->celltype |= CellInfoType::kIntenseHeat;
			} else if (iter->second->type & CellTypes::kHeat)
				cinfo->celltype |= CellInfoType::kHeat;
			else if (iter->second->type & CellTypes::kIceland) {
				cinfo->celltype |= CellInfoType::kIceland;
				cinfo->celltype |= CellInfoType::kIntenseCold;
			} else if (iter->second->type & CellTypes::kSnow) {
				cinfo->celltype |= CellInfoType::kCold;
				cinfo->celltype |= CellInfoType::kSnow;
			} else if (iter->second->type & CellTypes::kSwamp)
				cinfo->celltype |= CellInfoType::kSwamp;
			else if (iter->second->type & CellTypes::kSummerCold && season == Season::Season::kSummer)
				cinfo->celltype |= CellInfoType::kCold;
			else if (iter->second->type & CellTypes::kSummerHot && season == Season::Season::kSummer)
				cinfo->celltype |= CellInfoType::kHeat;
			else if (iter->second->type & CellTypes::kWinterCold && season == Season::Season::kWinter)
				cinfo->celltype |= CellInfoType::kCold;
			else if (iter->second->type & CellTypes::kWinterHot && season == Season::Season::kWinter)
				cinfo->celltype |= CellInfoType::kHeat;
		}
	}
	RE::FormID formid = 0;
	if (cell->GetRuntimeData().cellLand &&
		cell->GetRuntimeData().cellLand->loadedData) {
		for (int i = 0; i < 4; i++) {
			if (cell->GetRuntimeData().cellLand->loadedData->defQuadTextures[i] &&
				cell->GetRuntimeData().cellLand->loadedData->defQuadTextures[i]->textureSet) {
				formid = cell->GetRuntimeData().cellLand->loadedData->defQuadTextures[i]->textureSet->GetFormID();
				auto itra = textureMap.find(formid);
				if (itra != textureMap.end()) {
					if (itra->second) {
						if (itra->second->type & TextureTypes::kIceland) {
							cinfo->celltype |= CellInfoType::kIceland;
							cinfo->celltype |= CellInfoType::kIntenseCold;
						} else if (itra->second->type & TextureTypes::kAshland) {
							cinfo->celltype |= CellInfoType::kAshland;
						} else if (itra->second->type & TextureTypes::kCold) {
							cinfo->celltype |= CellInfoType::kCold;
						} else if (itra->second->type & TextureTypes::kDessert) {
							cinfo->celltype |= CellInfoType::kDessert;
							cinfo->celltype |= CellInfoType::kIntenseHeat;
						} else if (itra->second->type & TextureTypes::kHeat) {
							cinfo->celltype |= CellInfoType::kHeat;
						} else if (itra->second->type & TextureTypes::kSnow) {
							cinfo->celltype |= CellInfoType::kCold;
							cinfo->celltype |= CellInfoType::kSnow;
						} else if (itra->second->type & TextureTypes::kSwamp) {
							cinfo->celltype |= CellInfoType::kSwamp;
						}

					}
				}
			}
		}
	}
	cellinfoMap.insert_or_assign(cell->GetFormID(), cinfo);

	return cinfo;
}

std::shared_ptr<WeatherInfo> Data::FindWeather(RE::TESWeather* weather)
{
	// use this if we get an invalid object, and need to return something nonetheless. since its not in the map
	// no one will ever try to delete it
	static std::shared_ptr<WeatherInfo> statwinfo = std::make_shared<WeatherInfo>();
	if (weather == nullptr)
		return statwinfo;
	std::shared_ptr<WeatherInfo> winfo = nullptr;
	auto itr = weatherinfoMap.find(weather->GetFormID());
	if (itr == weatherinfoMap.end()) {
		winfo = std::make_shared<WeatherInfo>();
		// calc and assign flags
	} else if (itr->second == nullptr || itr->second->weather == nullptr) {
		weatherinfoMap.erase(weather->GetFormID());
		winfo = std::make_shared<WeatherInfo>();
		// calc and assign flags
	} else {
		winfo = itr->second;
		return winfo;
	}
	// calc and assign flags
	// all other pathes have already returned
	winfo->weather = weather;
	auto iter = weatherMap.find(weather->GetFormID());
	if (iter != weatherMap.end()) {
		if (iter->second) {

		}
	}
	// use game weather flags
	auto itra = weather->sounds.begin();
	while (itra != weather->sounds.end()) {
		if ((*itra)->type & RE::TESWeather::SoundType::kThunder) {
			winfo->weathertype |= WeatherTypes::kThunderstorm;
		}
		static_cast<void>(itra++);
	}
	if (weather->data.flags & RE::TESWeather::WeatherDataFlag::kRainy)
		winfo->weathertype |= WeatherTypes::kRain;
	if (weather->data.flags & RE::TESWeather::WeatherDataFlag::kSnow)
		winfo->weathertype |= WeatherTypes::kSnow;

	return winfo;
}

bool Data::IsIntenseCold(RE::TESObjectCELL* cell, RE::TESWeather* weather)
{
	std::shared_ptr<CellInfo> cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kIntenseCold)
		return true;
	if (weather) {
		auto itr = weatherMap.find(weather->GetFormID());
		if (itr != weatherMap.end()) {
			if (itr->second && 
				(itr->second->type & WeatherTypes::kBlizzard || 
				 itr->second->type & WeatherTypes::kCold))
				return true;
		}
	}
	return false;
}

bool Data::IsIntenseHeat(RE::TESObjectCELL* cell, RE::TESWeather* weather)
{
	std::shared_ptr<CellInfo> cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kIntenseHeat)
		return true;
	if (weather) {
		auto itr = weatherMap.find(weather->GetFormID());
		if (itr != weatherMap.end()) {
			if (itr->second &&
				(itr->second->type & WeatherTypes::kHeat))
				return true;
		}
	}
	return false;
}

bool Data::IsDessert(RE::TESObjectCELL* cell)
{
	std::shared_ptr<CellInfo> cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kDessert)
		return true;
	return false;
}

bool Data::IsIceland(RE::TESObjectCELL* cell)
{
	std::shared_ptr<CellInfo> cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kIceland)
		return true;
	return false;
}

bool Data::IsAshland(RE::TESObjectCELL* cell)
{
	std::shared_ptr<CellInfo> cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kAshland)
		return true;
	return false;
}

bool Data::IsSwamp(RE::TESObjectCELL* cell)
{
	std::shared_ptr<CellInfo> cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kSwamp)
		return true;
	return false;
}

bool Data::IsAshstorm(RE::TESWeather* weather)
{
	if (!weather)
		return false;
	auto itr = weatherMap.find(weather->GetFormID());
	if (itr != weatherMap.end()) {
		if (itr->second && itr->second->type & WeatherTypes::kAshstorm)
			return true;
	} else
		return false; // there is no information so assume false
	return false;
}

bool Data::IsSandstorm(RE::TESWeather* weather)
{
	if (!weather)
		return false;
	auto itr = weatherMap.find(weather->GetFormID());
	if (itr != weatherMap.end()) {
		if (itr->second && itr->second->type & WeatherTypes::kSandstorm)
			return true;
	} else
		return false;  // there is no information so assume false
	return false;
}

bool Data::IsBlizzard(RE::TESWeather* weather)
{
	if (!weather)
		return false;
	auto itr = weatherMap.find(weather->GetFormID());
	if (itr != weatherMap.end()) {
		if (itr->second && itr->second->type & WeatherTypes::kBlizzard)
			return true;
	} else
		return false;  // there is no information so assume false
	return false;
}

std::shared_ptr<Disease> Data::GetDisease(Diseases::Disease value)
{
	if (diseases[value] == nullptr) {
		return {};
	}
	return diseases[value];
}

void Data::InitDiseases()
{
	// init permanent modifier stuff
	
	// kPotionOfCureCommonDisease
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x800, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x0;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureCommonDisease;
			opt->strengthfirst = 6;
			opt->strengthadditional = 3;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++)
			{
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis)
				{
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}

			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfStrongCureCommonDisease
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x801, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x1;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfStrongCureCommonDisease;
			opt->strengthfirst = 12;
			opt->strengthadditional = 6;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}

			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCureBlight
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x802, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x2;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureBlight;
			opt->strengthfirst = 6;
			opt->strengthadditional = 3;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCureFever
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x803, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x3;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureFever;
			opt->strengthfirst = 6;
			opt->strengthadditional = 3;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfStrongCureFever
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x804, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x4;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfStrongCureFever;
			opt->strengthfirst = 6;
			opt->strengthadditional = 3;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCureCholera
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x805, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x5;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureCholera;
			opt->strengthfirst = 12;
			opt->strengthadditional = 6;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCureLeprosy
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x806, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x7;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureLeprosy;
			opt->strengthfirst = 12;
			opt->strengthadditional = 6;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCurePlague
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x807, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x8;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCurePlague;
			opt->strengthfirst = 12;
			opt->strengthadditional = 6;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCureSanguinareVampirism
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x808, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x9;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCurePlague;
			opt->strengthfirst = 6;
			opt->strengthadditional = 0;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}
	// kPotionOfCureStrongDisease
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x809, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x10;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureStrongDisease;
			opt->strengthfirst = 12;
			opt->strengthadditional = 6;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}

	// kSpellOfCureBlight
	{
		RE::TESForm* form = Utility::GetTESForm(datahandler, 0x0, Settings::PluginName);
		if (form) {
			CureDiseaseOption* opt = new CureDiseaseOption();
			opt->id = 0x100;
			opt->type = CureDiseaseOption::CureDiseaseType::Potion;
			opt->modifier = PermanentModifiers::kPotionOfCureBlight;
			opt->strengthfirst = 6;
			opt->strengthadditional = 3;

			// get diseases with this modifier
			for (int i = 0; i < Diseases::kMaxValue; i++) {
				if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
					if (dis->_validModifiers & static_cast<EnumType>(opt->modifier))
						opt->diseases.push_back(static_cast<Diseases::Disease>(i));
				}
			}
	
			cureOptions.insert_or_assign(form->GetFormID(), opt);
		}
	}

	// kShrine
	{
		cureOptionsShrine = std::make_shared<CureDiseaseOption>();
		cureOptionsShrine->id = 0;
		cureOptionsShrine->type = CureDiseaseOption::CureDiseaseType::Potion;
		cureOptionsShrine->modifier = PermanentModifiers::kPotionOfCureBlight;
		cureOptionsShrine->strengthfirst = 6;
		cureOptionsShrine->strengthadditional = 3;

		// get diseases with this modifier
		for (int i = 0; i < Diseases::kMaxValue; i++) {
			if (auto dis = GetDisease(static_cast<Diseases::Disease>(i)); dis) {
				if (dis->_validModifiers & static_cast<EnumType>(cureOptionsShrine->modifier))
					cureOptionsShrine->diseases.push_back(static_cast<Diseases::Disease>(i));
			}
		}
	}
}

void Data::SetAlchItemEffects(uint32_t id, AlchemicEffect effects, int duration, float magnitude, bool detrimental, int dosage)
{
	std::tuple<AlchemicEffect, int, float, bool, int> t = { effects, duration, magnitude, detrimental, dosage };
	alchitemEffectMap.insert_or_assign(id, t);
}

std::tuple<bool, AlchemicEffect, int, float, bool, int> Data::GetAlchItemEffects(uint32_t id)
{
	auto itr = alchitemEffectMap.find(id);
	if (itr != alchitemEffectMap.end()) {
		auto [eff, dur, mag, detri, dosage] = itr->second;
		// found
		return { true, eff, dur, mag, detri, dosage };
	} else {
		// not found
		return { false, 0, 0, 0.0f, false, 0 };
	}
}

void Data::ResetAlchItemEffects()
{
	alchitemEffectMap.clear();
}

int Data::GetPoisonDosage(RE::AlchemyItem* poison)
{
	{
		auto [mapf, eff, dur, mag, detr, dosage] = GetAlchItemEffects(poison->GetFormID());
		if (mapf) {
			// found it in database
			return dosage;
		}
	}
	// we didn't find it, so we need to calculate it
	ACM::HasAlchemyEffect(poison, 0xFFFFFFFFFFFFFFFF);  // find any effect, results will be entered into database if valid
	{
		auto [mapf, eff, dur, mag, detr, dosage] = GetAlchItemEffects(poison->GetFormID());
		if (mapf) {
			// found it in database
			return dosage;
		}
	}
	return Settings::NUPSettings::Poisons::_Dosage;
}

void Data::AddDiseaseStage(std::shared_ptr<DiseaseStage> stage, uint16_t stageid)
{
	stage->CalcFlags();
	diseaseStagesMap.insert_or_assign(stageid, stage);
}

void Data::InitDisease(std::shared_ptr<Disease> disease, uint16_t stageinfection, uint16_t stageincubation, std::vector<uint16_t> stageids)
{
	auto itr = diseaseStagesMap.find(stageinfection);
	if (itr != diseaseStagesMap.end())
	{
		disease->_stageInfection = itr->second;
	}

	// init stages array
	disease->_stages = new std::shared_ptr<DiseaseStage>[disease->_numstages + 1];

	itr = diseaseStagesMap.find(stageincubation);
	if (itr != diseaseStagesMap.end())
	{
		disease->_stages[0] = itr->second;
	}
	for (int i = 0; i < stageids.size(); i++)
	{
		itr = diseaseStagesMap.find(stageids[i]);
		if (itr != diseaseStagesMap.end())
		{
			disease->_stages[i + 1] = itr->second;
		}
	}

	disease->CalcFlags();

	diseases[disease->_disease] = disease;

	if (disease->_stageInfection->_spreading[Spreading::kIntenseCold].first > 0)
		spreadingDiseaseMap[Spreading::kIntenseCold].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kIntenseHeat].first > 0)
		spreadingDiseaseMap[Spreading::kIntenseHeat].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInAshland].first > 0)
		spreadingDiseaseMap[Spreading::kInAshland].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInSwamp].first > 0)
		spreadingDiseaseMap[Spreading::kInSwamp].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInDessert].first > 0)
		spreadingDiseaseMap[Spreading::kInDessert].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInAshstorm].first > 0)
		spreadingDiseaseMap[Spreading::kInAshstorm].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInSandstorm].first > 0)
		spreadingDiseaseMap[Spreading::kInSandstorm].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInBlizzard].first > 0)
		spreadingDiseaseMap[Spreading::kInBlizzard].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kInRain].first > 0)
		spreadingDiseaseMap[Spreading::kInRain].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kIsWindy].first > 0)
		spreadingDiseaseMap[Spreading::kIsWindy].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kIsStormy].first > 0)
		spreadingDiseaseMap[Spreading::kIsStormy].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kIsCold].first > 0)
		spreadingDiseaseMap[Spreading::kIsCold].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kIsHeat].first > 0)
		spreadingDiseaseMap[Spreading::kIsHeat].push_back(disease->_disease);
	if (disease->_stageInfection->_spreading[Spreading::kExtremeConditions].first > 0)
		spreadingDiseaseMap[Spreading::kExtremeConditions].push_back(disease->_disease);
}

void Data::ResetDiseases()
{
	for (int i = 0; i < Diseases::kMaxValue; i++)
	{
		diseases[i]->_stageInfection = {};
		for (int c = 0; c <= diseases[i]->_numstages; c++)
		{
			diseases[i]->_stages[c] = {};
		}
		delete diseases[i]->_stages;
		diseases[i] = {};
	}

	diseaseStagesMap.clear();
}

std::pair<std::unordered_map<Diseases::Disease, std::pair<float /*chance*/, float /*scale*/>> , std::set<Diseases::Disease>> Data::GetPossibleInfections(std::shared_ptr<ActorInfo> const& acinfo, Misc::NPCTPLTInfo* tpltinfo)
{
	auto begin = std::chrono::steady_clock::now();
	if (acinfo == nullptr || acinfo->IsValid() == false) {
		return {};
	}
	// get npc template info
	Misc::NPCTPLTInfo tplt;
	if (tpltinfo == nullptr) {
		tplt = Utility::ExtractTemplateInfo(acinfo->GetActor());
		tpltinfo = &tplt;
	}

	std::unordered_map<Diseases::Disease, std::pair<float /*chance*/, float /*scale*/>> infec;
	auto infecinsert = [&infec](std::tuple < Diseases::Disease, float, float> tup) {
		auto [dis, chance, scale] = tup;
		auto itr = infec.find(dis);
		if (itr != infec.end())
		{
			auto [origchance, origscale] = itr->second;
			if (origchance == 0 && chance != 0 ||
				origchance != 0 && chance != 0 && chance > origchance ||
				origchance == 0 && chance == 0 && scale > origscale)
			{
				infec.insert_or_assign(dis, std::pair<float, float>{ chance, scale });
			}
			else
			{
				// what we have has the larger chance in the end
			}
		}
		else
		{
			infec.insert_or_assign(dis, std::pair<float, float>{ chance, scale });
		}
	};
	std::set<Diseases::Disease> infecforce;

	auto base = acinfo->GetActorBase();

	// define general stuff
	auto race = acinfo->GetRace();

	// find rule in npc map
	// npc rules always have the highest priority
	auto itnpcf = diseasesForceAssoc.find(acinfo->GetFormID());
	if (itnpcf != diseasesForceAssoc.end()) {  // found the right rule!
		infecforce.insert(itnpcf->second);
	}

	auto itnpc = diseasesAssoc.find(acinfo->GetFormID());
	if (itnpc != diseasesAssoc.end() && itnpc->second) {  // found the right rule!
		for (int i = 0; i < itnpc->second->size(); i++) {
			infecinsert(itnpc->second->at(i));
		}
	}

	// search for base npc
	itnpcf = diseasesForceAssoc.find(acinfo->GetActorBaseFormID());
	if (itnpcf != diseasesForceAssoc.end()) {  // found the right rule!
		infecforce.insert(itnpcf->second);
	}

	itnpc = diseasesAssoc.find(acinfo->GetActorBaseFormID());
	if (itnpc != diseasesAssoc.end() && itnpc->second) {  // found the right rule!
		for (int i = 0; i < itnpc->second->size(); i++)
			infecinsert(itnpc->second->at(i));
	}

	// perform check on tpltactorbaseinformation
	if (tpltinfo->base != nullptr && tpltinfo->base != acinfo->GetActorBase()) {
		itnpcf = diseasesForceAssoc.find(tpltinfo->base->GetFormID());
		if (itnpcf != diseasesForceAssoc.end()) {  // found the right rule!
			infecforce.insert(itnpcf->second);
		}
		itnpc = diseasesAssoc.find(tpltinfo->base->GetFormID());
		if (itnpc != diseasesAssoc.end() && itnpc->second) {  // found the right rule!
			for (int i = 0; i < itnpc->second->size(); i++)
				infecinsert(itnpc->second->at(i));
		}
	}

	//if (tpltinfo && tpltinfo->tpltrace)
	//	race = tpltinfo->tpltrace;
	// now that we didnt't find something so far, check the rest
	// this time all the priorities are the same
	auto it = diseasesAssoc.find(race->GetFormID());
	if (it != diseasesAssoc.end())
		for (int i = 0; i < it->second->size(); i++)
			infecinsert(it->second->at(i));
	for (uint32_t i = 0; i < race->numKeywords; i++) {
		auto itr = diseasesAssoc.find(race->keywords[i]->GetFormID());
		if (itr != diseasesAssoc.end()) {
			for (int i = 0; i < itr->second->size(); i++)
				infecinsert(itr->second->at(i));
		}
	}
	auto itf = diseasesForceAssoc.find(race->GetFormID());
	if (itf != diseasesForceAssoc.end())
		infecforce.insert(itf->second);
	for (uint32_t i = 0; i < race->numKeywords; i++) {
		auto itrf = diseasesForceAssoc.find(race->keywords[i]->GetFormID());
		if (itrf != diseasesForceAssoc.end()) {
			infecforce.insert(itrf->second);
		}
	}


	// handle keywords
	for (unsigned int i = 0; i < base->numKeywords; i++) {
		auto key = base->keywords[i];
		if (key) {
			auto it = diseasesAssoc.find(key->GetFormID());
			if (it != diseasesAssoc.end())
				for (int i = 0; i < it->second->size(); i++)
					infecinsert(it->second->at(i));

			auto itr = diseasesForceAssoc.find(key->GetFormID());
			if (itr != diseasesForceAssoc.end())
				infecforce.insert(itr->second);
		}
	}
	if (tpltinfo) {
		//logger::info("rule 10");
		for (int i = 0; i < tpltinfo->tpltkeywords.size(); i++) {
			if (tpltinfo->tpltkeywords[i]) {
				auto it = diseasesAssoc.find(tpltinfo->tpltkeywords[i]->GetFormID());
				if (it != diseasesAssoc.end())
					for (int i = 0; i < it->second->size(); i++)
						infecinsert(it->second->at(i));
				auto itr = diseasesForceAssoc.find(tpltinfo->tpltkeywords[i]->GetFormID());
				if (itr != diseasesForceAssoc.end())
					infecforce.insert(itr->second);
			}
		}
	}

	// handle factions
	for (uint32_t i = 0; i < base->factions.size(); i++) {
		auto it = diseasesAssoc.find(base->factions[i].faction->GetFormID());
		if (it != diseasesAssoc.end()) {
			for (int i = 0; i < it->second->size(); i++)
				infecinsert(it->second->at(i));
		}
		auto itr = diseasesForceAssoc.find(base->factions[i].faction->GetFormID());
		if (itr != diseasesForceAssoc.end()) {
			infecforce.insert(itr->second);
		}
	}
	if (tpltinfo) {
		for (int i = 0; i < tpltinfo->tpltfactions.size(); i++) {
			if (tpltinfo->tpltfactions[i]) {
				auto it = diseasesAssoc.find(tpltinfo->tpltfactions[i]->GetFormID());
				if (it != diseasesAssoc.end()) {
					for (int i = 0; i < it->second->size(); i++)
						infecinsert(it->second->at(i));
				}
				auto itr = diseasesForceAssoc.find(tpltinfo->tpltfactions[i]->GetFormID());
				if (itr != diseasesForceAssoc.end()) {
					infecforce.insert(itr->second);
				}
			}
		}
	}

	// dont use tplt for class and combatstyle, since they may have been modified during runtime

	// handle classes
	if (base->npcClass) {
		auto it = diseasesAssoc.find(base->npcClass->GetFormID());
		if (it != diseasesAssoc.end()) {
			for (int i = 0; i < it->second->size(); i++)
				infecinsert(it->second->at(i));
		}
		auto itr = diseasesForceAssoc.find(base->npcClass->GetFormID());
		if (itr != diseasesForceAssoc.end()) {
			infecforce.insert(itr->second);
		}
	}
	// handle combat styles
	if (base->combatStyle) {
		auto it = diseasesAssoc.find(base->combatStyle->GetFormID());
		if (it != diseasesAssoc.end()) {
			for (int i = 0; i < it->second->size(); i++)
				infecinsert(it->second->at(i));
		}
		auto itr = diseasesForceAssoc.find(base->combatStyle->GetFormID());
		if (itr != diseasesForceAssoc.end()) {
			infecforce.insert(itr->second);
		}
	}

	PROF1_1("{}[Data] [GetPossibleInfections] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));

	return std::pair<std::unordered_map<Diseases::Disease, std::pair<float /*chance*/, float /*scale*/>>, std::set<Diseases::Disease>>{ infec, infecforce };
}

void Data::PatchGameObjects()
{
	// Patch Sanguinare Vampirism to use the new magic effect instead of the old one
	{
		RE::EffectSetting* effect = Utility::GetTESForm<RE::EffectSetting>(datahandler, 0x0899, Settings::PluginName);
		if (effect)
		{
			RE::SpellItem* sanvam = RE::TESForm::LookupByID<RE::SpellItem>(0xB8780);
			if (sanvam)
			{
				if (sanvam->effects.size() > 0)
				{
					if (sanvam->effects.size() > 1)
					{
						for (int i = 0; i < sanvam->effects.size(); i++) {
							auto eff = sanvam->effects.back();
							sanvam->effects.pop_back();
							RE::free(eff);
						}
					}
					sanvam->effects[0]->baseEffect = effect;
				}
				else
				{
					RE::Effect* eff = RE::malloc<RE::Effect>();
					eff->baseEffect = effect;
					eff->effectItem.area = 0;
					eff->effectItem.magnitude = 25;
					eff->effectItem.duration = 0;
					eff->conditions.head = nullptr;
				}
			}
		}
	}

	// Patch all magic effects used by my diseases to show up under active magic effects if wanted
	if (Settings::System::_showDiseaseEffects)
	{
		for (int i = 0; i < Diseases::kMaxValue; i++)
		{
			if (diseases[i])
			{
				if (diseases[i]->endeffect)
				{
					for (int c = 0; c < diseases[i]->endeffect->effects.size(); c++)
					{
						if (auto eff = diseases[i]->endeffect->effects[c]->baseEffect; eff)
							eff->data.flags = eff->data.flags.reset(RE::EffectSetting::EffectSettingData::Flag::kHideInUI);
					}
				}
				if (diseases[i]->_stageInfection && diseases[i]->_stageInfection->effect) {
					for (int c = 0; c < diseases[i]->_stageInfection->effect->effects.size(); c++) {
						if (auto eff = diseases[i]->_stageInfection->effect->effects[c]->baseEffect; eff)
							eff->data.flags = eff->data.flags.reset(RE::EffectSetting::EffectSettingData::Flag::kHideInUI);
					}
				}
				for (int x = 0; x <= diseases[i]->_numstages; x++)
				{
					if (diseases[i]->_stages[x] && diseases[i]->_stages[x]->effect) {
						for (int c = 0; c < diseases[i]->_stages[x]->effect->effects.size(); c++) {
							if (auto eff = diseases[i]->_stages[x]->effect->effects[c]->baseEffect; eff)
								eff->data.flags = eff->data.flags.reset(RE::EffectSetting::EffectSettingData::Flag::kHideInUI);
						}
					}
				}
			}
		}
	}
}


void Data::RemoveAllDiseases()
{
	lockdata.acquire();
	for (auto [key, acinfo] : actorinfoMap)
	{
		acinfo->RemoveAllDiseases();
	}
	
	lockdata.release();
}
