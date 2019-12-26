//Injector source file for the Hallucination Spell Order hook module.
#include "defense_matrix.h"
#include <hook_tools.h>
#include <SCBW/api.h>



namespace {
	bool ordersSpell_Sub_4926D0(CUnit* unit, u32 techId, u16* techEnergyCost, u32 sightRange, u32 error_message_index);	//926D0
	u8 calculateOverlaySize(CUnit* unit);
	void function_00498D70(CSprite* sprite, u32 imageId, u32 unk1, u32 unk2, u32 unk3);	//98D70
}

namespace hooks {
	void orders_DefensiveMatrix(CUnit* unit) {
		int sightRange = unit->getSightRange(true);
		u16 techCost;

		if (ordersSpell_Sub_4926D0(
			unit,
			TechId::DefensiveMatrix,
			&techCost,
			sightRange * 32,
			880 //Invalid target
		)) {
			unit->spendUnitEnergy(techCost);

			CUnit* target = unit->orderTarget.unit;

			if (target->defensiveMatrixTimer > 0 || target->status & UnitStatus::Burrowed) {

			}
			else {
				u8 overlaySize = calculateOverlaySize(target);
				if (target->subunit != NULL) {
					target->subunit->sprite->createTopOverlay(overlaySize + ImageId::DefensiveMatrixFront_Small, 0, 0, 0);
				}
				else {
					target->sprite->createTopOverlay(overlaySize + ImageId::DefensiveMatrixFront_Small, 0, 0, 0);
				}
				//this is some other overlay creation function, not sure what the difference is?
				function_00498D70(target->sprite, overlaySize + ImageId::DefensiveMatrixBack_Small, 0, 0, 0);

			}

			target->defensiveMatrixHp = 0x0FA00;
			target->defensiveMatrixTimer = 0xA8;

			scbw::playSound(SoundId::Terran_VESSEL_TVeDef00_WAV, unit);
			unit->sprite->createTopOverlay(ImageId::ScienceVesselOverlay_Part2, 0, 0, 0);

			if (unit->orderQueueHead != NULL) {
				unit->userActionFlags |= 1;
				prepareForNextOrder(unit);
			}
			else
				if (unit->pAI != NULL)
					unit->orderComputerCL(OrderId::ComputerAI);
				else
					unit->orderComputerCL(units_dat::ReturnToIdleOrder[unit->id]);
		}


	}

} //namespace



namespace {

	void __declspec(naked) orders_DefenseMatrixWrapper() {

		static CUnit* unit;
		static CUnit* sourceUnit;

		__asm {
			push    ebp
			mov     ebp, esp
			push    ecx
			push    ebx
			push    esi
			mov     esi, [ebp + 8] // sourceUnit
			mov	 sourceUnit, esi
		}

		hooks::orders_DefensiveMatrix(sourceUnit);
		__asm {
			pop     esi
			pop     ebx
			mov     esp, ebp
			pop     ebp
			retn    4
		}

	}

	;

	const u32 Func_Sub_4926D0 = 0x004926D0;
	bool ordersSpell_Sub_4926D0(CUnit* unit, u32 techId, u16* techEnergyCost, u32 sightRange, u32 error_message_index) {

		static Bool32 bPreResult;

		__asm {
			PUSHAD
			PUSH techEnergyCost
			PUSH sightRange
			PUSH techId
			MOV EAX, error_message_index
			MOV EBX, unit
			CALL Func_Sub_4926D0
			MOV bPreResult, EAX
			POPAD
		}

		return (bPreResult != 0);

	}

	;

	const u32 Func_calcOverlay = 0x0047B720;
	u8 calculateOverlaySize(CUnit* unit) {

		static u8 result;

		__asm {
			PUSHAD
			MOV EAX, unit
			CALL Func_calcOverlay
			MOV result, AL
			POPAD
		}

		return result;

	}

	;

	const u32 Func_Sub498D70 = 0x00498D70;
	void function_00498D70(CSprite* sprite, u32 imageId, u32 unk1, u32 unk2, u32 unk3) {

		__asm {
			PUSHAD
			MOV EAX, sprite
			MOV ESI, imageId
			PUSH unk1
			PUSH unk2
			PUSH unk3
			CALL Func_Sub498D70
			POPAD
		}

	}

	;

}//unnamed namespace

namespace hooks {

	void injectDefenseMatrixHook() {
		jmpPatch(orders_DefenseMatrixWrapper, 0x004550A0, 0);
	}

} //hooks