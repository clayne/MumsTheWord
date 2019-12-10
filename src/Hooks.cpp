#include "Hooks.h"

#include "skse64_common/SafeWrite.h"

#include <vector>

#include "Settings.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace
{
	class PlayerCharacterEx : public RE::PlayerCharacter
	{
	public:
		void TryToSteal(RE::TESObjectREFR* a_fromRefr, RE::TESForm* a_item, RE::BaseExtraList* a_extraList)
		{
			if (!a_fromRefr || !aiProcess || !aiProcess->highProcess) {
				return;
			}

			if (Settings::useThreshold) {
				auto value = a_item->GetGoldValue();
				if (value > Settings::costThreshold) {
					return;
				}
			}

			bool stolen = false;
			auto owner = a_fromRefr->GetOwner();
			if (!owner && a_fromRefr->Is(RE::FormType::ActorCharacter) && !a_fromRefr->IsDead(true)) {
				stolen = a_fromRefr != this;
			} else if (owner && owner != this) {
				stolen = true;
				if (owner->Is(RE::FormType::Faction)) {
					auto faction = static_cast<RE::TESFaction*>(owner);
					if (IsInFaction(faction)) {
						stolen = false;
					}
				}
			}

			if (stolen) {
				auto highProcess = aiProcess->highProcess;
				std::vector<RE::NiPointer<RE::Actor>> actors;
				{
					RE::BSReadLockGuard locker(highProcess->knowledgeDataLock);
					for (auto& knowledgeData : highProcess->knowledgeData) {
						auto knowledge = knowledgeData.knowledge;
						if (knowledge) {
							auto akRef = RE::Actor::LookupByHandle(knowledge->toHandle);
							if (akRef) {
								actors.emplace_back(std::move(akRef));
							}
						}
					}
				}

				bool detected = false;
				for (auto& actor : actors) {
					if (!actor->IsPlayerTeammate() && actor->GetDetectionLevel(this) > 0) {
						detected = true;
						break;
					}
				}

				if (!detected) {
					auto xOwnership = a_extraList->GetByType<RE::ExtraOwnership>();
					if (xOwnership) {
						xOwnership->owner = this;
					} else {
						xOwnership = new RE::ExtraOwnership(this);
						a_extraList->Add(xOwnership);
					}
				}
			}
		}


		void Hook_AddItem(RE::TESForm* a_item, RE::BaseExtraList* a_extraList, SInt32 a_count, RE::TESObjectREFR* a_fromRefr)	// 5A
		{
			if (!a_extraList) {
				a_extraList = new RE::BaseExtraList();
			}

			TryToSteal(a_fromRefr, a_item, a_extraList);
			_AddItem(this, a_item, a_extraList, a_count, a_fromRefr);
		}


		void Hook_PickUpItem(TESObjectREFR* a_item, UInt32 a_count, bool a_arg3, bool a_playSound)	// CC
		{
			TryToSteal(a_item, a_item->baseForm, &a_item->extraData);
			_PickUpItem(this, a_item, a_count, a_arg3, a_playSound);
		}


		static void InstallHooks()
		{
			{
				REL::Offset<AddItem_t**> vFunc(RE::Offset::PlayerCharacter::Vtbl + (0x8 * 0x5A));
				_AddItem = *vFunc;
				SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(&PlayerCharacterEx::Hook_AddItem));
			}

			{
				REL::Offset<PickUpItem_t**> vFunc(RE::Offset::PlayerCharacter::Vtbl + (0x8 * 0xCC));
				_PickUpItem = *vFunc;
				SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(&PlayerCharacterEx::Hook_PickUpItem));
			}
		}


		using AddItem_t = function_type_t<decltype(&PlayerCharacterEx::AddItem)>;
		inline static AddItem_t* _AddItem = 0;

		using PickUpItem_t = function_type_t<decltype(&PlayerCharacterEx::PickUpItem)>;
		inline static PickUpItem_t* _PickUpItem = 0;
	};
}


void InstallHooks()
{
	PlayerCharacterEx::InstallHooks();
}
