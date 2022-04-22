/*
 * TravelAction.h
 *
 *  Created on: Feb 16, 2016
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_UNIT_ACTION_TRAVELACTION_H_
#define SRC_CIRCUIT_UNIT_ACTION_TRAVELACTION_H_

#include "unit/action/UnitAction.h"
#include "terrain/path/PathInfo.h"

#include <memory>

namespace circuit {

class ITravelAction: public IUnitAction {
public:
	ITravelAction(CCircuitUnit* owner, Type type, int squareSize, float speed = NO_SPEED_LIMIT);
	ITravelAction(CCircuitUnit* owner, Type type, const std::shared_ptr<CPathInfo>& pPath,
			int squareSize, float speed = NO_SPEED_LIMIT);
	virtual ~ITravelAction();

	virtual void OnEnd() override;

	void SetPath(const std::shared_ptr<CPathInfo>& pPath, float speed = NO_SPEED_LIMIT);
	const std::shared_ptr<CPathInfo>& GetPath() const { return pPath; }

protected:
	int CalcSpeedStep(float& stepSpeed);

	std::shared_ptr<CPathInfo> pPath;
	float speed;
	int pathIterator;
	int increment;
	int minSqDist;
	int lastSector;  // last issued sector index
	int lastFrame;
};

} // namespace circuit

#endif // SRC_CIRCUIT_UNIT_ACTION_TRAVELACTION_H_
