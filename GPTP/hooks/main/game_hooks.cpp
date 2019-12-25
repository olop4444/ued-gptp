/// This is where the magic happens; program your plug-in's core behavior here.

#include "game_hooks.h"
#include <graphics/graphics.h>
#include <SCBW/api.h>
#include "../psi_field.h"
#include <cstdio>
#include <unordered_set>
#include <algorithm>

//Helper functions
namespace {
	void getShieldableUnits(CUnit* shielder);
	void setAllImageGroupFlagsPal11(CSprite* sprite);		//0x00497430
	CUnit** getAllUnitsInBounds(Box16* coords);		//0x0042FF80
} //unnamed namespace

namespace hooks {

/// This hook is called every frame; most of your plugin's logic goes here.
bool nextFrame() {

	if (!scbw::isGamePaused()) { //If the game is not paused

		scbw::setInGameLoopState(true); //Needed for scbw::random() to work
		graphics::resetAllGraphics();
		hooks::updatePsiFieldProviders();

		//This block is executed once every game.
		if (*elapsedTimeFrames == 0) {
			//Write your code here
			//scbw::printText(PLUGIN_NAME ": Hello, world!");
			for (int i = 0; i < PLAYER_COUNT; i++) {
				UpgradesSc->maxLevel[i][ScUpgrade::AegisProtection] = 1;
			}
		}

		//Loop through all visible units in the game.
		bool hasShieldGen[12] = { 0 };
		std::unordered_set<CUnit*> terranUnits;
		std::unordered_set<CUnit*> shielders;
		for (CUnit* unit = *firstVisibleUnit; unit; unit = unit->link.next) {
			//support tower needs to use the field for shield aura
			if (*elapsedTimeFrames % 8 == 0 && unit->id != UnitId::TerranStarbase) {
				unit->unusedTimer = 0;
			}
			else if (unit->id == UnitId::TerranGoliath &&
				     UpgradesSc->currentLevel[unit->playerId][ScUpgrade::AegisProtection] == 1) {
				if (unit->_unused_0x106 >= 3) {
					unit->_padding_0x132 = 240; //10 second delay, change value in weapon_damage if this changes
				} else if (unit->_padding_0x132 <= 0) {
					if (unit->defensiveMatrixTimer == 0 && unit->defensiveMatrixHp == 0) {
						unit->sprite->createTopOverlay(ImageId::DefensiveMatrixFront_Small, 0, 0, 0);
						unit->sprite->createOverlay(ImageId::DefensiveMatrixBack_Small, 0, 0, 0);
					}
					unit->defensiveMatrixHp++;
					if (unit->defensiveMatrixTimer == 0)
						unit->defensiveMatrixTimer = 168;
					unit->_unused_0x106++;
					unit->_padding_0x132 = 240; //10 second delay, change value in weapon_damage if this changes
				}
				else {
					unit->_padding_0x132--;
				}
			}


			if (unit->id == UnitId::TerranArmory && (unit->status & UnitStatus::Completed)) {
				hasShieldGen[unit->getLastOwnerId()] = true;
				shielders.insert(unit);
			}
			else if (unit->id == UnitId::TerranStarbase && (unit->status & UnitStatus::Completed)) {
				shielders.insert(unit);
			}
			else {
				if (unit->getRace() == RaceId::Terran &&
					units_dat::ShieldsEnabled[unit->id] != 0 &&
					unit->shields > 0) {
					terranUnits.insert(unit);
				}
			}
		}
		if (*elapsedTimeFrames % 8 == 0) {
			for (CUnit* shielder : shielders) {
				if (hasShieldGen[shielder->getLastOwnerId()] == true) {
					getShieldableUnits(shielder);
				}
				else {
					shielder->unusedTimer = 0;
					shielder->sprite->removeOverlay(ImageId::RechargeShields_Medium);
				}
			}
		}

		for (CUnit* unit : terranUnits) {
			if (!hasShieldGen[unit->getLastOwnerId()]) {
				unit->shields = 0;

				if (unit->sprite->flags & CSprite_Flags::Selected)  //If the unit is currently selected, redraw its graphics
					setAllImageGroupFlagsPal11(unit->sprite);
			}
		}

		scbw::setInGameLoopState(false);

	}

	return true;
}

bool gameOn() {
	return true;
}

bool gameEnd() {
	return true;
}

} //hooks

