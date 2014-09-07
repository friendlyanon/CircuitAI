/*
 * RagMatrix.h
 *
 *  Created on: Sep 7, 2014
 *      Author: rlcevg
 */

#ifndef RAGMATRIX_H_
#define RAGMATRIX_H_

namespace circuit {

class CRagMatrix {
public:
	CRagMatrix(int nrows);
	CRagMatrix(const CRagMatrix& matrix);
	virtual ~CRagMatrix();

	int GetNrows();
	float FindClosestPair(int n, int& ir, int& jr);
	float& operator()(int row, int column) const;

private:
	int nrows;
	float* data;
};

} // namespace circuit

#endif // RAGMATRIX_H_
