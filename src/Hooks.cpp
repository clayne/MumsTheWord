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
		void TryToSteal(RE::TESObjectREFR* a_fromRefr, RE::TESForm* a_item, RE::ExtraDataList* a_extraList)
		{
			if (!a_fromRefr || !currentProcess || !currentProcess->high) {
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
				auto high = currentProcess->high;
				std::vector<RE::NiPointer<RE::Actor>> actors;
				{
					RE::BSReadLockGuard locker(high->knowledgeLock);
					for (auto& knowledgeData : high->knowledgeArray) {
						auto& knowledge = knowledgeData.second;
						if (knowledge) {
							auto akRef = knowledge->target.get();
							if (akRef) {
								actors.emplace_back(std::move(akRef));
							}
						}
					}
				}

				bool detected = false;
				for (auto& actor : actors) {
					if (!actor->IsPlayerTeammate() && !actor->IsDead(true) && actor->RequestDetectionLevel(this) > 0) {
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


		void Hook_AddObjectToContainer(RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraList, SInt32 a_count, RE::TESObjectREFR* a_fromRefr)	// 5A
		{
			if (!a_extraList) {
				a_extraList = new RE::ExtraDataList();
			}

			TryToSteal(a_fromRefr, a_object, a_extraList);
			_AddObjectToContainer(this, a_object, a_extraList, a_count, a_fromRefr);
		}


		void Hook_PickUpObject(TESObjectREFR* a_item, UInt32 a_count, bool a_arg3, bool a_playSound)	// CC
		{
			TryToSteal(a_item, a_item->GetBaseObject(), &a_item->extraList);
			_PickUpObject(this, a_item, a_count, a_arg3, a_playSound);
		}


		static void InstallHooks()
		{
			{
				REL::Offset<AddObjectToContainer_t**> vFunc(RE::Offset::PlayerCharacter::Vtbl + (0x8 * 0x5A));
				_AddObjectToContainer = *vFunc;
				SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(AddObjectToContainer_f));
			}

			{
				REL::Offset<PickUpObject_t**> vFunc(RE::Offset::PlayerCharacter::Vtbl + (0x8 * 0xCC));
				_PickUpObject = *vFunc;
				SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(PickUpObject_f));
			}
		}


		static inline auto AddObjectToContainer_f = &PlayerCharacterEx::Hook_AddObjectToContainer;
		using AddObjectToContainer_t = function_type_t<decltype(AddObjectToContainer_f)>;
		inline static AddObjectToContainer_t* _AddObjectToContainer = 0;

		static inline auto PickUpObject_f = &PlayerCharacterEx::Hook_PickUpObject;
		using PickUpObject_t = function_type_t<decltype(PickUpObject_f)>;
		inline static PickUpObject_t* _PickUpObject = 0;
	};
}


void InstallHooks()
{
	PlayerCharacterEx::InstallHooks();
}
