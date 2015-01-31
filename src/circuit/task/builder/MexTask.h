/*
 * MexTask.h
 *
 *  Created on: Jan 31, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_TASK_BUILDER_MEXTASK_H_
#define SRC_CIRCUIT_TASK_BUILDER_MEXTASK_H_

#include "task/builder/BuilderTask.h"

namespace circuit {

class CBMexTask: public IBuilderTask {
public:
	CBMexTask(CCircuitAI* circuit, Priority priority,
			  springai::UnitDef* buildDef, const springai::AIFloat3& position,
			  BuildType type, float cost, int timeout);
	virtual ~CBMexTask();

	virtual void Execute(CCircuitUnit* unit);
};

} // namespace circuit

#endif // SRC_CIRCUIT_TASK_BUILDER_MEXTASK_H_
