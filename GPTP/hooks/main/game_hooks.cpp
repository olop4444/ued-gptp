/// This is where the magic happens; program your plug-in's core behavior here.

#include "game_hooks.h"
#include <graphics/graphics.h>
#include <SCBW/api.h>
#include "../psi_field.h"
#include <cstdio>
#include <unordered_set>

//Helper functions
namespace {
	void getShieldableUnits(CUnit* shielder, std::unordered_set<CUnit*>* shieldableUnits);
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
		}

		//Loop through all visible units in the game.
		bool hasShieldGen[12] = { 0 };
		std::unordered_set<CUnit*> terranUnits;
		std::unordered_set<CUnit*> shieldableUnits;
		std::unordered_set<CUnit*> shielders;
		for (CUnit* unit = *firstVisibleUnit; unit; unit = unit->link.next) {
			if (unit->id == UnitId::TerranArmory && unit->remainingBuildTime == 0) {
				hasShieldGen[unit->getLastOwnerId()] = true;
				shielders.insert(unit);
			}
			//unused terran1 = starbase = support tower
			/*else if (unit->id == UnitId::UnusedTerran1 && unit->remainingBuildTime == 0) {
				shielders.insert(unit);
			}*/
			else {
				if (unit->getRace() == RaceId::Terran &&
					units_dat::ShieldsEnabled[unit->id] != 0 &&
					unit->shields > 0) {
					terranUnits.insert(unit);
				}
			}
		}

		for (CUnit* shielder : shielders) {
			if (hasShieldGen[shielder->getLastOwnerId()] == true) {
				getShieldableUnits(shielder, &shieldableUnits);
			}
		}
		for (CUnit* unit : shieldableUnits) {
			s32 maxShields = (s32)(units_dat::MaxShieldPoints[unit->id]) * 256;

			if (unit->shields != maxShields) {

				unit->shields += 7;

				if (unit->shields > maxShields)
					unit->shields = maxShields;

				if (unit->sprite->flags & CSprite_Flags::Selected)  //If the unit is currently selected, redraw its graphics
					setAllImageGroupFlagsPal11(unit->sprite);
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
	void getShieldableUnits(CUnit* shielder, std::unordered_set<CUnit*>* shieldableUnits) {
		static u16* const maxBoxRightValue = (u16*)0x00628450;	//should usually be mapTileSize->width * 32
		static u16* const maxBoxBottomValue = (u16*)0x006284B4;	//should usually be mapTileSize->height * 32

		Box16 area_of_effect;

		CUnit** unitsInAreaOfEffect;
		CUnit* current_unit;

		//Range of 8
		u32 shieldRadius = 8 * 32;

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
				current_unit->getLastOwnerId() == shielder->getLastOwnerId()) {
				shieldableUnits->insert(current_unit);
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