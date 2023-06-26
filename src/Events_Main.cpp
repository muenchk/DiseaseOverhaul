
#include <chrono>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "ActorManipulation.h"
#include "BufferOperations.h"
#include "Distribution.h"
#include "Events.h"
#include "Logging.h"
#include "Random.h"
#include "Settings.h"
#include "Utility.h"
#include "UtilityAlch.h"
#include "Stats.h"

namespace Events
{


	void Main::HandleActors()
	{
		// wait until processing is allowed, or we should kill ourselves
		while (!CanProcess() && stopactorhandler == false)
			std::this_thread::sleep_for(10ms);
		LOG_1("{}[Events] [HandleActors]");
		_handleactorsrunning = true;
		// static section
		RE::UI* ui = RE::UI::GetSingleton();
		auto begin = std::chrono::steady_clock::now();
		auto beginparticles = std::chrono::steady_clock::now();
		auto datahandler = RE::TESDataHandler::GetSingleton();
		RE::TESObjectCELL* cell = nullptr;
		RE::TESObjectREFR* refr = nullptr;
		RE::Actor* actor = nullptr;
		std::weak_ptr<ActorInfo> playerweak = data->FindActor(RE::PlayerCharacter::GetSingleton());

		std::unordered_set<uint32_t> visited;
		std::vector<std::shared_ptr<ActorInfo>> actors;
		std::vector<std::shared_ptr<ActorInfo>> infected[Diseases::kMaxValue];
		std::vector<std::shared_ptr<ActorInfo>> allinfected;

		while (_handleactorsstop == false) {
			if (!CanProcess())
				goto HandleActorsSkipIteration;

			_handleactorsworking = true;

			PlayerDied((bool)(RE::PlayerCharacter::GetSingleton()->GetActorRuntimeData().boolBits & RE::Actor::BOOL_BITS::kDead) || RE::PlayerCharacter::GetSingleton()->IsDead());
			
			// if we are in a paused menu (SoulsRE unpauses menus, which is supported by this) skip iter
			if (!ui->GameIsPaused() && initialized && !playerdied) {
				LOG_1("{}[Events] [HandleActors] begin round");

				// stat tracking
				Stats::MainHandler_Run++;

				begin = std::chrono::steady_clock::now();

				// get lock
				sem.acquire();
				// take iter time

				std::shared_ptr<CellInfo> cinfo;
				// get cell information
				if (std::shared_ptr<ActorInfo> playerinfo = playerweak.lock()) {
					cinfo = data->FindCell(playerinfo->GetParentCell());
					if (!cellist.contains(cinfo))
						cellist.insert(cinfo);
				}

				// get weather information
				std::shared_ptr<WeatherInfo> winfo = data->FindWeather(RE::Sky::GetSingleton()->currentWeather);

				LOG_1("{}[Events] [HandleActors] get all actors");

				auto citer = cellist.begin();
				while (citer != cellist.end()) {
					cinfo = *citer;
					// first get all actors that shall be handled in player cell
					for (auto& ptr : cinfo->cell->GetRuntimeData().references) {
						if (ptr.get()) {
							actor = ptr.get()->As<RE::Actor>();
							if (actor) {
								actors.push_back(data->FindActor(actor));
							}
						}
					}
					citer++;
				}

				// release lock
				sem.release();

				if (!CanProcess())
					goto HandleActorsSkipIteration;
				if (IsPlayerDead())
					break;

				LOG_1("{}[Events] [HandleActors] create necessary vars");

				float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

				float* points = new float[actors.size()];
				int* ticks = new int[actors.size()];
				bool* kill = new bool[actors.size()];

				LOG_1("{}[Events] [HandleActors] calculate ticks for all actors");

				// calculate ticks for each actor and set current time as last change
				for (int x = 0; x < actors.size(); x++) {
					ticks[x] = (int)((currentgameday - actors[x]->dinfo.LastGameTime) / Settings::System::_ticklength);
					//LOG4_1("{}[Events] [HandleActors] time: {}\tlast: {}\tticklength: {}\tticks: {}", currentgameday, actors[x]->dinfo->LastGameTime, Settings::AlchExtSettings::TickLength, ticks[x]);
					if (ticks[x] > 0)
						actors[x]->dinfo.LastGameTime = currentgameday;
					points[x] = 0;
					kill[x] = false;
				}

				beginparticles = std::chrono::steady_clock::now();

				LOG_1("{}[Events] [HandleActors] calculate particle effects");

				// first calculate effects occuring in particle range
				// 0) calculate actor pairs for infected
				// 1) calculate actor distances
				// 2) get those in particle range
				// particles work in a close range distance in any cells / between cells
				{
					for (int i = 0; i < Diseases::kMaxValue; i++)
						infected[i] = UtilityAlch::GetProgressingActors(actors, static_cast<Diseases::Disease>(i));
					allinfected = UtilityAlch::GetProgressingActors(actors);
					// will be deleted after we leave the block
					std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> distances = UtilityAlch::GetActorDistancesMap(allinfected, actors, Settings::Disease::_particleRange);
					LOG1_1("{}[Events] [HandleActors] calculate particle effects for {} pairs", distances.size());

					// iterate over illnesses
					for (int i = 0; i < Diseases::kMaxValue; i++) {
						// get disease
						Diseases::Disease disval = static_cast<Diseases::Disease>(i);
						std::shared_ptr<Disease> dis = data->GetDisease(disval);
						if (!dis)
							continue;
						if (dis->ParticleSpread() == false) {
							//LOG1_1("{}[Events] [HandleEvents] Disease does not spread via particles: {}", UtilityAlch::ToString(disval));
							continue;  // disease does not spread via particles
						}
						std::shared_ptr<DiseaseStage> stage = nullptr;

						// deprecated
						/*if (particlehandling) {
							// iterate over infected for disease
							for (int c = 0; c < infected[i].size(); c++) {
								float scale = 0;
								// get disease stage of infected
								stage = dis->_stages[infected[i][c]->dinfo->FindDisease(disval)->stage];
								// get scaling from infectivity
								switch (stage->_infectivity) {
								case Infectivity::kLow:
									scale = 0.7f;
									break;
								case Infectivity::kNormal:
									scale = 1.0f;
									break;
								case Infectivity::kHigher:
									scale = 1.3f;
									break;
								case Infectivity::kHigh:
									scale = 1.7f;
									break;
								case Infectivity::kVeryHigh:
									scale = 2.0f;
								}
								if (scale == 0)
									continue;  // got to next infected instead
								// iterate over other actors
								for (int x = 0; x < actors.size(); x++) {
									auto itr = distances.find((((uint64_t)infected[i][c]->actor->GetFormID()) << 32) | actors[x]->actor->GetFormID());
									if (itr != distances.end()) {
										//LOG3_1("{}[Events] [HandleActors] particle distance: {} & {}, distance: {}", infected[i][c]->actor->GetName(), actors[x]->actor->GetName(), itr->second);
										// there is no iteration for the ticks here, since particle range can change very fast
										// if chance succeeds
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kParticle]))
											points[x] += scale * (1 - actors[x]->IgnoresDisease()) * std::get<1>(stage->_spreading[Spreading::kParticle]);
									}
								}
							}
						} else */
						{
							std::shared_ptr<ActorInfo> infec, act;
							int idx2;
							auto iter = distances.begin();
							while (iter != distances.end()) {
								//LOG2_1("{}tried to access index: {} of max {}", iter->first >> 32, allinfected.size());
								if (iter->first >> 32 > allinfected.size() || (uint32_t)(iter->first) > actors.size()) {
									iter++;
									continue;
								}
								infec = allinfected[iter->first >> 32];
								idx2 = (uint32_t)iter->first;
								act = actors[idx2];
								float scale = 0;
								// get disease stage of infected
								stage = dis->_stages[infec->dinfo.FindDisease(disval)->stage];
								// get scaling from infectivity
								switch (stage->_infectivity) {
								case Infectivity::kLow:
									scale = 0.7f;
									break;
								case Infectivity::kNormal:
									scale = 1.0f;
									break;
								case Infectivity::kHigher:
									scale = 1.3f;
									break;
								case Infectivity::kHigh:
									scale = 1.7f;
									break;
								case Infectivity::kVeryHigh:
									scale = 2.0f;
								}
								scale = scale * (1 - act->IgnoresDisease());
								if (scale == 0)
									continue;  // got to next infected instead
								if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kParticle]))
									points[idx2] += scale * std::get<1>(stage->_spreading[Spreading::kParticle]);

								iter++;
							}

							// now we have calculated the points that will be added for the disease
							// add the points
							for (int x = 0; x < actors.size(); x++) {
								if (points[x] != 0) {
									kill[x] |= actors[x]->dinfo.ProgressDisease(actors[x]->GetActor(), disval, points[x]);
									// reset points for next iteration / further stuff
									points[x] = 0;
								}
							}
						}
					}
				}

