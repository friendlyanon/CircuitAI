/*
 * EnergyTask.cpp
 *
 *  Created on: Jan 30, 2015
 *      Author: rlcevg
 */

#include "task/builder/EnergyTask.h"
#include "task/TaskManager.h"
#include "module/EconomyManager.h"
#include "unit/CircuitUnit.h"
#include "CircuitAI.h"
#include "util/Utils.h"

namespace circuit {

using namespace springai;

CBEnergyTask::CBEnergyTask(ITaskManager* mgr, Priority priority,
						   CCircuitDef* buildDef, const AIFloat3& position,
						   float cost, float shake, int timeout)
		: IBuilderTask(mgr, priority, buildDef, position, Type::BUILDER, BuildType::ENERGY, cost, shake, timeout)
		, isStalling(false)
{
}

CBEnergyTask::CBEnergyTask(ITaskManager* mgr)
		: IBuilderTask(mgr, Type::BUILDER, BuildType::ENERGY)
		, isStalling(false)
{
}

CBEnergyTask::~CBEnergyTask()
{
}

void CBEnergyTask::Update()
{
	IBuilderTask::Update();
	if (units.empty() || (target == nullptr)) {
		return;
	}

	CCircuitAI* circuit = manager->GetCircuit();
	bool isEnergyStalling = circuit->GetEconomyManager()->IsEnergyStalling();
	if (isStalling == isEnergyStalling) {
		return;
	}
	isStalling = isEnergyStalling;
	priority = isEnergyStalling ? IBuilderTask::Priority::HIGH : IBuilderTask::Priority::NORMAL;
	TRY_UNIT(circuit, target,
		target->CmdPriority(ClampPriority());
	)
}

void CBEnergyTask::Finish()
{
	manager->GetCircuit()->GetEconomyManager()->ClearEnergyRequired();

	IBuilderTask::Finish();
}

void CBEnergyTask::Cancel()
{
	manager->GetCircuit()->GetEconomyManager()->ClearEnergyRequired();

	IBuilderTask::Cancel();
}

#define SERIALIZE(stream, func)	\
	utils::binary_##func(stream, isStalling);

bool CBEnergyTask::Load(std::istream& is)
{
	IBuilderTask::Load(is);
	SERIALIZE(is, read)
	return true;
}

void CBEnergyTask::Save(std::ostream& os) const
{
	IBuilderTask::Save(os);
	SERIALIZE(os, write)
}

} // namespace circuit
