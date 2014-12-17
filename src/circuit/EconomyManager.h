/*
 * EconomyManager.h
 *
 *  Created on: Sep 5, 2014
 *      Author: rlcevg
 */

#ifndef ECONOMYMANAGER_H_
#define ECONOMYMANAGER_H_

#include "Module.h"

#include "AIFloat3.h"

#include <vector>

namespace springai {
	class Resource;
	class Economy;
}

namespace circuit {

class CBuilderTask;
class CFactoryTask;

class CEconomyManager: public virtual IModule {
public:
	CEconomyManager(CCircuitAI* circuit);
	virtual ~CEconomyManager();

	virtual int UnitCreated(CCircuitUnit* unit, CCircuitUnit* builder);
	virtual int UnitFinished(CCircuitUnit* unit);
	virtual int UnitDestroyed(CCircuitUnit* unit, CCircuitUnit* attacker);

	CBuilderTask* CreateBuilderTask(CCircuitUnit* unit);
	CFactoryTask* CreateFactoryTask(CCircuitUnit* unit);
	springai::Resource* GetMetalRes();
	springai::Resource* GetEnergyRes();
	springai::AIFloat3 FindBuildPos(CCircuitUnit* unit);

private:
	void Init();
	CBuilderTask* UpdateMetalTasks();
	CBuilderTask* UpdateEnergyTasks();
	CBuilderTask* UpdateBuilderTasks();
	CFactoryTask* UpdateFactoryTasks();

	Handlers2 createdHandler;
	Handlers1 finishedHandler;
	Handlers2 destroyedHandler;

	springai::Resource* metalRes;
	springai::Resource* energyRes;
	springai::Economy* eco;

	struct ClusterInfo {
		CCircuitUnit* factory;
		CCircuitUnit* pylon;
	};
	std::vector<ClusterInfo> clusterInfo;
	int solarCount;
	int fusionCount;
	float pylonRange;
};

} // namespace circuit

#endif // ECONOMYMANAGER_H_
