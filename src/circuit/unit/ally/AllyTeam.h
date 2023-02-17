/*
 * AllyTeam.h
 *
 *  Created on: Apr 7, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_SETUP_ALLYTEAM_H_
#define SRC_CIRCUIT_SETUP_ALLYTEAM_H_

#include "unit/enemy/EnemyManager.h"
#include "util/math/QuadField.h"
#include "util/math/Region.h"

#include <memory>
#include <map>
#include <unordered_set>

namespace springai {
	class AIFloat3;
	class Unit;
}

namespace terrain {
	struct SArea;
}

namespace circuit {

class CCircuitAI;
class CAllyUnit;
class CMapManager;
class CMetalManager;
class CEnergyManager;
class CEnergyGrid;
class CDefenceData;
class CPathFinder;
class CFactoryData;

class CAllyTeam {
public:
	using Id = int;
	using AllyUnits = std::map<ICoreUnit::Id, CAllyUnit*>;
	using TeamIds = std::unordered_set<Id>;
	struct SClusterTeam {
		SClusterTeam(int tid, unsigned cnt = 0) : teamId(tid), count(cnt) {}
		int teamId;  // cluster leader
		unsigned count;  // number of commanders
	};
	struct SAreaTeam {
		SAreaTeam(int tid) : teamId(tid) {}
		int teamId;  // area leader
	};

public:
	CAllyTeam(const TeamIds& tids, const utils::CRegion& sb);
	~CAllyTeam();

	int GetSize() const { return teamIds.size(); }
	int GetAliveSize() const { return GetSize() - resignSize; }
	const TeamIds& GetTeamIds() const { return teamIds; }
	const Id GetLeaderId() const;
	const CCircuitAI* GetLeader() const { return circuit; }
	const utils::CRegion& GetStartBox() const { return startBox; }

	void Init(CCircuitAI* circuit, float decloakRadius);
	void NonDefaultThreats(std::set<CCircuitDef::RoleT>&& modRoles, CCircuitAI* ai);
	void Release();

	void ForceUpdateFriendlyUnits();
	void UpdateFriendlyUnits();
	CAllyUnit* GetFriendlyUnit(ICoreUnit::Id unitId) const;
	const AllyUnits& GetFriendlyUnits() const { return friendlyUnits; }

	const std::set<CEnemyUnit*>& GetDyingEnemies() const { return enemyManager->GetDyingEnemies(); }
	void DyingEnemy(CEnemyUnit* enemy, int frame) { enemyManager->DyingEnemy(enemy, frame); }
	bool EnemyInLOS(CEnemyUnit* data, CCircuitAI* ai);
	std::pair<CEnemyUnit*, bool> RegisterEnemyUnit(ICoreUnit::Id unitId, bool isInLOS, CCircuitAI* ai);
	CEnemyUnit* RegisterEnemyUnit(springai::Unit* e, CCircuitAI* ai);
	void UnregisterEnemyUnit(CEnemyUnit* data, CCircuitAI* ai);

	void RegisterEnemyFake(CCircuitDef::Id unitDefId, const springai::AIFloat3& pos, int timeout);
	void UnregisterEnemyFake(CEnemyFake* data);

	void EnemyEnterLOS(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyLeaveLOS(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyEnterRadar(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyLeaveRadar(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyDestroyed(CEnemyUnit* enemy, CCircuitAI* ai);

	void UpdateInLOS(CEnemyUnit* data, CCircuitDef::Id unitDefId);

	void Update(CCircuitAI* ai);
	void EnqueueUpdate();

	CEnemyUnit* GetEnemyOrFakeIn(const springai::AIFloat3& startPos, const springai::AIFloat3& dir, float length,
			const springai::AIFloat3& enemyPos, float radius, const std::set<CCircuitDef::Id>& unitDefIds);

	const std::shared_ptr<CMapManager>&    GetMapManager()    const { return mapManager; }
	const std::shared_ptr<CEnemyManager>&  GetEnemyManager()  const { return enemyManager; }
	const std::shared_ptr<CMetalManager>&  GetMetalManager()  const { return metalManager; }
	const std::shared_ptr<CEnergyManager>& GetEnergyManager() const { return energyManager; }
	const std::shared_ptr<CEnergyGrid>&    GetEnergyGrid()    const { return energyGrid; }
	const std::shared_ptr<CDefenceData>&   GetDefenceData()   const { return defence; }
	const std::shared_ptr<CPathFinder>&    GetPathfinder()    const { return pathfinder; }
	const std::shared_ptr<CFactoryData>&   GetFactoryData()   const { return factoryData; }

	void OccupyCluster(int clusterId, int teamId);
	SClusterTeam GetClusterTeam(int clusterId);
	void OccupyArea(terrain::SArea* area, int teamId);
	SAreaTeam GetAreaTeam(terrain::SArea* area);
	void ResetStartOnce();

	CCircuitAI* GetAuthority() const { return circuit; }
	void SetAuthority(CCircuitAI* authority);
private:
	void DelegateAuthority();
	void ApplyAuthority(CCircuitAI* newOwner);

	CCircuitAI* circuit;  // authority
	std::shared_ptr<IMainJob> releaseTask;
	TeamIds teamIds;
	utils::CRegion startBox;

	int initCount;
	int resignSize;
	int lastUpdate;
	AllyUnits friendlyUnits;  // owner
	CQuadField quadField;

	std::map<int, SClusterTeam> occupants;  // Cluster owner on start. clusterId: SClusterTeam
	std::map<terrain::SArea*, SAreaTeam> habitants;  // Area habitants on start.
	bool isResetStart;

	std::shared_ptr<CMapManager> mapManager;
	std::shared_ptr<CMetalManager> metalManager;
	std::shared_ptr<CEnergyManager> energyManager;
	std::shared_ptr<CEnergyGrid> energyGrid;
	std::shared_ptr<CDefenceData> defence;
	std::shared_ptr<CPathFinder> pathfinder;
	std::shared_ptr<CFactoryData> factoryData;

	std::shared_ptr<CEnemyManager> enemyManager;

	int uEnemyMark;
};

} // namespace circuit

#endif // SRC_CIRCUIT_SETUP_ALLYTEAM_H_
