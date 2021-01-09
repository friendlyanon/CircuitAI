/*
 * SquadTask.cpp
 *
 *  Created on: Jan 23, 2016
 *      Author: rlcevg
 */

#include "task/fighter/SquadTask.h"
#include "task/TaskManager.h"
#include "map/InfluenceMap.h"
#include "module/BuilderManager.h"
#include "module/MilitaryManager.h"
#include "terrain/TerrainManager.h"
#include "terrain/path/PathFinder.h"
#include "terrain/path/QueryLineMap.h"
#include "unit/action/TravelAction.h"
#include "CircuitAI.h"
#include "util/Utils.h"

#include <cmath>

namespace circuit {

using namespace springai;

ISquadTask::ISquadTask(ITaskManager* mgr, FightType type, float powerMod)
		: IFighterTask(mgr, type, powerMod)
		, lowestRange(std::numeric_limits<float>::max())
		, highestRange(.0f)
		, lowestSpeed(std::numeric_limits<float>::max())
		, highestSpeed(.0f)
		, leader(nullptr)
		, groupPos(-RgtVector)
		, prevGroupPos(-RgtVector)
		, pPath(std::make_shared<PathInfo>())
		, groupFrame(0)
		, attackFrame(-1)
{
}

ISquadTask::~ISquadTask()
{
}

void ISquadTask::AssignTo(CCircuitUnit* unit)
{
	IFighterTask::AssignTo(unit);

	CCircuitDef* cdef = unit->GetCircuitDef();
	const float range = cdef->GetMinRange();
	rangeUnits[range].insert(unit);

	if (leader == nullptr) {
		lowestRange  = cdef->GetMaxRange();
		highestRange = cdef->GetMaxRange();
		lowestSpeed  = cdef->GetSpeed();
		highestSpeed = cdef->GetSpeed();
		leader = unit;
	} else {
		lowestRange  = std::min(lowestRange,  cdef->GetMaxRange());
		highestRange = std::max(highestRange, cdef->GetMaxRange());
		lowestSpeed  = std::min(lowestSpeed,  cdef->GetSpeed());
		highestSpeed = std::max(highestSpeed, cdef->GetSpeed());
		if (cdef->IsRoleSupport()) {
			return;
		}
		if ((leader->GetArea() == nullptr) ||
			leader->GetCircuitDef()->IsRoleSupport() ||
			((unit->GetArea() != nullptr) && (unit->GetArea()->percentOfMap < leader->GetArea()->percentOfMap)))
		{
			leader = unit;
		}
	}
}

void ISquadTask::RemoveAssignee(CCircuitUnit* unit)
{
	IFighterTask::RemoveAssignee(unit);

	CCircuitDef* cdef = unit->GetCircuitDef();
	const float range = cdef->GetMinRange();
	std::set<CCircuitUnit*>& setUnits = rangeUnits[range];
	setUnits.erase(unit);
	if (setUnits.empty()) {
		rangeUnits.erase(range);
	}

	leader = nullptr;
	lowestRange = lowestSpeed = std::numeric_limits<float>::max();
	highestRange = highestSpeed = .0f;

	if (units.empty()) {
		return;
	}

	FindLeader(units.begin(), units.end());
}

void ISquadTask::Merge(ISquadTask* task)
{
	const std::set<CCircuitUnit*>& rookies = task->GetAssignees();
	IAction::State state = leader->GetTravelAct()->GetState();
	const std::shared_ptr<PathInfo>& lPath = leader->GetTravelAct()->GetPath();
	for (CCircuitUnit* unit : rookies) {
		unit->SetTask(this);
		if (unit->GetCircuitDef()->IsRoleSupport()) {
			continue;
		}
		unit->GetTravelAct()->SetPath(lPath);
		unit->GetTravelAct()->SetState(state);
	}
	units.insert(rookies.begin(), rookies.end());
	attackPower += task->GetAttackPower();
	const std::set<CCircuitUnit*>& sh = task->GetShields();
	shields.insert(sh.begin(), sh.end());

	const std::map<float, std::set<CCircuitUnit*>>& rangers = task->GetRangeUnits();
	for (const auto& kv : rangers) {
		rangeUnits[kv.first].insert(kv.second.begin(), kv.second.end());
	}

	FindLeader(rookies.begin(), rookies.end());
}

const AIFloat3& ISquadTask::GetLeaderPos(int frame) const
{
	return (leader != nullptr) ? leader->GetPos(frame) : GetPosition();
}

void ISquadTask::FindLeader(decltype(units)::iterator itBegin, decltype(units)::iterator itEnd)
{
	if (leader == nullptr) {
		for (; itBegin != itEnd; ++itBegin) {
			CCircuitUnit* ass = *itBegin;
			lowestRange  = std::min(lowestRange,  ass->GetCircuitDef()->GetMaxRange());
			highestRange = std::max(highestRange, ass->GetCircuitDef()->GetMaxRange());
			lowestSpeed  = std::min(lowestSpeed,  ass->GetCircuitDef()->GetSpeed());
			highestSpeed = std::max(highestSpeed, ass->GetCircuitDef()->GetSpeed());
			if (!ass->GetCircuitDef()->IsRoleSupport()) {
				leader = ass;
				++itBegin;
				break;
			}
		}
	}
	for (; itBegin != itEnd; ++itBegin) {
		CCircuitUnit* ass = *itBegin;
		lowestRange  = std::min(lowestRange,  ass->GetCircuitDef()->GetMaxRange());
		highestRange = std::max(highestRange, ass->GetCircuitDef()->GetMaxRange());
		lowestSpeed  = std::min(lowestSpeed,  ass->GetCircuitDef()->GetSpeed());
		highestSpeed = std::max(highestSpeed, ass->GetCircuitDef()->GetSpeed());
		if (ass->GetCircuitDef()->IsRoleSupport() || (ass->GetArea() == nullptr)) {
			continue;
		}
		if ((leader->GetArea() == nullptr) ||
			leader->GetCircuitDef()->IsRoleSupport() ||
			(ass->GetArea()->percentOfMap < leader->GetArea()->percentOfMap))
		{
			leader = ass;
		}
	}
}

bool ISquadTask::IsMergeSafe() const
{
	CCircuitAI* circuit = manager->GetCircuit();
	const AIFloat3& pos = leader->GetPos(circuit->GetLastFrame());
	return (circuit->GetInflMap()->GetInfluenceAt(pos) > -INFL_EPS);
}

ISquadTask* ISquadTask::CheckMergeTask()
{
	const ISquadTask* task = nullptr;

	CCircuitAI* circuit = manager->GetCircuit();
	const int frame = circuit->GetLastFrame();
	const AIFloat3& pos = leader->GetPos(frame);
	STerrainMapArea* area = leader->GetArea();
	CTerrainManager* terrainMgr = circuit->GetTerrainManager();
	const float sqMaxDistCost = SQUARE(MAX_TRAVEL_SEC * lowestSpeed);
	float metric = std::numeric_limits<float>::max();

	CPathFinder* pathfinder = circuit->GetPathfinder();
	std::shared_ptr<CQueryLineMap> query = std::static_pointer_cast<CQueryLineMap>(
			pathfinder->CreateLineMapQuery(leader, circuit->GetThreatMap(), frame));

	const std::set<IFighterTask*>& tasks = static_cast<CMilitaryManager*>(manager)->GetTasks(fightType);
	for (const IFighterTask* candidate : tasks) {
		if ((candidate == this) ||
			(candidate->GetAttackPower() < attackPower) ||
			!candidate->CanAssignTo(leader))
		{
			continue;
		}
		const ISquadTask* candy = static_cast<const ISquadTask*>(candidate);

		const AIFloat3& tp = candy->GetLeaderPos(frame);
		const AIFloat3& taskPos = utils::is_valid(tp) ? tp : pos;

		if (!terrainMgr->CanMoveToPos(area, taskPos)) {  // ensure that path always exists
			continue;
		}

		if (!query->IsSafeLine(pos, taskPos)) {  // ensure safe passage
			continue;
		}

		// Check time-distance to target
		float sqDistCost = pos.SqDistance2D(taskPos);
		if ((sqDistCost < metric) && (sqDistCost < sqMaxDistCost)) {
			task = candy;
			metric = sqDistCost;
		}
	}

	return const_cast<ISquadTask*>(task);
}

ISquadTask* ISquadTask::GetMergeTask()
{
	if (updCount % 32 == 1) {
		return IsMergeSafe() ? CheckMergeTask() : nullptr;
	}
	return nullptr;
}

bool ISquadTask::IsMustRegroup()
{
	if ((State::ENGAGE == state) || (updCount % 16 != 15)) {
		return false;
	}

	CCircuitAI* circuit = manager->GetCircuit();
	const int frame = circuit->GetLastFrame();
	if (circuit->GetInflMap()->GetEnemyInflAt(leader->GetPos(frame)) > INFL_EPS) {  // IsMergeSafe() ?
		state = State::ROAM;
		return false;
	}

	static std::vector<CCircuitUnit*> validUnits;  // NOTE: micro-opt
//	validUnits.reserve(units.size());
	CTerrainManager* terrainMgr = circuit->GetTerrainManager();;
	for (CCircuitUnit* unit : units) {
		if (!unit->GetCircuitDef()->IsPlane() &&
			terrainMgr->CanMoveToPos(unit->GetArea(), unit->GetPos(frame)))
		{
			validUnits.push_back(unit);
		}
	}
	if (validUnits.empty()) {
		state = State::ROAM;
		return false;
	}

	if (State::REGROUP != state) {
		groupPos = leader->GetPos(frame);
		groupFrame = frame;
	} else if (frame >= groupFrame + FRAMES_PER_SEC * 60) {
		// eliminate buggy units
		const float sqMaxDist = SQUARE(std::max<float>(SQUARE_SIZE * 8 * validUnits.size(), highestRange));
		for (CCircuitUnit* unit : units) {
			if (unit->GetCircuitDef()->IsPlane()) {
				continue;
			}
			const AIFloat3& pos = unit->GetPos(frame);
			const float sqDist = groupPos.SqDistance2D(pos);
			if ((sqDist > sqMaxDist) &&
				((unit->GetTaskFrame() < groupFrame) || !terrainMgr->CanMoveToPos(unit->GetArea(), pos)))
			{
				TRY_UNIT(circuit, unit,
					unit->GetUnit()->Stop();
					unit->GetUnit()->SetMoveState(2);
				)
				circuit->Garbage(unit, "stuck");
//				circuit->GetBuilderManager()->EnqueueReclaim(IBuilderTask::Priority::HIGH, unit);
			}
		}

		validUnits.clear();
		state = State::ROAM;
		return false;
	}

	bool wasRegroup = (State::REGROUP == state);
	state = State::ROAM;

	const float sqMaxDist = SQUARE(std::max<float>(SQUARE_SIZE * 8 * validUnits.size(), highestRange));
	for (CCircuitUnit* unit : validUnits) {
		const float sqDist = groupPos.SqDistance2D(unit->GetPos(frame));
		if (sqDist > sqMaxDist) {
			state = State::REGROUP;
			break;
		}
	}

	if (!wasRegroup && (State::REGROUP == state)) {
		if (utils::is_equal_pos(prevGroupPos, groupPos)) {
			TRY_UNIT(circuit, leader,
				leader->GetUnit()->Stop();
				leader->GetUnit()->SetMoveState(2);
			)
			circuit->Garbage(leader, "stuck");
//			circuit->GetBuilderManager()->EnqueueReclaim(IBuilderTask::Priority::HIGH, leader);
		}
		prevGroupPos = groupPos;
	}

	validUnits.clear();
	return State::REGROUP == state;
}

void ISquadTask::ActivePath(float speed)
{
	for (CCircuitUnit* unit : units) {
		unit->GetTravelAct()->SetPath(pPath, speed);
	}
}

NSMicroPather::TestFunc ISquadTask::GetHitTest() const
{
	CTerrainManager* terrainMgr = manager->GetCircuit()->GetTerrainManager();
	const std::vector<STerrainMapSector>& sectors = terrainMgr->GetAreaData()->sector;
	const int sectorXSize = terrainMgr->GetSectorXSize();
	const float aimLift = leader->GetCircuitDef()->GetHeight() / 2;  // TODO: Use aim-pos of attacker and enemy
	return [&sectors, sectorXSize, aimLift](int2 start, int2 end) {  // losTest
		float startHeight = sectors[start.y * sectorXSize + start.x].maxElevation + aimLift;
		float diffHeight = sectors[end.y * sectorXSize + end.x].maxElevation + SQUARE_SIZE - startHeight;
		// All octant line draw
		const int dx =  abs(end.x - start.x), sx = start.x < end.x ? 1 : -1;
		const int dy = -abs(end.y - start.y), sy = start.y < end.y ? 1 : -1;
		int err = dx + dy;  // error value e_xy
		for (int x = start.x, y = start.y;;) {
			const int e2 = 2 * err;
			if (e2 >= dy) {  // e_xy + e_x > 0
				if (x == end.x) break;
				err += dy; x += sx;
			}
			if (e2 <= dx) {  // e_xy + e_y < 0
				if (y == end.y) break;
				err += dx; y += sy;
			}

			const float t = fabs((dx > -dy) ? float(x - start.x) / dx : float(y - start.y) / dy);
			if (sectors[y * sectorXSize + x].maxElevation > diffHeight * t + startHeight) {
				return false;
			}
		}
		return true;
	};
}

void ISquadTask::Attack(const int frame)
{
	const AIFloat3& tPos = GetTarget()->GetPos();
	const int targetTile = manager->GetCircuit()->GetInflMap()->Pos2Index(tPos);
	const bool isRepeatAttack = (frame >= attackFrame + FRAMES_PER_SEC * 3);
	attackFrame = isRepeatAttack ? frame : attackFrame;

	auto it = rangeUnits.begin()->second.begin();
	std::advance(it, rangeUnits.begin()->second.size() / 2);  // TODO: Optimize
	AIFloat3 dir = (*it)->GetPos(frame) - tPos;
	const float alpha = atan2f(dir.z, dir.x);
	// incorrect, it should check aoe in vicinity
	const float aoe = (GetTarget()->GetCircuitDef() != nullptr) ? GetTarget()->GetCircuitDef()->GetAoe() : SQUARE_SIZE;
	const bool isGroundAttack = GetTarget()->GetUnit()->IsCloaked();

	int row = 0;
	for (const auto& kv : rangeUnits) {
		CCircuitDef* rowDef = (*kv.second.begin())->GetCircuitDef();
		const float range = ((row++ == 0) ? std::min(kv.first, rowDef->GetLosRadius()) : kv.first) * RANGE_MOD;
		const float maxDelta = (M_PI * 0.8f) / kv.second.size();
		// NOTE: float delta = asinf(cdef->GetRadius() / range);
		//       but sin of a small angle is similar to that angle, omit asinf() call
		float delta = (3.0f * (rowDef->GetRadius() + aoe)) / range;
		if (delta > maxDelta) {
			delta = maxDelta;
		}
		float beta = -delta * (kv.second.size() / 2);
		for (CCircuitUnit* unit : kv.second) {
			unit->GetTravelAct()->StateWait();
			if (unit->Blocker() != nullptr) {
				continue;  // Do not interrupt current action
			}

			if (isRepeatAttack
				|| (unit->GetTarget() != GetTarget())
				|| (unit->GetTargetTile() != targetTile))
			{
				const float angle = alpha + beta;
				AIFloat3 newPos(tPos.x + range * cosf(angle), tPos.y, tPos.z + range * sinf(angle));
				CTerrainManager::CorrectPosition(newPos);
				unit->Attack(newPos, GetTarget(), targetTile, isGroundAttack, frame + FRAMES_PER_SEC * 60);
			}

			beta += delta;
		}
	}
}

#ifdef DEBUG_VIS
void ISquadTask::Log()
{
	IFighterTask::Log();

	CCircuitAI* circuit = manager->GetCircuit();
	circuit->LOG("pPath: %i | size: %i | TravelAct: %i", pPath.get(), pPath ? pPath->posPath.size() : 0,
			leader->GetTravelAct()->GetState());
	if (leader != nullptr) {
		circuit->GetDrawer()->AddPoint(leader->GetPos(circuit->GetLastFrame()), leader->GetCircuitDef()->GetDef()->GetName());
	}
}
#endif

} // namespace circuit