				Stats::MainHandler_Particles_Times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count());

				// all of the following is calculated in ticks, so remove those who do not need to be processed
				for (int x = 0; x < actors.size(); x++) {
					if (ticks[x] == 0) {
						//LOG1_1("{}[Events] [HandleActors] removed: {}", actors[x]->actor->GetName());
						actors[x] = actors[actors.size() - 1];
						ticks[x] = ticks[actors.size() - 1];
						actors.erase(std::prev(actors.end()));
						x--;
					}
				}

				LOG_1("{}[Events] [HandleActors] calculate air effects");

				// calculate effects occuring due to air infection
				// air infection may only happen in interior spaces
				{
					// get all npcs in the same interior spaces
					std::unordered_map<uint32_t, std::vector<int /*idx in actors*/>> cellmates;
					for (int i = 0; i < allinfected.size(); i++) {
						// skip those in exterior cells
						if (allinfected[i]->GetParentCell() && allinfected[i]->GetParentCell()->IsInteriorCell() == false)
							continue;
						std::vector<int> vec;
						for (int x = 0; x < actors.size(); x++) {
							// skip if same actor or not in same cell
							if (allinfected[i]->GetParentCell() != actors[x]->GetParentCell() || allinfected[i] == allinfected[x])
								continue;
							vec.push_back(x);
						}
						if (vec.size() > 0)
							cellmates.insert_or_assign(allinfected[i]->GetFormID(), std::vector<int>{});
					}

					// iterate over illnesses
					for (int i = 0; i < Diseases::kMaxValue; i++) {
						// get disease
						Diseases::Disease disval = static_cast<Diseases::Disease>(i);
						std::shared_ptr<Disease> dis = data->GetDisease(disval);
						if (!dis)
							continue;
						std::shared_ptr<DiseaseStage> stage = nullptr;
						// iterate over infected for disease
						for (int c = 0; c < infected[i].size(); c++) {
							auto itr = cellmates.find(infected[i][c]->GetFormID());
							if (itr != cellmates.end()) {
								auto vec = itr->second;
								float scale = 0;
								// get disease stage of infected
								stage = dis->_stages[infected[i][c]->dinfo.FindDisease(disval)->stage];
								// get scaling from infectivity
								switch (stage->_infectivity) {
								case Infectivity::kLow:
									scale = 0.7f;
									break;
								case Infectivity::kNormal:
									scale = 1.0f;
									break;
								case Infectivity::kHigher:
									scale = 1.3f;
									break;
								case Infectivity::kHigh:
									scale = 1.7f;
									break;
								case Infectivity::kVeryHigh:
									scale = 2.0f;
								}
								if (scale == 0)
									continue;  // got to next infected instead
								// iterate over other actors
								for (int y = 0; y < vec.size(); y++) {
									// iterate for number of ticks for this actor
									for (int t = 0; t < ticks[vec[y]]; t++) {
										// if chance succeeds
										if (float scl = scale * (1 - actors[vec[y]]->IgnoresDisease()); scl != 0.0f)
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kAir]))
											points[vec[y]] += scl * std::get<1>(stage->_spreading[Spreading::kAir]);
									}
								}
							}
						}
						// now we have calculated the points that will be added for the disease
						// add the points
						for (int x = 0; x < actors.size(); x++) {
							if (points[x] != 0) {
								kill[x] |= actors[x]->dinfo.ProgressDisease(actors[x]->GetActor(), disval, points[x]);
								// reset points for next iteration / further stuff
								points[x] = 0;
							}
						}
					}
				}

				LOG_1("{}[Events] [HandleActors] calculate static effects");

				// calculate static cell and weather effects
				// also calculate regular disease advancement while we are at it
				for (int i = 0; i < Diseases::kMaxValue; i++) {
					// get disease
					Diseases::Disease disval = static_cast<Diseases::Disease>(i);
					std::shared_ptr<Disease> dis = data->GetDisease(disval);
					if (!dis)
						continue;
					if (dis->AirSpread() == false)
						continue;  // disease does not spread via air
					std::shared_ptr<DiseaseStage> stage = nullptr;
					DiseaseInfo* dinfo = nullptr;
					// iterate actors
					for (int x = 0; x < actors.size(); x++) {
						//LOG1_1("{}[Events] [HandleActors] actor: {}", actors[x]->actor->GetName());
						if (ticks[x] == 0)
							continue;
						dinfo = actors[x]->dinfo.FindDisease(disval);
						if (dinfo == nullptr)
							stage = dis->_stageInfection;
						else
							stage = dis->_stages[dinfo->stage];
						// calc actor point scaling
						float scale = 0;
						switch (stage->_infectivity) {
						case Infectivity::kLow:
							scale = 0.7f;
							break;
						case Infectivity::kNormal:
							scale = 1.0f;
							break;
						case Infectivity::kHigher:
							scale = 1.3f;
							break;
						case Infectivity::kHigh:
							scale = 1.7f;
							break;
						case Infectivity::kVeryHigh:
							scale = 2.0f;
						}
						scale = scale * (1 - actors[x]->IgnoresDisease());
						if (scale > 0.0f) {
							//LOG1_1("{}[Events] [HandleActors] base, ticks: {}", ticks[x]);
							// iterate ticks
							if (ticks[x] < 10) {
								// for only a few ticks, use actual probabilistic calculation, since we don't have that much to do
								for (int t = 0; t < ticks[x]; t++) {
									// handle weather effects
									if (winfo->IsIntenseCold()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIntenseCold]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]);
									} else if (winfo->IsCold()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIsCold]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIsCold]);
									}
									if (winfo->IsIntenseHeat()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIntenseHeat]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]);
									} else if (winfo->IsHeat()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIsHeat]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]);
									}
									if (winfo->IsAshstorm()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInAshstorm]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInAshstorm]);
									}
									if (winfo->IsSandstorm()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInSandstorm]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInSandstorm]);
									}
									if (winfo->IsBlizzard()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInBlizzard]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInBlizzard]);
									}
									if (winfo->IsRain()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInRain]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInRain]);
									}
									if (winfo->IsStormy()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIsStormy]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIsStormy]);
									} else if (winfo->IsWindy()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIsWindy]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIsWindy]);
									}
									if (winfo->IsExtremeCondition())
									{
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kExtremeConditions]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kExtremeConditions]);
									}

									//  handle cell effects
									cinfo = data->FindCell(actors[x]->GetParentCell());
									if (cinfo->IsAshland()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInAshland]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInAshland]);
									}
									if (cinfo->IsIntenseCold()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIntenseCold]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]);
									} else if (cinfo->IsCold()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIsCold]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIsCold]);
									}
									if (cinfo->IsIntenseHeat()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIntenseHeat]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]);
									} else if (cinfo->IsHeat()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kIsHeat]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]);
									}
									if (cinfo->IsDessert()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInDessert]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInDessert]);
									}
									if (cinfo->IsSwamp()) {
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kInSwamp]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kInSwamp]);
									}
									if (cinfo->IsExtremeCondition())
									{
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kExtremeConditions]))
											points[x] += scale * std::get<1>(stage->_spreading[Spreading::kExtremeConditions]);
									}
								}
							} else {
								// if we have more ticks, use the limes, since we do not have the time to compute the result, and the actual result won't differ too much for large values
								// we may have thousands of ticks at once, so that may actually be a problem otherwise
								// handle weather effects
								if (winfo->IsIntenseCold()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]);
								} else if (winfo->IsCold()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIsCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsCold]);
								}
								if (winfo->IsIntenseHeat()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]);
								} else if (winfo->IsHeat()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIsHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]);
								}
								if (winfo->IsAshstorm()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInAshstorm]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInAshstorm]);
								}
								if (winfo->IsSandstorm()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInSandstorm]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInSandstorm]);
								}
								if (winfo->IsBlizzard()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInBlizzard]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInBlizzard]);
								}
								if (winfo->IsRain()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInRain]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInRain]);
								}
								if (winfo->IsStormy()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIsStormy]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsStormy]);
								} else if (winfo->IsWindy()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIsWindy]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsWindy]);
								}
								if (winfo->IsExtremeCondition()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kExtremeConditions]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInRain]);
								}

								//  handle cell effects
								cinfo = data->FindCell(actors[x]->GetParentCell());
								if (cinfo->IsAshland()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInAshland]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInAshland]);
								}
								if (cinfo->IsIntenseCold()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]);
								} else if (cinfo->IsCold()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIsCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsCold]);
								}
								if (cinfo->IsIntenseHeat()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]);
								} else if (cinfo->IsHeat()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kIsHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]);
								}
								if (cinfo->IsDessert()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInDessert]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInDessert]);
								}
								if (cinfo->IsSwamp()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kInSwamp]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInSwamp]);
								}
								if (cinfo->IsExtremeCondition()) {
									points[x] += ((float)std::get<0>(stage->_spreading[Spreading::kExtremeConditions]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInSwamp]);
								}
							}

							// handle static progression effects
							if (dinfo && dinfo->status == DiseaseStatus::kProgressing) {
								//LOG3_1("{}[Events] [HandleActors] base, ticks: {}, ignore: {}, points: {}", ticks[x], std::to_string(actors[x]->IgnoresDisease()), std::to_string((1 - actors[x]->IgnoresDisease()) * dis->_baseProgressionPoints * ticks[x]));
								points[x] += (1 - actors[x]->IgnoresDisease()) * dis->_baseProgressionPoints * ticks[x];
							}
						}
						if (dinfo && dinfo->permanentModifiersPoints != 0)
							points[x] += dinfo->permanentModifiersPoints * ticks[x];
					}
					// now we have calculated the points that will be added for the disease
					// add the points
					for (int x = 0; x < actors.size(); x++) {
						if (points[x] != 0) {
							kill[x] |= actors[x]->dinfo.ProgressDisease(actors[x]->GetActor(), disval, points[x]);
							// reset points for next iteration / further stuff
							points[x] = 0;
						}
					}
				}

				LOG_1("{}[Events] [HandleActors] kill actors");

				// now kill those who have to die from sickness MUHAHAHA
				for (int x = 0; x < actors.size(); x++) {
					//	if (kill[x] && !actors[x]->actor->IsDead())
					//actors[x]->actor->KillImmediate();
				}

				LOG_1("{}[Events] [HandleActors] delete vars");

				delete[] points;
				delete[] ticks;
				delete[] kill;

				// clean up player cell if needed
				if (cinfo->cell->IsAttached() == false)
					cellist.erase(cinfo);

				// endtime
				uint64_t runtime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

				// stat tracking
				if (actors.size() > Stats::MainHandler_ActorsHandled)
					Stats::MainHandler_ActorsHandled = actors.size();
				if (runtime > Stats::MainHandler_TimeTaken)
					Stats::MainHandler_TimeTaken = runtime;
				Stats::MainHandler_ActorsHandledTotal += actors.size();

				// write execution time of iteration
				PROF1_1("{}[Events] [HandleActors] execution time: {} µs", std::to_string(runtime));
				LOG1_1("{}[Events] [HandleActors] {} actors handled", std::to_string(actors.size()));

				// reset lists for next iteration
				actors.clear();
				for (int i = 0; i < Diseases::kMaxValue; i++)
					infected[i].clear();
				allinfected.clear();
			} else {
				LOG_1("{}[Events] [HandleActors] Skip round.");
			}

