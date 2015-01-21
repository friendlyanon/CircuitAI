/*
 * UnitModule.h
 *
 *  Created on: Jan 20, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_MODULE_UNITMODULE_H_
#define SRC_CIRCUIT_MODULE_UNITMODULE_H_

#include "module/Module.h"
#include "unit/UnitManager.h"

namespace circuit {

class IUnitModule: public IModule, public IUnitManager {
protected:
	IUnitModule(CCircuitAI* circuit);
public:
	virtual ~IUnitModule();

	virtual CCircuitAI* GetCircuit();
};

} // namespace circuit

#endif // SRC_CIRCUIT_MODULE_UNITMODULE_H_
