/*
 * UnitManager.cpp
 *
 *  Created on: Jan 15, 2015
 *      Author: rlcevg
 */

#include "unit/UnitManager.h"
#include "CircuitAI.h"
#include "task/IdleTask.h"
#include "task/RetreatTask.h"

namespace circuit {

IUnitManager::IUnitManager() :
		idleTask(new CIdleTask),
		retreatTask(new CRetreatTask)
{
}

IUnitManager::~IUnitManager()
{
	delete idleTask, retreatTask;
}

CIdleTask* IUnitManager::GetIdleTask()
{
	return idleTask;
}

CRetreatTask* IUnitManager::GetRetreatTask()
{
	return retreatTask;
}

} // namespace circuit
