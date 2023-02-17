#include <string.h>
#include <chrono>
#include <thread>
#include <forward_list>
#include <semaphore>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <fstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <deque>

#include "ActorInfo.h"
#include "Game.h"
#include "UtilityAlch.h"
#include "Events.h"
#include "Settings.h"
#include "Data.h"
#include "Random.h"
#include "Stats.h"
		
namespace Events
{
	using AlchemyEffect = AlchemyEffect;
#define Base(x) static_cast<uint64_t>(x)

	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	static std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
	/// <summary>
	/// trims random numbers to 1 to 100
	/// </summary>
	static std::uniform_int_distribution<signed> rand100(1, 100);
	/// <summary>
	/// trims random numbers to 1 to 3
	/// </summary>
	static std::uniform_int_distribution<signed> rand3(1, 3);

#define Look(s) RE::TESForm::LookupByEditorID(s)

	static Data* data = nullptr;

	/// <summary>
	/// determines whether events and functions are run
	/// </summary>
	static bool initialized = false;

	/// <summary>
	/// enables all active functions
	/// </summary>
	static bool enableProcessing = false;
#define EvalProcessing()   \
	if (!enableProcessing) \
		return;
#define EvalProcessingEvent() \
	if (!enableProcessing)    \
		return EventResult::kContinue;
	bool CanProcess()
	{
		return enableProcessing;
	}

	/// <summary>
	/// Temporarily locks processing for all functions
	/// </summary>
	/// <returns></returns>
	bool LockProcessing()
	{
		bool val = enableProcessing;
		enableProcessing = false;
		return val;
	}
	/// <summary>
	/// Unlocks processing for all functions
	/// </summary>
	void UnlockProcessing()
	{
		enableProcessing = true;
	}

	/// <summary>
	/// list that holds currently handled cells
	/// </summary>
	static std::set<CellInfo*> cellist{};
	/// <summary>
	/// semaphore used to sync access to actor handling, to prevent list changes while operations are done
	/// </summary>
	static std::binary_semaphore sem(1);

	/// <summary>
	/// since the TESDeathEvent seems to be able to fire more than once for an actor we need to track the deaths
	/// </summary>
	static std::unordered_set<RE::FormID> deads;

	/// <summary>
	/// signals whether the player has died
	/// </summary>
	static bool playerdied = false;

	/// <summary>
	/// initializes importent variables, which need to be initialized every time a game is loaded
	/// </summary>
	void InitializeObjects()
	{ /*
		// now that the game was loaded we can try to initialize all our variables we conuldn't before
		if (!initialized) {
			// if we are in com mode, try to find the needed items. If we cannot find them, deactivate comp mode
			if (Settings::_CompatibilityPotionAnimatedFx) {
				RE::TESForm* tmp = RE::TESForm::LookupByEditorID(std::string_view{ Settings::Compatibility::PAF_NPCDrinkingCoolDownEffect_name });
				if (tmp)
					Settings::Compatibility::PAF_NPCDrinkingCoolDownEffect = tmp->As<RE::EffectSetting>();
				tmp = RE::TESForm::LookupByEditorID(std::string_view{ Settings::Compatibility::PAF_NPCDrinkingCoolDownSpell_name });
				if (tmp)
					Settings::Compatibility::PAF_NPCDrinkingCoolDownSpell = tmp->As<RE::SpellItem>();
				if (!(Settings::Compatibility::PAF_NPCDrinkingCoolDownEffect && Settings::Compatibility::PAF_NPCDrinkingCoolDownSpell)) {
					Settings::_CompatibilityPotionAnimatedFx = false;
					Settings::_CompatibilityPotionAnimatedFX_UseAnimations = false;
					logger::info("[INIT] Some Forms from PotionAnimatedfx.esp seem to be missing. Forcefully deactivated compatibility mode");
				}
			}
			initialized = true;
		}*/
		if (!initialized)
			initialized = true;
	}

#define ReEvalPlayerDeath                                         \
	if (RE::PlayerCharacter::GetSingleton()->IsDead() == false) { \
		playerdied = false;                                       \
	} else {                                                      \
		playerdied = true;                                        \
	}

#define CheckDeadCheckHandlerLoop \
	if (playerdied) {             \
		break;                    \
	}

	std::thread* _handleactors = nullptr;
	bool _handleactorsrunning;
	bool _handleactorsstop;

	bool particlehandling = true;

