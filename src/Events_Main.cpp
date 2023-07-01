
#include <chrono>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <omp.h>

#include "ActorManipulation.h"
#include "BufferOperations.h"
#include "Distribution.h"
#include "Events.h"
#include "Game.h"
#include "Logging.h"
#include "Random.h"
#include "Settings.h"
#include "Utility.h"
#include "UtilityAlch.h"
#include "Stats.h"
#include "WorldspaceController.h"

namespace Events
{


	void Main::HandleActors()
	{
		// wait until processing is allowed, or we should kill ourselves
		while ((!CanProcess() || Game::IsFastTravelling()) && _handleactorsstop == false)
			std::this_thread::sleep_for(10ms);
		LOG_1("{}[Events] [HandleActors]");
		_handleactorsrunning = true;
		// static section
		RE::UI* ui = RE::UI::GetSingleton();
		auto begin = std::chrono::steady_clock::now();
		auto last = std::chrono::steady_clock::now();
		auto beginparticles = std::chrono::steady_clock::now();
		auto datahandler = RE::TESDataHandler::GetSingleton();
		RE::TESObjectCELL* cell = nullptr;
		RE::TESObjectREFR* refr = nullptr;
		RE::Actor* actor = nullptr;
		std::weak_ptr<ActorInfo> playerweak = data->FindActor(RE::PlayerCharacter::GetSingleton());

		std::unordered_set<uint32_t> visited;
		std::vector<std::shared_ptr<ActorInfo>> actors;
		std::vector<std::shared_ptr<ActorInfo>> actorsreduced;
		std::vector<std::shared_ptr<ActorInfo>> infected[Diseases::kMaxValue];
		std::vector<std::shared_ptr<ActorInfo>> allinfected;
		std::vector<Diseases::Disease> disvals;

		auto WaitForFastTravel = [&actors, &actorsreduced, &infected, &allinfected]() {
			if (Game::IsFastTravelling()) {
					while (Game::IsFastTravelling() && _handleactorsstop == false && CanProcess() == true)
						std::this_thread::sleep_for(10ms);
					if (_handleactorsstop || !CanProcess())
						return;
					auto itr = actors.begin();
					while (itr != actors.end())
					{
						if ((*itr)->IsValid())
							itr++;
						else
							itr = actors.erase(itr);
					}
					itr = actorsreduced.begin();
					while (itr != actorsreduced.end()) {
						if ((*itr)->IsValid())
							itr++;
						else
							itr = actorsreduced.erase(itr);
					}
					itr = allinfected.begin();
					while (itr != allinfected.end()) {
						if ((*itr)->IsValid())
							itr++;
						else
							itr = allinfected.erase(itr);
					}
					for (int i = 0; i < Diseases::kMaxValue; i++)
					{
						itr = infected[i].begin();
						while (itr != infected[i].end()) {
							if ((*itr)->IsValid())
								itr++;
							else
								itr = infected[i].erase(itr);
						}
					}
				} };

		while (_handleactorsstop == false) {
			if (!CanProcess())
				goto HandleActorsSkipIteration;
			if (Game::IsFastTravelling()) {
				LOG_2("{}[Events] [CheckActors] Skip iteration due to Fast Travel");
				goto HandleActorsSkipIteration;
			}

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

				//std::shared_ptr<CellInfo> cinfo;
				// get cell information
				if (std::shared_ptr<ActorInfo> playerinfo = playerweak.lock()) {
					actors.push_back(playerinfo);
				}

				// get weather information
				std::shared_ptr<WeatherInfo> winfo = data->FindWeather(RE::Sky::GetSingleton()->currentWeather);

				LOG_1("{}[Events] [HandleActors] get all actors");

				LOG1_1("{}[Events] [HandleActors] registered actors {}", acset.size());
				for (auto& weak : acset)
				{
					if (auto acinfo = weak.lock()) {
						actors.push_back(acinfo);
					}
				}
				LOG1_1("{}[Events] [HandleActors] valid actors {}", acset.size());

				// release lock
				sem.release();

				if (!CanProcess() || Game::IsFastTravelling())
					goto HandleActorsSkipIteration;
				if (IsPlayerDead())
					break;

				LOG_1("{}[Events] [HandleActors] create necessary vars");

				float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

				LOG_1("{}[Events] [HandleActors] calculate ticks for all actors");

				// calculate ticks for each actor and set current time as last change
#pragma omp parallel for num_threads(4) shared(currentgameday) schedule(runtime)
				for (int x = 0; x < actors.size(); x++) {
					// updates currently performed physical actions
					actors[x]->UpdateDynamicStats();
					actors[x]->UpdatePerformingPhysicalAction();
					actors[x]->SetDiseaseTicks((int)((currentgameday - actors[x]->GetDiseaseLastTime()) / Settings::System::_ticklength));
					//LOG4_1("{}[Events] [HandleActors] time: {}\tlast: {}\tticklength: {}\tticks: {}", currentgameday, actors[x]->GetDiseaseLastTime(), Settings::System::_ticklength, actors[x]->GetDiseaseTicks());
					if (actors[x]->GetDiseaseTicks() > 0)
						actors[x]->SetDiseaseLastTime(currentgameday);
				}

				beginparticles = std::chrono::steady_clock::now();

				LOG_1("{}[Events] [HandleActors] calculate particle effects");

				
#pragma omp parallel num_threads(4)
				{
					#pragma omp master
					allinfected = UtilityAlch::GetProgressingActors(actors);

					#pragma omp for
					for (int i = 0; i < Diseases::kMaxValue; i++)
						infected[i] = UtilityAlch::GetProgressingActors(actors, static_cast<Diseases::Disease>(i));
				}
				for (int i = 0; i < Diseases::kMaxValue; i++)
					if (infected[i].size() > 0)
						disvals.push_back(static_cast<Diseases::Disease>(i));

				// all of the following is calculated in ticks, so remove those who do not need to be processed
				auto itra = actors.begin();
				while (itra != actors.end()) {
					if (itra->get()->GetDiseaseTicks() != 0) {
						actorsreduced.push_back(*itra);
					}
					itra++;
				}

				if (actorsreduced.size() == 0)
					goto HandleActorsSkipIteration;

				// wait until fast travel is over to calculate tick
				WaitForFastTravel();
				if (!CanProcess() || _handleactorsstop)
					goto HandleActorsSkipIteration;

				PROF1_1("{}[Events] [HandleActors] execution time: Startup: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
				last = std::chrono::steady_clock::now();

				// first calculate effects occuring in particle range
				// 0) calculate actor pairs for infected
				// 1) calculate actor distances
				// 2) get those in particle range
				// particles work in a close range distance in any cells / between cells
				{
					LOG3_1("{}[Events] [HandleActors] calculate particle effects. actors {}, infected {}, actorsreduced {}", actors.size(), allinfected.size(), actorsreduced.size());
					// will be deleted after we leave the block
					auto list = UtilityAlch::GetActorDistancesList(allinfected, actorsreduced, Settings::Disease::_particleRange);
					//LOG1_1("{}[Events] [HandleActors] calculate particle effects for {} pairs", distances.size());
					//LOG1_1("{}[Events] [HandleActors] calculate particle effects for {} pairs", list.size());

					// iterate over illnesses
					for (auto disval : disvals) {
						//LOG1_1("{}[Events] [HandleEvents] Handling Particle disease: {}", UtilityAlch::ToString(disval));
						// get disease
						std::shared_ptr<Disease> dis = data->GetDisease(disval);
						if (!dis)
							continue;
						if (dis->ParticleSpread() == false) {
							//LOG1_1("{}[Events] [HandleEvents] Disease does not spread via particles: {}", UtilityAlch::ToString(disval));
							continue;  // disease does not spread via particles
						}
						std::shared_ptr<DiseaseStage> stage = nullptr;

						std::shared_ptr<ActorInfo> infec, act;
						std::shared_ptr<DiseaseInfo> dinfo;
						int idx2;
#pragma omp parallel for num_threads(4) schedule(runtime) private(infec, act, stage, dinfo) shared(dis, disval, list)
						for (int i = 0; i < list.size(); i++) {
							auto& [first, second] = list[i];
							//if (iter->first >> 32 > allinfected.size() || (uint32_t)(iter->first) > actors.size()) {
							if (first >> 32 > allinfected.size() || (uint32_t)(first) > actors.size()) {
								//iter++;
								continue;
							}
							//infec = allinfected[iter->first >> 32];
							infec = allinfected[first >> 32];
							dinfo = infec->FindDisease(disval);
							if (infec->IsInfectedProgressing(disval) == false || !dinfo) {
								//iter++;
								continue;
							}
							//LOG2_1("{}tried to access index: {} and {}", iter->first >> 32, (uint32_t)(iter->first));
							//LOG2_1("{}tried to access index: {} and {}", first >> 32, (uint32_t)(first));
							idx2 = (uint32_t)first;
							act = actorsreduced[idx2];
							float scale = 0;
							// get disease stage of infected
							stage = dis->_stages[dinfo->stage];
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
							scale = scale * (1 - act->dynamic._ignoresDisease);
							if (scale == 0)
								continue;  // got to next infected instead
							if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kParticle])) {
								actorsreduced[idx2]->AddDiseasePoints(disval, scale * std::get<1>(stage->_spreading[Spreading::kParticle]));
							}

						}
					}
				}