namespace {
	void getShieldableUnits(CUnit* shielder) {
		static u16* const maxBoxRightValue = (u16*)0x00628450;	//should usually be mapTileSize->width * 32
		static u16* const maxBoxBottomValue = (u16*)0x006284B4;	//should usually be mapTileSize->height * 32
		static u16 shieldRadius = 8 * 32;

		if (shielder->unusedTimer > 0) {
			shielder->unusedTimer = 1; //base energy cost same as cloaking, final cost is 10 x timer (to avoid overflow in the u8 field)
			if (*elapsedTimeFrames % 72 == 0) { // every 3 seconds at fastest
				scbw::playSound(SoundId::Bullet_pshield_wav, shielder);
			}
		}

		Box16 area_of_effect;

		CUnit** unitsInAreaOfEffect;
		CUnit* current_unit;

		//Range of 8

		area_of_effect.left = shielder->sprite->position.x - shieldRadius;
		area_of_effect.right = shielder->sprite->position.x + shieldRadius;
		area_of_effect.top = shielder->sprite->position.y - shieldRadius;
		area_of_effect.bottom = shielder->sprite->position.y + shieldRadius;

		if (area_of_effect.left < 0)
			area_of_effect.left = 0;
		if (area_of_effect.top < 0)
			area_of_effect.top = 0;
		if (area_of_effect.right > * maxBoxRightValue)
			area_of_effect.right = *maxBoxRightValue;
		if (area_of_effect.bottom > * maxBoxBottomValue)
			area_of_effect.bottom = *maxBoxBottomValue;

		unitsInAreaOfEffect = getAllUnitsInBounds(&area_of_effect);

		current_unit = *unitsInAreaOfEffect;

		while (current_unit != NULL) {
			if (current_unit->getRace() == RaceId::Terran &&
				units_dat::ShieldsEnabled[current_unit->id] != 0 &&
				current_unit->getLastOwnerId() == shielder->getLastOwnerId())
			{
				current_unit->unusedTimer = std::max(current_unit->unusedTimer, (u8)7);

				if (shielder->unusedTimer > 0) {
					current_unit->unusedTimer += 14;

					s32 maxShields = (s32)(units_dat::MaxShieldPoints[current_unit->id]) * 256;
					if (current_unit->shields != maxShields) {
						shielder->unusedTimer++; // 1 energy : 1.4 shield point, final cost is 10 x timer
					}
				}
			}
			unitsInAreaOfEffect++;
			current_unit = *unitsInAreaOfEffect;
		}

		*tempUnitsListCurrentArrayCount = tempUnitsListArraysCountsListLastIndex[*tempUnitsListArraysCountsListLastIndex];
		*tempUnitsListArraysCountsListLastIndex = *tempUnitsListArraysCountsListLastIndex - 1;
	}

	void setAllImageGroupFlagsPal11(CSprite* sprite) {

		for (
			CImage* current_image = sprite->images.head;
			current_image != NULL;
			current_image = current_image->link.next
			)
		{
			if (current_image->paletteType == PaletteType::RLE_HPFLOATDRAW)
				current_image->flags |= CImage_Flags::Redraw;
		}

	};

	const u32 Func_GetAllUnitsInBounds = 0x0042FF80;
	CUnit** getAllUnitsInBounds(Box16* coords) {

		static CUnit** units_in_bounds;

		__asm {
			PUSHAD
			MOV EAX, coords
			CALL Func_GetAllUnitsInBounds
			MOV units_in_bounds, EAX
			POPAD
		}

		return units_in_bounds;

	};
}