	void HandleActors()
	{
		EvalProcessing();
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
		RE::Actor* player = RE::PlayerCharacter::GetSingleton();

		std::unordered_set<uint32_t> visited;
		std::vector<ActorInfo*> actors;
		std::vector<ActorInfo*> infected[Diseases::kMaxValue];
		std::vector<ActorInfo*> allinfected;

		while (_handleactorsstop == false) {
			EvalProcessing();
			ActorInfo* curr;
			ReEvalPlayerDeath;
			// if we are in a paused menu (SoulsRE unpauses menus, which is supported by this) skip iter
			if (!ui->GameIsPaused() && initialized && !playerdied) {

				LOG_1("{}[Events] [HandleActors] begin round");

				// stat tracking
				Stats::MainHandler_Run++;

				begin = std::chrono::steady_clock::now();

				// get lock
				sem.acquire();
				// take iter time

				// get cell information
				CellInfo* cinfo = data->FindCell(player->GetParentCell());
				if (!cellist.contains(cinfo))
					cellist.insert(cinfo);

				// get weather information
				WeatherInfo* winfo = data->FindWeather(RE::Sky::GetSingleton()->currentWeather);

				LOG_1("{}[Events] [HandleActors] get all actors");
				
				auto citer = cellist.begin();
				while (citer != cellist.end()) {
					cinfo = *citer;
					// first get all actors that shall be handled in player cell
					auto iter = cinfo->cell->references.begin();
					while (iter != cinfo->cell->references.end()) {
						if (iter->get()) {
							actor = iter->get()->As<RE::Actor>();
							if (actor) {
								curr = data->FindActor(actor);
								// now do stuff to the actor
								actors.push_back(curr);
							}
						}
						iter++;
					}
					citer++;
				}

				// release lock
				sem.release();

				CheckDeadCheckHandlerLoop;

				LOG_1("{}[Events] [HandleActors] create necessary vars");

				float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

				float* points = new float[actors.size()];
				int* ticks = new int[actors.size()];
				bool* kill = new bool[actors.size()];

				LOG_1("{}[Events] [HandleActors] calculate ticks for all actors");

				// calculate ticks for each actor and set current time as last change
				for (int x = 0; x < actors.size(); x++) {
					ticks[x] = (int)((currentgameday - actors[x]->dinfo->LastGameTime) / Settings::AlchExtSettings::TickLength);
					//LOG4_1("{}[Events] [HandleActors] time: {}\tlast: {}\tticklength: {}\tticks: {}", currentgameday, actors[x]->dinfo->LastGameTime, Settings::AlchExtSettings::TickLength, ticks[x]);
					if (ticks[x] > 0)
						actors[x]->dinfo->LastGameTime = currentgameday;
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
					std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> distances = UtilityAlch::GetActorDistancesMap(allinfected, actors, Settings::AlchExtSettings::_particleRange);
					LOG1_1("{}[Events] [HandleActors] calculate particle effects for {} pairs", distances.size());

					// iterate over illnesses
					for (int i = 0; i < Diseases::kMaxValue; i++) {
						// get disease
						Diseases::Disease disval = static_cast<Diseases::Disease>(i);
						Disease* dis = data->GetDisease(disval);
						if (dis->ParticleSpread() == false) {
							//LOG1_1("{}[Events] [HandleEvents] Disease does not spread via particles: {}", UtilityAlch::ToString(disval));
							continue;  // disease does not spread via particles
						}
						DiseaseStage* stage = nullptr;

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
							ActorInfo *infec, *act;
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
								stage = dis->_stages[infec->dinfo->FindDisease(disval)->stage];
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
								if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kParticle]))
									points[idx2] += scale * (1 - act->IgnoresDisease()) * std::get<1>(stage->_spreading[Spreading::kParticle]);

								iter++;
							}

							// now we have calculated the points that will be added for the disease
							// add the points
							for (int x = 0; x < actors.size(); x++) {
								if (points[x] != 0) {
									kill[x] |= actors[x]->dinfo->ProgressDisease(actors[x]->actor, disval, points[x]);
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
						if (allinfected[i]->actor->GetParentCell()->IsInteriorCell() == false)
							continue;
						std::vector<int> vec;
						for (int x = 0; x < actors.size(); x++) {
							// skip if same actor or not in same cell
							if (allinfected[i]->actor->GetParentCell() != actors[x]->actor->GetParentCell() || allinfected[i] == allinfected[x])
								continue;
							vec.push_back(x);
						}
						if (vec.size() > 0)
							cellmates.insert_or_assign(allinfected[i]->actor->GetFormID(), std::vector<int>{});
					}

					// iterate over illnesses
					for (int i = 0; i < Diseases::kMaxValue; i++)
					{
						// get disease
						Diseases::Disease disval = static_cast<Diseases::Disease>(i);
						Disease* dis = data->GetDisease(disval);
						DiseaseStage* stage = nullptr;
						// iterate over infected for disease
						for (int c = 0; c < infected[i].size(); c++) {
							auto itr = cellmates.find(infected[i][c]->actor->GetFormID());
							if (itr != cellmates.end()) {
								auto vec = itr->second;
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
								for (int y = 0; y < vec.size(); y++) {
									// iterate for number of ticks for this actor
									for (int t = 0; t < ticks[vec[y]]; t++) {
										// if chance succeeds
										if (Random::rand100(Random::rand) < std::get<0>(stage->_spreading[Spreading::kParticle]))
											points[vec[y]] += scale * (1 - actors[vec[y]]->IgnoresDisease()) * std::get<1>(stage->_spreading[Spreading::kParticle]);
									}
								}
							}
						}
						// now we have calculated the points that will be added for the disease
						// add the points
						for (int x = 0; x < actors.size(); x++) {
							if (points[x] != 0) {
								kill[x] |= actors[x]->dinfo->ProgressDisease(actors[x]->actor, disval, points[x]);
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
					Disease* dis = data->GetDisease(disval);
					if (dis->AirSpread() == false)
						continue;  // disease does not spread via air
					DiseaseStage* stage = nullptr;
					DiseaseInfo* dinfo = nullptr;
					// iterate actors
					for (int x = 0; x < actors.size(); x++) {
						//LOG1_1("{}[Events] [HandleActors] actor: {}", actors[x]->actor->GetName());
						if (ticks[x] == 0)
							continue;
						dinfo = actors[x]->dinfo->FindDisease(disval);
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

								//  handle cell effects
								cinfo = data->FindCell(actors[x]->actor->GetParentCell());
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

							//  handle cell effects
							cinfo = data->FindCell(actors[x]->actor->GetParentCell());
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
						}
						// handle static progression effects
						if (dinfo && dinfo->status == DiseaseStatus::kProgressing) {
							//LOG3_1("{}[Events] [HandleActors] base, ticks: {}, ignore: {}, points: {}", ticks[x], std::to_string(actors[x]->IgnoresDisease()), std::to_string((1 - actors[x]->IgnoresDisease()) * dis->_baseProgressionPoints * ticks[x]));
							points[x] += (1 - actors[x]->IgnoresDisease()) * dis->_baseProgressionPoints * ticks[x];
						}
						if (dinfo && dinfo->permanentModifiersPoints != 0)
							points[x] += dinfo->permanentModifiersPoints * ticks[x];
					}
					// now we have calculated the points that will be added for the disease
					// add the points
					for (int x = 0; x < actors.size(); x++) {
						if (points[x] != 0) {
							kill[x] |= actors[x]->dinfo->ProgressDisease(actors[x]->actor, disval, points[x]);
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
				cinfo = data->FindCell(player->GetParentCell());
				if (cinfo->registeredactors == 0)
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


			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(Settings::AlchExtSettings::CycleTime));
		}

		LOG_1("{}[Events] [HandleActors] Exit.");
		_handleactorsrunning = false;
		_handleactorsstop = false;
	}


	/// <summary>
	/// Processes TESHitEvents
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* /*a_event*/, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [TESHitEvent]");
		// currently unused
		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for TESDeathEvent
	/// removed unused potions and poisons from actor, to avoid economy instability
	/// only registered if itemremoval is activated in the settings
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [DeathEvent]");
		ReEvalPlayerDeath;
		InitializeObjects();
		auto actor = a_event->actorDying->As<RE::Actor>();
		if (actor->IsPlayerRef()) {
			playerdied = true;
		} else
		if (actor && actor != RE::PlayerCharacter::GetSingleton()) {
			// as with potion distribution, exlude excluded actors and potential followers
			if (!Distribution::ExcludedNPC(actor) && deads.contains(actor->GetFormID()) == false) {
				// create and insert new event
				EvalProcessingEvent();
				ActorInfo* acinfo = data->FindActor(actor);
				deads.insert(actor->GetFormID());
				// distribute death items
				auto ditems = acinfo->FilterCustomConditionsDistrItems(acinfo->citems->death);
				// item, chance, num, cond1, cond2
				for (int i = 0; i < ditems.size(); i++) {
					// calc chances
					if (rand100(rand) <= std::get<1>(ditems[i])) {
						// distr item
						RE::ExtraDataList* extra = new RE::ExtraDataList();
						extra->SetOwner(actor);
						actor->AddObjectToContainer(std::get<0>(ditems[i]), extra, std::get<2>(ditems[i]), nullptr);
					}
				}
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Harvested Items
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHarvestedEvent::ItemHarvested* a_event, RE::BSTEventSource<RE::TESHarvestedEvent::ItemHarvested>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [HarvestEvent]");
		ReEvalPlayerDeath;
		// check that all event fields are valid
		if (a_event && a_event->produceItem && a_event->harvester) {
			LOG2_1("{}[Events] [HarvestEvent] harvested: {} {}", Utility::GetHex(a_event->produceItem->GetFormID()), a_event->produceItem->GetName());
			auto itr = Settings::AlchExtRuntime::harvestMap()->find(a_event->produceItem->GetFormID());
			if (itr != Settings::AlchExtRuntime::harvestMap()->end()) {
				auto vec = itr->second;
				auto iter = vec.begin();
				while (iter != vec.end()) {
					if (*iter == nullptr) {
						// error item not valid, clean up vector
						iter = vec.erase(iter);
						Settings::AlchExtRuntime::harvestMap()->insert_or_assign(a_event->produceItem->GetFormID(), vec);
					} else if ((*iter)->object == nullptr) {
						delete *iter;
						iter = vec.erase(iter);
						Settings::AlchExtRuntime::harvestMap()->insert_or_assign(a_event->produceItem->GetFormID(), vec);
					} else {
						for (int x = 0; x < (*iter)->num; x++) {
							if (rand100(rand) < (*iter)->chance) {
								RE::ExtraDataList* extra = new RE::ExtraDataList();
								extra->SetOwner(a_event->harvester);
								a_event->harvester->AddObjectToContainer((*iter)->object, extra, 1, nullptr);
							}
						}
					}
					iter++;
				}
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Debug purposes. It calculates the distribution rules for all npcs in the cell
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* /*a_event*/, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [EquipEvent]");
		ReEvalPlayerDeath;

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Actors being attached / detached
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
	{
		EvalProcessingEvent();
		//LOG_1("{}[Events] [TESCellAttachDetachEvent]");
		ReEvalPlayerDeath;

		if (a_event && a_event->reference) {
			RE::Actor* actor = a_event->reference->As<RE::Actor>();
			if (actor && deads.find(actor->GetFormID()) == deads.end() && !actor->IsDead() && !actor->IsPlayerRef()) {
				if (a_event->attached) {
					// the reference was attached
					LOG_3("{}[Events] [TESCellAttachDetachEvent] [Attach]");
					CellInfo* cinfo = data->FindCell(actor->GetParentCell());
					ActorInfo* acinfo = data->FindActor(actor);

					// register actor for handling
					sem.acquire();
					auto itra = cellist.begin();
					bool cont = false;
					if (!cellist.contains(cinfo)) {
						cellist.insert(cellist.begin(), cinfo);
					}
					cinfo->registeredactors++;

					sem.release();

					EvalProcessingEvent();

				} else {
					// the reference was detached
					LOG_3("{}[Events] [TESCellAttachDetachEvent] [Detach]");
					CellInfo* cinfo = data->FindCell(actor->GetParentCell());

					// unregister actor from handling
					sem.acquire();
					if (cellist.contains(cinfo)) {
						cinfo->registeredactors--;
						if (cinfo->registeredactors <= 0)
							cellist.erase(cinfo);
					}
					sem.release();
				}
			}
		}

		return EventResult::kContinue;
	}


	/// <summary>
	/// EventHandler for TESLoadGameEvent. Loads main thread
	/// </summary>
	/// <param name="">unused</param>
	/// <param name="">unused</param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		LOG_1("{}[Events] [LoadGameEvent]");
		// if we canceled the main thread, reset that
		initialized = false;
		// set player to alive
		ReEvalPlayerDeath;
		// reset the list of actors that died
		deads.clear();

		_handleactorsstop = true;
		if (_handleactors != nullptr) {
			if (_handleactors->joinable())
				_handleactors->join();
			delete _handleactors;
			_handleactors = nullptr;
		}
		_handleactorsstop = false;
		_handleactors = new std::thread(HandleActors);
		_handleactors->detach();
		
		enableProcessing = true;

		InitializeObjects();

		return EventResult::kContinue;
	}

	RE::Actor* target = nullptr;
	RE::TESObjectREFR* targetobj = nullptr;

	EventResult EventHandler::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* /*a_eventSource*/)
	{
		using EventType = RE::INPUT_EVENT_TYPE;
		using DeviceType = RE::INPUT_DEVICE;

		if (!a_event) {
			return EventResult::kContinue;
		}

		for (auto event = *a_event; event; event = event->next) {
			if (event->eventType != EventType::kButton) {
				continue;
			}

			auto& userEvent = event->QUserEvent();
			auto userEvents = RE::UserEvents::GetSingleton();

			auto button = static_cast<RE::ButtonEvent*>(event);
			if (button->IsDown()) {
				auto key = button->idCode;
				switch (button->device.get()) {
				case DeviceType::kMouse:
					key += 256;
					break;
				case DeviceType::kKeyboard:
					key += 0;
					break;
				default:
					continue;
				}

				// handle key here
				auto console = RE::ConsoleLog::GetSingleton();

				if (key == 0x52) { // numpad 0
					LOG_3("{}[Events] [InputEvent] registered target selection event");
					// lock target 
					targetobj = RE::CrosshairPickData::GetSingleton()->targetActor.get().get();
					if (targetobj) {
						target = targetobj->As<RE::Actor>();
						if (target) {
							console->Print(std::string("AlchExt: Changed target actor").c_str());
							return EventResult::kContinue;
						}
					}
					target = RE::PlayerCharacter::GetSingleton();
					console->Print(std::string("AlchExt: Changed target to player").c_str());
				} else if (key == 0x4f) {  // numpad 1
					LOG_3("{}[Events] [InputEvent] registered stage increase event");
					// increase stage target
					if (target) {
						ActorInfo* acinfo = data->FindActor(target);
						acinfo->dinfo->ForceIncreaseStage(target, Diseases::kAshWoeBlight);
						console->Print(std::string("AlchExt: Increased target stage").c_str());
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
				} else if (key == 0x50) {  // numpad 2
					LOG_3("{}[Events] [InputEvent] registered stage decrease event");
					// decrease stage target
					if (target) {
						ActorInfo* acinfo = data->FindActor(target);
						acinfo->dinfo->ForceDecreaseStage(target, Diseases::kAshWoeBlight);
						console->Print(std::string("AlchExt: Decreased target stage").c_str());
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
				} else if (key == 0x51) {  // numpad 3
					LOG_3("{}[Events] [InputEvent] registered print information event");
					// print disease information
					if (target) {
						ActorInfo* acinfo = data->FindActor(target);
						console->Print((std::string("AlchExt: Printing target information: \t") + target->GetName()).c_str());
						console->Print((std::string("Vampire: \t\t") + std::to_string(acinfo->_vampire)).c_str());
						console->Print((std::string("Automaton: \t\t") + std::to_string(acinfo->_automaton)).c_str());
						console->Print((std::string("Werewolf: \t\t") + std::to_string(acinfo->_werewolf)).c_str());
						console->Print((std::string("Printing disease information: \t")).c_str());
						for (int i = 0; i < acinfo->dinfo->diseases.size(); i++) {
							console->Print((std::string("\tDisease:\t\t\t") + data->GetDisease(acinfo->dinfo->diseases[i]->disease)->GetName()).c_str());
							console->Print((std::string("\t\tstatus:\t\t") + UtilityAlch::ToString(acinfo->dinfo->diseases[i]->status)).c_str());
							console->Print((std::string("\t\tstage:\t\t") + std::to_string(acinfo->dinfo->diseases[i]->stage)).c_str());
							console->Print((std::string("\t\tadvPoints:\t") + std::to_string(acinfo->dinfo->diseases[i]->advPoints)).c_str());
							console->Print((std::string("\t\tearladv:\t\t") + std::to_string(acinfo->dinfo->diseases[i]->earliestAdvancement)).c_str());
							console->Print((std::string("\t\timmuneuntil:\t") + std::to_string(acinfo->dinfo->diseases[i]->immuneUntil)).c_str());
							console->Print((std::string("\t\tpermanentMods:\t") + std::to_string(acinfo->dinfo->diseases[i]->permanentModifiers)).c_str());
							console->Print((std::string("\t\tpermanentPoints:\t") + std::to_string(acinfo->dinfo->diseases[i]->permanentModifiersPoints)).c_str());
						}
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
				} else if (key == 0x4b) {  // numpad 4
					LOG_3("{}[Events] [InputEvent] registered inc adv points event");
					if (target) {
						ActorInfo* acinfo = data->FindActor(target);
						bool kill = acinfo->dinfo->ProgressDisease(target, Diseases::kAshWoeBlight, 500);
						console->Print(std::string("AlchExt: Decreased target stage").c_str());
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
					// increase adv points by 500
				} else if (key == 0x4c) {  // numpad 5
					LOG_3("{}[Events] [InputEvent] registered print stats event");
					// print stats
					console->Print((std::string("MainHandler_Run:                 ") + std::to_string(Stats::MainHandler_Run)).c_str());
					console->Print((std::string("MainHandler_ActorsHandled:       ") + std::to_string(Stats::MainHandler_ActorsHandled)).c_str());
					console->Print((std::string("MainHandler_TimeTake:            ") + std::to_string(Stats::MainHandler_TimeTaken) + "us").c_str());
					console->Print((std::string("DiseaseStats_ProgressDisease:    ") + std::to_string(Stats::DiseaseStats_ProgressDisease)).c_str());
					console->Print((std::string("Average Particle Calc Time:      ") + std::to_string(UtilityAlch::Sum(Stats::MainHandler_Particles_Times) / Stats::MainHandler_Particles_Times.size())).c_str());
				} else if (key == 0x4d) {  // numpad 6
					LOG_3("{}[Events] [InputEvent] registered reset stats event");
					console->Print(std::string("Reset Stats").c_str());
					Stats::DiseaseStats_ProgressDisease = 0;
					Stats::MainHandler_ActorsHandled = 0;
					Stats::MainHandler_ActorsHandledTotal = 0;
					Stats::MainHandler_Run = 0;
					Stats::MainHandler_TimeTaken = 0;
					Stats::MainHandler_Particles_Times.clear();
				} else if (key == 0x47) {  // numpad 7
					LOG_3("{}[Events] [InputEvent] registered switch particle handler event");
					console->Print(std::string("Switched particle handler, deprecated").c_str());
					particlehandling = !particlehandling;
				} else if (key == 0x48) {  // numpad 8
				} else if (key == 0x49) {  // numpad 9
				}
			}
		}
		return EventResult::kContinue;
	}

	void LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [LoadGameCallback]");
	}

	void SaveGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [SaveGameCallback]");
	}

	void RevertGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [Events] [RevertGameCallback]");
		enableProcessing = false;
		// reset actor processing list
		cellist.clear();
		_handleactorsstop = true;
	}

    /// <summary>
    /// returns singleton to the EventHandler
    /// </summary>
    /// <returns></returns>
    EventHandler* EventHandler::GetSingleton()
    {
        static EventHandler singleton;
        return std::addressof(singleton);
    }

    /// <summary>
    /// Registers us for all Events we want to receive
    /// </summary>
	void EventHandler::Register()
	{
		auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		scriptEventSourceHolder->GetEventSource<RE::TESHitEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHitEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESLoadGameEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESLoadGameEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESEquipEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESEquipEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESDeathEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESDeathEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESCellAttachDetachEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESCellAttachDetachEvent).name());
		RE::TESHarvestedEvent::GetEventSource()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHarvestedEvent::ItemHarvested).name());
		RE::BSInputDeviceManager::GetSingleton()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::InputEvent).name());
		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000001, LoadGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForSaveCallback(0xFF000002, SaveGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForRevertCallback(0xFF000003, RevertGameCallback);
		data = Data::GetSingleton();
	}

	/// <summary>
	/// Registers all EventHandlers, if we would have multiple
	/// </summary>
	void RegisterAllEventHandlers()
	{
		EventHandler::Register();
		LOG_1("{}Registered all event handlers"sv);
	}

	/// <summary>
	/// sets the main threads to stop on the next iteration
	/// </summary>
	void DisableThreads()
	{
	}

}