				PROF1_1("{}[Events] [HandleActors] execution time: Particle: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - last).count()));
				last = std::chrono::steady_clock::now();

				Stats::MainHandler_Particles_Times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count());

				// wait until fast travel is over to calculate tick
				WaitForFastTravel();
				if (!CanProcess() || _handleactorsstop)
					goto HandleActorsSkipIteration;
				
				LOG_1("{}[Events] [HandleActors] calculate air effects");

				// calculate effects occuring due to air infection
				// air infection may only happen in interior spaces
				{
					// get all npcs in the same interior spaces
					std::unordered_map<uint32_t, std::vector<int /*idx in actors*/>> cellmates;
#pragma omp parallel for num_threads(4) schedule(runtime)
					for (int i = 0; i < allinfected.size(); i++) {
						// skip those in exterior cells
						if (allinfected[i]->dynamic._parentCellInterior == false)
							continue;
						std::vector<int> vec;
						for (int x = 0; x < actorsreduced.size(); x++) {
							// skip if same actor or not in same cell
							if (allinfected[i]->dynamic._parentCellID != actorsreduced[x]->dynamic._parentCellID || allinfected[i] == allinfected[x])
								continue;
							vec.push_back(x);
						}
						if (vec.size() > 0) {
							#pragma omp critical (EventsHandleActorsCellMates)
							{
								cellmates.insert_or_assign(allinfected[i]->GetFormID(), std::vector<int>{});
							}
						}
					}

					// iterate over illnesses
					for (auto disval : disvals) {
						//LOG1_1("{}[Events] [HandleEvents] Handling Air disease: {}", UtilityAlch::ToString(disval));
						// get disease
						std::shared_ptr<Disease> dis = data->GetDisease(disval);
						if (!dis)
							continue;
						if (dis->AirSpread() == false)
							continue;  // disease does not spread via air
						std::shared_ptr<DiseaseStage> stage = nullptr;
						// iterate over infected for disease
						#pragma omp parallel private(stage) num_threads(4)
						for (int c = 0; c < infected[disval].size(); c++) {
							auto itr = cellmates.find(infected[disval][c]->GetFormID());
							if (itr != cellmates.end()) {
								auto& vec = itr->second;
								float scale = 0;
								//LOG1_1("{}[Events] [HandleEvents] Handling stage: {}", infected[disval][c]->FindDisease(disval)->stage);
								// get disease stage of infected
								stage = dis->_stages[infected[disval][c]->FindDisease(disval)->stage];
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
								// iterate over other actorsreduced
								for (int y = 0; y < vec.size(); y++) {
									// iterate for number of ticks for this actor
									for (int t = 0; t < actorsreduced[vec[y]]->GetDiseaseTicks(); t++) {
										// if chance succeeds
										if (float scl = scale * (1 - actors[vec[y]]->dynamic._ignoresDisease); scl != 0.0f)
											if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kAir]))
												actorsreduced[vec[y]]->AddDiseasePoints(disval, scl * std::get<1>(stage->_spreading[Spreading::kAir]));
									}
								}
							}
						}
					}
				}

				LOG3_1("{}[Events] [HandleActors] calculate static effects. actors {}, infected {}, actorsreduced {}", actors.size(), allinfected.size(), actorsreduced.size());

				PROF1_1("{}[Events] [HandleActors] execution time: Air: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - last).count()));
				last = std::chrono::steady_clock::now();

				// wait until fast travel is over to calculate tick
				WaitForFastTravel();
				if (!CanProcess() || _handleactorsstop)
					goto HandleActorsSkipIteration;

				// calculate static cell and weather effects
				// also calculate regular disease advancement while we are at it
				for (int i = 0; i < Diseases::kMaxValue; i++) {
					//LOG1_1("{}[Events] [HandleEvents] Handling Static disease: {}", UtilityAlch::ToString(static_cast<Diseases::Disease>(i)));
					// get disease
					Diseases::Disease disval = static_cast<Diseases::Disease>(i);
					std::shared_ptr<Disease> dis = data->GetDisease(disval);
					if (!dis) {
						//LOG_1("{}[Events] [HandleActors] skip disease");
						continue;
					}
					std::shared_ptr<DiseaseStage> stage = nullptr;
					// iterate actorsreduced
#pragma omp parallel for private(stage) num_threads(4) schedule(runtime) shared(disval, dis, actorsreduced)
					for (int x = 0; x < actorsreduced.size(); x++) {
						//LOG1_1("{}[Events] [HandleActors] actor: {}", actorsreduced[x]->GetName());
						auto dinfo = actorsreduced[x]->FindDisease(disval);
						if (!dinfo)
							stage = dis->_stageInfection;
						else
							stage = dis->_stages[dinfo->stage];
						// calc actor point scaling
						float scale = 1;
						scale = scale * (1 - actorsreduced[x]->dynamic._ignoresDisease);
						int ticks = actorsreduced[x]->GetDiseaseTicks();
						float points = 0;
						//LOG2_1("{}[Events] [HandleActors] base, ticks: {}, scale: {}", ticks, scale);
						if (scale > 0.0f) {
							//LOG1_1("{}[Events] [HandleActors] base, ticks: {}", ticks[x]);
							// iterate ticks
							if (ticks < 10) {
								// for only a few ticks, use actual probabilistic calculation, since we don't have that much to do
								for (int t = 0; t < ticks; t++) {
									// handle weather effects
									if (winfo->IsIntenseCold()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIntenseCold])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]);
									} else if (winfo->IsCold()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIsCold])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIsCold]);
									}
									if (winfo->IsIntenseHeat()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIntenseHeat])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]);
									} else if (winfo->IsHeat()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIsHeat])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]);
									}
									if (winfo->IsAshstorm()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInAshstorm])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInAshstorm]);
									}
									if (winfo->IsSandstorm()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInSandstorm])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInSandstorm]);
									}
									if (winfo->IsBlizzard()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInBlizzard])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInBlizzard]);
									}
									if (winfo->IsRain()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInRain])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInRain]);
									}
									if (winfo->IsStormy()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIsStormy])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIsStormy]);
									} else if (winfo->IsWindy()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIsWindy])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIsWindy]);
									}
									if (winfo->IsExtremeCondition())
									{
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kExtremeConditions])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kExtremeConditions]);
									}

									//  handle cell effects
									if (actorsreduced[x]->dynamic.cinfo->IsAshland()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInAshland])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInAshland]);
									}
									if (actorsreduced[x]->dynamic.cinfo->IsIntenseCold()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIntenseCold])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]);
									} else if (actorsreduced[x]->dynamic.cinfo->IsCold()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIsCold])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIsCold]);
									}
									if (actorsreduced[x]->dynamic.cinfo->IsIntenseHeat()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIntenseHeat])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]);
									} else if (actorsreduced[x]->dynamic.cinfo->IsHeat()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kIsHeat])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]);
									}
									if (actorsreduced[x]->dynamic.cinfo->IsDessert()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInDessert])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInDessert]);
									}
									if (actorsreduced[x]->dynamic.cinfo->IsSwamp()) {
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kInSwamp])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kInSwamp]);
									}
									if (actorsreduced[x]->dynamic.cinfo->IsExtremeCondition())
									{
										if (UtilityAlch::CalcChance(std::get<0>(stage->_spreading[Spreading::kExtremeConditions])))
											points += scale * std::get<1>(stage->_spreading[Spreading::kExtremeConditions]);
									}
								}
							} else {
								// if we have more ticks, use the limes, since we do not have the time to compute the result, and the actual result won't differ too much for large values
								// we may have thousands of ticks at once, so that may actually be a problem otherwise
								// handle weather effects
								if (winfo->IsIntenseCold()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]) * ticks;
								} else if (winfo->IsCold()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIsCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsCold]) * ticks;
								}
								if (winfo->IsIntenseHeat()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]) * ticks;
								} else if (winfo->IsHeat()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIsHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]) * ticks;
								}
								if (winfo->IsAshstorm()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInAshstorm]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInAshstorm]) * ticks;
								}
								if (winfo->IsSandstorm()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInSandstorm]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInSandstorm]) * ticks;
								}
								if (winfo->IsBlizzard()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInBlizzard]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInBlizzard]) * ticks;
								}
								if (winfo->IsRain()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInRain]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInRain]) * ticks;
								}
								if (winfo->IsStormy()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIsStormy]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsStormy]) * ticks;
								} else if (winfo->IsWindy()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIsWindy]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsWindy]) * ticks;
								}
								if (winfo->IsExtremeCondition()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kExtremeConditions]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInRain]) * ticks;
								}

								//  handle cell effects
								if (actorsreduced[x]->dynamic.cinfo->IsAshland()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInAshland]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInAshland]) * ticks;
								}
								if (actorsreduced[x]->dynamic.cinfo->IsIntenseCold()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseCold]) * ticks;
								} else if (actorsreduced[x]->dynamic.cinfo->IsCold()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIsCold]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsCold]) * ticks;
								}
								if (actorsreduced[x]->dynamic.cinfo->IsIntenseHeat()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIntenseHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIntenseHeat]) * ticks;
								} else if (actorsreduced[x]->dynamic.cinfo->IsHeat()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kIsHeat]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kIsHeat]) * ticks;
								}
								if (actorsreduced[x]->dynamic.cinfo->IsDessert()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInDessert]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInDessert]) * ticks;
								}
								if (actorsreduced[x]->dynamic.cinfo->IsSwamp()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kInSwamp]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInSwamp]) * ticks;
								}
								if (actorsreduced[x]->dynamic.cinfo->IsExtremeCondition()) {
									points += ((float)std::get<0>(stage->_spreading[Spreading::kExtremeConditions]) / 100) * scale * std::get<1>(stage->_spreading[Spreading::kInSwamp]) * ticks;
								}
							}

							if (stage->_spreading[Spreading::kActionPhysical].second > 0)
								points += scale * stage->_spreading[Spreading::kActionPhysical].second * actorsreduced[x]->GetPhysicalActions();
						}

						// handle static progression effects
						if (dinfo && dinfo->status == DiseaseStatus::kProgressing) {
							points += scale * dis->_baseProgressionPoints * ticks;
						}
						// handle static regression effects
						if (dinfo && dinfo->permanentModifiersPoints != 0)
							points += dinfo->permanentModifiersPoints * ticks;
						actorsreduced[x]->AddDiseasePoints(disval, points);
					}
				}

				// wait until fast travel is over to calculate tick
				WaitForFastTravel();
				if (!CanProcess() || _handleactorsstop)
					goto HandleActorsSkipIteration;

				PROF1_1("{}[Events] [HandleActors] execution time: Static: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - last).count()));
				last = std::chrono::steady_clock::now();

				#pragma omp parallel for num_threads(4) schedule(runtime)
				for (int i = 0; i < actors.size(); i++)
				{
					if (actors[i]->ProgressAllDiseases() && Settings::Disease::_AllowActorDeath) {
						logusage("[Events] [HandleActors] Actor {} has died from their disease", Utility::PrintFormNonDebug(actors[i]));
						actors[i]->Kill();
					}
				}

				PROF1_1("{}[Events] [HandleActors] execution time: Progress: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - last).count()));
				last = std::chrono::steady_clock::now();

				LOG_1("{}[Events] [HandleActors] kill actors");

				// now kill those who have to die from sickness MUHAHAHA
				for (int x = 0; x < actors.size(); x++) {
					//	if (kill[x] && !actors[x]->actor->IsDead())
					//actors[x]->actor->KillImmediate();
				}

				LOG_1("{}[Events] [HandleActors] delete vars");

				// endtime
				uint64_t runtime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

				// stat tracking
				if (actors.size())
					Stats::MainHandler_ActorsHandled = actors.size();
				if (runtime)
					Stats::MainHandler_TimeTaken = runtime;
				Stats::MainHandler_ActorsHandledTotal += actors.size();

				// write execution time of iteration
				PROF1_1("{}[Events] [HandleActors] execution time: {} µs", std::to_string(runtime));
				LOG1_1("{}[Events] [HandleActors] {} actors handled", std::to_string(actors.size()));

			} else {
				LOG_1("{}[Events] [HandleActors] Skip round.");
			}

HandleActorsSkipIteration:
			// reset lists for next iteration
			actors.clear();
			actorsreduced.clear();
			for (int i = 0; i < Diseases::kMaxValue; i++)
				infected[i].clear();
			allinfected.clear();
			disvals.clear();
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

	void Main::KillThreads()
	{
		_handleactorsstop = true;
		std::this_thread::sleep_for(10ms);
		if (_handleactors != nullptr)
			_handleactors->~thread();
		_handleactors = nullptr;
	}

	void Main::InitThreads()
	{
		if (_handleactors != nullptr) {
			// if the thread is there, then destroy and delete it
			// if it is joinable and not running it has already finished, but needs to be joined before
			// it can be destroyed savely
			_handleactors->~thread();
			delete _handleactors;
			_handleactors = nullptr;
		}
		_handleactors = new std::thread(HandleActors);
		_handleactors->detach();
	}

	void Main::LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [LoadGameCallback]");
		auto begin = std::chrono::steady_clock::now();
		// if we canceled the main thread, reset that
		_handleactorsstop = false;
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

		World::GetSingleton()->PlayerChangeCell(RE::PlayerCharacter::GetSingleton()->GetParentCell());

		
		// when loading the game, the attach detach events for actors aren't fired until cells have been changed
		// thus we need to get all currently loaded npcs manually
		RE::TESObjectCELL* cell = nullptr;
		std::vector<RE::TESObjectCELL*> gamecells;
		const auto& [hashtable, lock] = RE::TESForm::GetAllForms();
		{
			const RE::BSReadLockGuard locker{ lock };
			if (hashtable) {
				for (auto& [id, form] : *hashtable) {
					if (form) {
						cell = form->As<RE::TESObjectCELL>();
						if (cell) {
							gamecells.push_back(cell);
						}
					}
				}
			}
		}
		LOG1_1("{}[Events] [LoadGameSub] found {} cells", std::to_string(gamecells.size()));
		for (int i = 0; i < (int)gamecells.size(); i++) {
			if (gamecells[i]->IsAttached()) {
				for (auto& ptr : gamecells[i]->GetRuntimeData().references) {
					if (ptr.get()) {
						RE::Actor* actor = ptr.get()->As<RE::Actor>();
						if (Utility::ValidateActor(actor) && !Main::IsDead(actor) && !actor->IsPlayerRef()) {
							if (Distribution::ExcludedNPCFromHandling(actor) == false)
								RegisterNPC(actor);
						}
					}
				}
			}
		}

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
		KillThreads();
		LOG1_1("{}[PlayerDead] {}", playerdied);
		// reset actor processing list
		acset.clear();
		//cellist.clear();

		World::GetSingleton()->Reset();
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
				if (a_intfc->OpenRecord('DOED', 0)) {
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
