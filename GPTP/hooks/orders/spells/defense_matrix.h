#pragma once
#include "SCBW/structures/CUnit.h"

namespace hooks {

	void orders_DefensiveMatrix(CUnit* unit);	//0x004F6D40

	void injectDefenseMatrixHook();

} //hooks