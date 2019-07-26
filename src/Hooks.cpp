#include "Hooks.h"

#include "skse64_common/SafeWrite.h"

#include "Settings.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace
{
	class PlayerCharacterEx : public RE::PlayerCharacter
	{
	public:
		void TryToSteal(RE::TESObjectREFR* a_containerOrItem, RE::BaseExtraList* a_extraList)
		{
			if (!a_containerOrItem) {
				return;
			}

			if (Settings::useThreshold) {
				auto value = a_containerOrItem->baseForm->GetValue();
				if (value > Settings::costThreshold) {
					return;
				}
			}

			bool stolen = false;
			auto owner = a_containerOrItem->GetOwner();
			if (owner && owner != this) {
				stolen = true;
				if (owner->Is(RE::FormType::Faction)) {
					auto faction = static_cast<RE::TESFaction*>(owner);
					if (IsInFaction(faction)) {
						stolen = false;
					}
				}
			}

			if (stolen) {
				bool detected = false;
				auto data010 = processManager->unk010;
				RE::BSReadLockGuard locker(data010->knowledgeDataLock);
				for (auto& knowledgeData : data010->knowledgeData) {
					auto knowledge = knowledgeData.knowledge;
					if (knowledge) {
						RE::TESObjectREFRPtr refr;
						RE::TESObjectREFR::LookupByHandle(knowledge->toHandle, refr);
						auto akRef = static_cast<RE::Actor*>(refr.get());
						if (akRef) {
							if (GetDetectionLevel(akRef) > 0 || akRef->GetDetectionLevel(this) > 0) {
								detected = true;
								break;
							}
						}
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

			TryToSteal(a_fromRefr, a_extraList);
			_AddItem(this, a_item, a_extraList, a_count, a_fromRefr);
		}


		void Hook_PickUpItem(TESObjectREFR* a_item, UInt32 a_count, bool a_arg3, bool a_playSound)	// CC
		{
			TryToSteal(a_item, &a_item->extraData);
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
