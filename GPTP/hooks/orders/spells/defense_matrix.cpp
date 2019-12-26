//Injector source file for the Hallucination Spell Order hook module.
#include "defense_matrix.h"
#include <hook_tools.h>
#include <SCBW/api.h>

namespace {
	bool ordersSpell_Sub_4926D0(CUnit* unit, u32 techId, u16* techEnergyCost, u32 sightRange, u32 error_message_index);	//926D0
	u8 calculateOverlaySize(CUnit* unit);
	void function_00498D70(CSprite* sprite, u32 imageId, u32 unk1, u32 unk2, u32 unk3);	//98D70
	void setNextWaypoint_Sub4EB290(CUnit* unit);										//0x004EB290
	u32 get_statTxt_Str_0(CUnit* unit, u32 playerId, char* message);					//0x0048EF30
	bool canCastSpell_0(CUnit* unit);													//0x00492140
	u32 function_00492620(CUnit* unit, Bool32 wasMainOrderStateZero);					//0x00492620
	void unitOrderMoveToTargetUnit(CUnit* unit, CUnit* target);							//79FE0
	void function_00494BB0(CUnit* unit, int x, int y);									//94BB0
	bool function_004EB900(CUnit* unit, CUnit* target);									//EB900
	bool orderToMoveToTarget(CUnit* unit, CUnit* target);								//EB980
}

namespace hooks {
	void orders_DefensiveMatrix(CUnit* unit) {
		int sightRange = unit->getSightRange(true);
		CUnit* target = unit->orderTarget.unit;

		//this if statement from cast_order
		if (!canCastSpell_0(unit)) {
			char* message = (char*)statTxtTbl->getString(876); //Invalid Target

			//do the actual transmission corresponding to the message
			get_statTxt_Str_0(unit, unit->playerId, message);

			//make the unit stop since the order failed
			setNextWaypoint_Sub4EB290(unit);
			unit->orderToIdle();
		}
		else if (unit->getDistanceToTarget(target) > sightRange * 32) {
			if (unit->mainOrderState == 0)
				unit->mainOrderState = 1;

			//below is from repair_order, not sure which are needed or not
			if (unit->mainOrderState == 1 && orderToMoveToTarget(unit, target)) {
				unitOrderMoveToTargetUnit(unit, target);
				unit->mainOrderState = 6; //don't continue directly into this case
			}
			else if (unit->mainOrderState == 6) {
				if (unit->getMovableState() == 0) { //not reached destination
					function_004EB900(unit, target); //may cause unit to move or become idle
				}
				else { //reached destination or unmovable
					function_00494BB0(unit, target->sprite->position.x, target->sprite->position.y);
					unitOrderMoveToTargetUnit(unit, target);
					unit->mainOrderState = 7; //don't continue directly into this case
				}
			}
			else if (unit->mainOrderState == 7 || unit->mainOrderState == 8) {
				function_00494BB0(unit, target->sprite->position.x, target->sprite->position.y);
				unitOrderMoveToTargetUnit(unit, target);
			}
		}
		else {
			unit->spendUnitEnergy(techdata_dat::EnergyCost[TechId::DefensiveMatrix] * 256);

			if (target->defensiveMatrixTimer <= 0 && !(target->status & UnitStatus::Burrowed)) {
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

			target->defensiveMatrixHp = 15;
			target->defensiveMatrixTimer = 168;

			scbw::playSound(SoundId::Bullet_tscFir00_wav, unit);
			unit->sprite->createTopOverlay(ImageId::ScienceVesselOverlay_Part2, 0, 0, 0);
			setNextWaypoint_Sub4EB290(unit);
			unit->mainOrderState = 2;
			unit->orderToIdle();
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

	//Related to path/movement decision
	const u32 Func_sub_4EB290 = 0x004EB290;
	void setNextWaypoint_Sub4EB290(CUnit* unit) {

		__asm {
			PUSHAD
			MOV EAX, unit
			CALL Func_sub_4EB290
			POPAD
		}
	}


	;

	const u32 Func_get_statTxt_Str_0 = 0x0048EF30;
	u32 get_statTxt_Str_0(CUnit* unit, u32 playerId, char* message) {

		static u32 return_value;

		__asm {
			PUSHAD
			MOV EAX, message
			MOV ECX, playerId
			MOV EDX, unit
			CALL Func_get_statTxt_Str_0
			MOV EAX, return_value
			POPAD
		}

		return return_value;

	}

	;

	const u32 Func_canCastSpell_0 = 0x00492140;
	bool canCastSpell_0(CUnit* unit) {

		static Bool32 bPreResult;

		__asm {
			PUSHAD
			MOV EDI, unit
			CALL Func_canCastSpell_0
			MOV bPreResult, EAX
			POPAD
		}

		//0 means "can cast spell"
		return (bPreResult == 0);

	}

	;

	const u32 Func_Sub492620 = 0x00492620;
	u32 function_00492620(CUnit* unit, Bool32 wasMainOrderStateZero) {

		static u32 return_value;

		__asm {
			PUSHAD
			PUSH wasMainOrderStateZero
			MOV EAX, unit
			CALL Func_Sub492620
			MOV return_value, EAX
			POPAD
		}

		return return_value;

	}

	;

	const u32 Func_Sub494BB0 = 0x00494BB0;
	void function_00494BB0(CUnit* unit, int x, int y) {

		__asm {
			PUSHAD
			MOV ECX, y
			MOV EDX, x
			MOV EAX, unit
			CALL Func_Sub494BB0
			POPAD
		}

	}

	;

	const u32 Func_Sub4EB900 = 0x004EB900;
	bool function_004EB900(CUnit* unit, CUnit* target) {

		static Bool32 bPreResult;

		__asm {
			PUSHAD
			MOV EAX, target
			MOV ECX, unit
			CALL Func_Sub4EB900
			MOV bPreResult, EAX
			POPAD
		}

		return (bPreResult != 0);

	}

	;

	const u32 Func_unitOrderMoveToTargetUnit = 0x00479FE0;
	void unitOrderMoveToTargetUnit(CUnit* unit, CUnit* target) {

		__asm {
			PUSHAD
			MOV EAX, unit
			MOV ECX, target
			CALL Func_unitOrderMoveToTargetUnit
			POPAD
		}

	}

	;


	const u32 Func__moveToTarget = 0x004EB980;
	bool orderToMoveToTarget(CUnit* unit, CUnit* target) {

		static Bool32 bPreResult;

		__asm {
			PUSHAD
			MOV EAX, target
			MOV ECX, unit
			CALL Func__moveToTarget
			MOV bPreResult, EAX
			POPAD
		}

		return bPreResult != 0;

	}

	;
}//unnamed namespace

namespace hooks {

	void injectDefenseMatrixHook() {
		jmpPatch(orders_DefenseMatrixWrapper, 0x004550A0, 0);
	}

} //hooks