HandleActorsSkipIteration:
			// reset combatants
			combatants.clear();
			_handleactorsworking = false;
			if (!_handleactorsstop)
				std::this_thread::sleep_for(std::chrono::duration<int, std::milli>  (Settings::System::_cycletime));
		}

		LOG_1("{}[Events] [HandleActors] Exit.");
		_handleactorsrunning = false;
		_handleactorsstop = false;
	}

	void Main::LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [LoadGameCallback]");
		auto begin = std::chrono::steady_clock::now();
		// if we canceled the main thread, reset that
		stopactorhandler = false;
		initialized = false;

		if (_handleactorsrunning == false) {
			if (_handleactors != nullptr) {
				// if the thread is there, then destroy and delete it
				// if it is joinable and not running it has already finished, but needs to be joined before
				// it can be destroyed savely
				_handleactors->~thread();
				delete _handleactors;
				_handleactors = nullptr;
			}
			_handleactorsstop = false;
			_handleactors = new std::thread(HandleActors);
			_handleactors->detach();
			LOG_1("{}[Events] [LoadGameSub] Started HandleActors");
		}
		// reset the list of actors that died
		deads.clear();
		// reset list of actors in combat
		combatants.clear();
		// set player to alive
		PlayerDied((bool)(RE::PlayerCharacter::GetSingleton()->GetActorRuntimeData().boolBits & RE::Actor::BOOL_BITS::kDead) || RE::PlayerCharacter::GetSingleton()->IsDead());
		enableProcessing = true;

		enableProcessing = true;

		InitializeCompatibilityObjects();
		LOG_1("{}[Events] [LoadGameCallback] end");
		PROF1_1("{}[Events] [LoadGameCallback] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
	}

	void Main::SaveGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [SaveGameCallback]");
	}

	void Main::RevertGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [RevertGameCallback]");
		enableProcessing = false;
		_handleactorsstop = true;
		std::this_thread::sleep_for(10ms);
		if (actorhandler != nullptr)
			actorhandler->~thread();
		LOG1_1("{}[PlayerDead] {}", playerdied);
		// reset actor processing list
		acset.clear();
		cellist.clear();
	}
	
	long Main::SaveDeadActors(SKSE::SerializationInterface* a_intfc)
	{
		LOG_1("{}[Events] [SaveDeadActors] Writing dead actors");
		LOG1_1("{}[Events] [SaveDeadActors] {} dead actors to write", deads.size());

		long size = 0;
		long successfulwritten = 0;

		for (auto& handle : deads) {
			if (RE::Actor* actor = handle.get().get(); actor != nullptr) {
				RE::FormID id = actor->GetFormID();
				uint32_t formid = Utility::Mods::GetIndexLessFormID(id);
				std::string pluginname = Utility::Mods::GetPluginNameFromID(id);
				if (a_intfc->OpenRecord('EDID', 0)) {
					// get entry length
					int length = 4 + Buffer::CalcStringLength(pluginname);
					// save written bytes number
					size += length;
					// create buffer
					unsigned char* buffer = new unsigned char[length + 1];
					if (buffer == nullptr) {
						logwarn("[Events] [SaveDeadActors] failed to write Dead Actor record: buffer null");
						continue;
					}
					// fill buffer
					int offset = 0;
					Buffer::Write(id, buffer, offset);
					Buffer::Write(pluginname, buffer, offset);
					// write record
					a_intfc->WriteRecordData(buffer, length);
					delete[] buffer;
					successfulwritten++;
				}
			}
		}

		LOG_1("{}[Events] [SaveDeadActors] Writing dead actors finished");

		return size;
	}

	long Main::ReadDeadActors(SKSE::SerializationInterface* a_intfc, uint32_t length)
	{
		long size = 0;

		LOG_1("{}[Events] [ReadDeadActors] Reading Dead Actor...");
		unsigned char* buffer = new unsigned char[length];
		a_intfc->ReadRecordData(buffer, length);
		if (length >= 12) {
			int offset = 0;
			uint32_t formid = Buffer::ReadUInt32(buffer, offset);
			std::string pluginname = Buffer::ReadString(buffer, offset);
			RE::TESForm* form = RE::TESDataHandler::GetSingleton()->LookupForm(formid, pluginname);
			if (form) {
				if (RE::Actor* actor = form->As<RE::Actor>(); actor != nullptr) {
					deads.insert(actor->GetHandle());
				}
			}
		}
		delete[] buffer;

		return size;
	}
}
