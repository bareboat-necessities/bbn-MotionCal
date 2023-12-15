// Copyright (c) 2014, Freescale Semiconductor, Inc.
// All rights reserved.
// vim: set ts=4:
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Freescale Semiconductor, Inc. nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FREESCALE SEMICONDUCTOR, INC. BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file contains magnetic calibration functions.  It is STRONGLY RECOMMENDED
// that the casual developer NOT TOUCH THIS FILE.  The mathematics behind this file
// is extremely complex, and it will be very easy (almost inevitable) that you screw
// it up.
//
// Haha - This file has been edited!  Please do not blame or pester NXP (formerly
//        Freescale) about the "almost inevitable" issues!

#include "libcalib.h"
#include "matrix.h"

#include <math.h>

#define FXOS8700_UTPERCOUNT  0.1f
#define DEFAULTB 50.0F				// default geomagnetic field (uT)
#define X 0                         // vector components
#define Y 1
#define Z 2
#define ONETHIRD 0.33333333F        // one third
#define ONESIXTH 0.166666667F       // one sixth
#define MINMEASUREMENTS4CAL 40      // minimum number of measurements for 4 element calibration
#define MINMEASUREMENTS7CAL 100     // minimum number of measurements for 7 element calibration
#define MINMEASUREMENTS10CAL 150    // minimum number of measurements for 10 element calibration
#define MINBFITUT 22.0F             // minimum geomagnetic field B (uT) for valid calibration
#define MAXBFITUT 67.0F             // maximum geomagnetic field B (uT) for valid calibration
#define FITERRORAGINGSECS 7200.0F   // 2 hours: time for fit error to increase (age) by e=2.718

static void fUpdateCalibration4INV(MagCalibration_t *MagCal);
static void fUpdateCalibration7EIG(MagCalibration_t *MagCal);
static void fUpdateCalibration10EIG(MagCalibration_t *MagCal);



// run the magnetic calibration
int MagCal_Run(void)
{
	int i, j;			// loop counters
	int isolver;		// magnetic solver used
	int count=0;
	static int waitcount=0;

	// only do the calibration occasionally
	if (++waitcount < 20) return 0;
	waitcount = 0;

	// count number of data points
	for (i=0; i < MAGBUFFSIZE; i++) {
		if (magcal.m_aBpIsValid[i]) count++;
	}

	if (count < MINMEASUREMENTS4CAL) return 0;

	if (magcal.m_isValid) {
		// age the existing fit error to avoid one good calibration locking out future updates
		magcal.m_errorFitAged *= 1.02f;
	}

	// is enough data collected
	if (count < MINMEASUREMENTS7CAL) {
		isolver = 4;
		fUpdateCalibration4INV(&magcal); // 4 element matrix inversion calibration
		if (magcal.m_errorFitNext < 12.0f) magcal.m_errorFitNext = 12.0f;
	} else if (count < MINMEASUREMENTS10CAL) {
		isolver = 7;
		fUpdateCalibration7EIG(&magcal); // 7 element eigenpair calibration
		if (magcal.m_errorFitNext < 7.5f) magcal.m_errorFitNext = 7.5f;
	} else {
		isolver = 10;
		fUpdateCalibration10EIG(&magcal); // 10 element eigenpair calibration
	}

	// the trial geomagnetic field must be in range (earth is 22uT to 67uT)
	if ((magcal.m_calNext_B >= MINBFITUT) && (magcal.m_calNext_B <= MAXBFITUT))	{
		// always accept the calibration if
		//  1: no previous calibration exists
		//  2: the calibration fit is reduced or
		//  3: an improved solver was used giving a good trial calibration (4% or under)
		if ((magcal.m_isValid == 0) ||
				(magcal.m_errorFitNext <= magcal.m_errorFitAged) ||
				((isolver > magcal.m_isValid) && (magcal.m_errorFitNext <= 4.0F))) {
			// accept the new calibration solution
			//printf("new magnetic cal, B=%.2f uT\n", magcal.m_calNext_B);
			magcal.m_isValid = isolver;
			magcal.m_errorFit = magcal.m_errorFitNext;
			if (magcal.m_errorFitNext > 2.0f) {
				magcal.m_errorFitAged = magcal.m_errorFitNext;
			} else {
				magcal.m_errorFitAged = 2.0f;
			}
			magcal.m_cal_B = magcal.m_calNext_B;
			for (i = X; i <= Z; i++) {
				magcal.m_cal_V[i] = magcal.m_calNext_V[i];
				for (j = X; j <= Z; j++) {
					magcal.m_cal_invW[i][j] = magcal.m_calNext_invW[i][j];
				}
			}
			return 1; // indicates new calibration applied
		}
	}
	return 0;
}



// 4 element calibration using 4x4 matrix inverse
static void fUpdateCalibration4INV(MagCalibration_t *MagCal)
{
	float fBp2;					// fBp[X]^2+fBp[Y]^2+fBp[Z]^2
	float fSumBp4;				// sum of fBp2
	float fscaling;				// set to FUTPERCOUNT * FMATRIXSCALING
	float fE;					// error function = r^T.r
	int16_t iOffset[3];			// offset to remove large DC hard iron bias in matrix
	int16_t iCount;				// number of measurements counted
	int i, j, k;				// loop counters

	// working arrays for 4x4 matrix inversion
	float *pfRows[4];
	int8_t iColInd[4];
	int8_t iRowInd[4];
	int8_t iPivot[4];

	// compute fscaling to reduce multiplications later
	fscaling = FXOS8700_UTPERCOUNT / DEFAULTB;

	// the trial inverse soft iron matrix m_cal_invW always equals
	// the identity matrix for 4 element calibration
	f3x3matrixAeqI(MagCal->m_calNext_invW);

	// zero fSumBp4=Y^T.Y, m_vecB=X^T.Y (4x1) and on and above
	// diagonal elements of m_matA=X^T*X (4x4)
	fSumBp4 = 0.0F;
	for (i = 0; i < 4; i++) {
		MagCal->m_vecB[i] = 0.0F;
		for (j = i; j < 4; j++) {
			MagCal->m_matA[i][j] = 0.0F;
		}
	}

	// the offsets are guaranteed to be set from the first element but to avoid compiler error
	iOffset[X] = iOffset[Y] = iOffset[Z] = 0;

	// use from MINEQUATIONS up to MAXEQUATIONS entries from magnetic buffer to compute matrices
	iCount = 0;
	for (j = 0; j < MAGBUFFSIZE; j++) {
		if (MagCal->m_aBpIsValid[j]) {
			// use first valid magnetic buffer entry as estimate (in counts) for offset
			if (iCount == 0) {
				for (k = X; k <= Z; k++) {
					iOffset[k] = MagCal->m_aBpFast[k][j];
				}
			}

			// store scaled and offset fBp[XYZ] in m_vecA[0-2] and fBp[XYZ]^2 in m_vecA[3-5]
			for (k = X; k <= Z; k++) {
				MagCal->m_vecA[k] = (float)((int32_t)MagCal->m_aBpFast[k][j]
					- (int32_t)iOffset[k]) * fscaling;
				MagCal->m_vecA[k + 3] = MagCal->m_vecA[k] * MagCal->m_vecA[k];
			}

			// calculate fBp2 = Bp[X]^2 + Bp[Y]^2 + Bp[Z]^2 (scaled uT^2)
			fBp2 = MagCal->m_vecA[3] + MagCal->m_vecA[4] + MagCal->m_vecA[5];

			// accumulate fBp^4 over all measurements into fSumBp4=Y^T.Y
			fSumBp4 += fBp2 * fBp2;

			// now we have fBp2, accumulate m_vecB[0-2] = X^T.Y =sum(Bp2.Bp[XYZ])
			for (k = X; k <= Z; k++) {
				MagCal->m_vecB[k] += MagCal->m_vecA[k] * fBp2;
			}

			//accumulate m_vecB[3] = X^T.Y =sum(fBp2)
			MagCal->m_vecB[3] += fBp2;

			// accumulate on and above-diagonal terms of m_matA = X^T.X ignoring m_matA[3][3]
			MagCal->m_matA[0][0] += MagCal->m_vecA[X + 3];
			MagCal->m_matA[0][1] += MagCal->m_vecA[X] * MagCal->m_vecA[Y];
			MagCal->m_matA[0][2] += MagCal->m_vecA[X] * MagCal->m_vecA[Z];
			MagCal->m_matA[0][3] += MagCal->m_vecA[X];
			MagCal->m_matA[1][1] += MagCal->m_vecA[Y + 3];
			MagCal->m_matA[1][2] += MagCal->m_vecA[Y] * MagCal->m_vecA[Z];
			MagCal->m_matA[1][3] += MagCal->m_vecA[Y];
			MagCal->m_matA[2][2] += MagCal->m_vecA[Z + 3];
			MagCal->m_matA[2][3] += MagCal->m_vecA[Z];

			// increment the counter for next iteration
			iCount++;
		}
	}

	// set the last element of the measurement matrix to the number of buffer elements used
	MagCal->m_matA[3][3] = (float) iCount;

	// store the number of measurements accumulated
	MagCal->m_cBpIsValid = iCount;

	// use above diagonal elements of symmetric m_matA to set both m_matB and m_matA to X^T.X
	for (i = 0; i < 4; i++) {
		for (j = i; j < 4; j++) {
			MagCal->m_matB[i][j] = MagCal->m_matB[j][i]
				= MagCal->m_matA[j][i] = MagCal->m_matA[i][j];
		}
	}

	// calculate in situ inverse of m_matB = inv(X^T.X) (4x4) while m_matA still holds X^T.X
	for (i = 0; i < 4; i++) {
		pfRows[i] = MagCal->m_matB[i];
	}
	fmatrixAeqInvA(pfRows, iColInd, iRowInd, iPivot, 4);

	// calculate m_vecA = solution beta (4x1) = inv(X^T.X).X^T.Y = m_matB * m_vecB
	for (i = 0; i < 4; i++) {
		MagCal->m_vecA[i] = 0.0F;
		for (k = 0; k < 4; k++) {
			MagCal->m_vecA[i] += MagCal->m_matB[i][k] * MagCal->m_vecB[k];
		}
	}

	// calculate P = r^T.r = Y^T.Y - 2 * beta^T.(X^T.Y) + beta^T.(X^T.X).beta
	// = fSumBp4 - 2 * m_vecA^T.m_vecB + m_vecA^T.m_matA.m_vecA
	// first set P = Y^T.Y - 2 * beta^T.(X^T.Y) = SumBp4 - 2 * m_vecA^T.m_vecB
	fE = 0.0F;
	for (i = 0; i < 4; i++) {
		fE += MagCal->m_vecA[i] * MagCal->m_vecB[i];
	}
	fE = fSumBp4 - 2.0F * fE;

	// set m_vecB = (X^T.X).beta = m_matA.m_vecA
	for (i = 0; i < 4; i++) {
		MagCal->m_vecB[i] = 0.0F;
		for (k = 0; k < 4; k++) {
			MagCal->m_vecB[i] += MagCal->m_matA[i][k] * MagCal->m_vecA[k];
		}
	}

	// complete calculation of P by adding beta^T.(X^T.X).beta = m_vecA^T * m_vecB
	for (i = 0; i < 4; i++) {
		fE += MagCal->m_vecB[i] * MagCal->m_vecA[i];
	}

	// compute the hard iron vector (in uT but offset and scaled by FMATRIXSCALING)
	for (k = X; k <= Z; k++) {
		MagCal->m_calNext_V[k] = 0.5F * MagCal->m_vecA[k];
	}

	// compute the scaled geomagnetic field strength B (in uT but scaled by FMATRIXSCALING)
	MagCal->m_calNext_B = sqrtf(MagCal->m_vecA[3] + MagCal->m_calNext_V[X] * MagCal->m_calNext_V[X] +
			MagCal->m_calNext_V[Y] * MagCal->m_calNext_V[Y] + MagCal->m_calNext_V[Z] * MagCal->m_calNext_V[Z]);

	// calculate the trial fit error (percent) normalized to number of measurements
	// and scaled geomagnetic field strength
	MagCal->m_errorFitNext = sqrtf(fE / (float) MagCal->m_cBpIsValid) * 100.0F /
			(2.0F * MagCal->m_calNext_B * MagCal->m_calNext_B);

	// correct the hard iron estimate for FMATRIXSCALING and the offsets applied (result in uT)
	for (k = X; k <= Z; k++) {
		MagCal->m_calNext_V[k] = MagCal->m_calNext_V[k] * DEFAULTB
			+ (float)iOffset[k] * FXOS8700_UTPERCOUNT;
	}

	// correct the geomagnetic field strength B to correct scaling (result in uT)
	MagCal->m_calNext_B *= DEFAULTB;
}










// 7 element calibration using direct eigen-decomposition
static void fUpdateCalibration7EIG(MagCalibration_t *MagCal)
{
	float det;					// matrix determinant
	float fscaling;				// set to FUTPERCOUNT * FMATRIXSCALING
	float ftmp;					// scratch variable
	int16_t iOffset[3];			// offset to remove large DC hard iron bias
	int16_t iCount;				// number of measurements counted
	int i, j, k, m, n;			// loop counters

	// compute fscaling to reduce multiplications later
	fscaling = FXOS8700_UTPERCOUNT / DEFAULTB;

	// the offsets are guaranteed to be set from the first element but to avoid compiler error
	iOffset[X] = iOffset[Y] = iOffset[Z] = 0;

	// zero the on and above diagonal elements of the 7x7 symmetric measurement matrix m_matA
	for (m = 0; m < 7; m++) {
		for (n = m; n < 7; n++) {
			MagCal->m_matA[m][n] = 0.0F;
		}
	}

	// place from MINEQUATIONS to MAXEQUATIONS entries into product matrix m_matA
	iCount = 0;
	for (j = 0; j < MAGBUFFSIZE; j++) {
		if (MagCal->m_aBpIsValid[j]) {
			// use first valid magnetic buffer entry as offset estimate (bit counts)
			if (iCount == 0) {
				for (k = X; k <= Z; k++) {
					iOffset[k] = MagCal->m_aBpFast[k][j];
				}
			}

			// apply the offset and scaling and store in m_vecA
			for (k = X; k <= Z; k++) {
				MagCal->m_vecA[k + 3] = (float)((int32_t)MagCal->m_aBpFast[k][j]
					- (int32_t)iOffset[k]) * fscaling;
				MagCal->m_vecA[k] = MagCal->m_vecA[k + 3] * MagCal->m_vecA[k + 3];
			}

			// accumulate the on-and above-diagonal terms of
			// MagCal->m_matA=Sigma{m_vecA^T * m_vecA}
			// with the exception of m_matA[6][6] which will sum to the number
			// of measurements and remembering that m_vecA[6] equals 1.0F
			// update the right hand column [6] of m_matA except for m_matA[6][6]
			for (m = 0; m < 6; m++) {
				MagCal->m_matA[m][6] += MagCal->m_vecA[m];
			}
			// update the on and above diagonal terms except for right hand column 6
			for (m = 0; m < 6; m++) {
				for (n = m; n < 6; n++) {
					MagCal->m_matA[m][n] += MagCal->m_vecA[m] * MagCal->m_vecA[n];
				}
			}

			// increment the measurement counter for the next iteration
			iCount++;
		}
	}

	// finally set the last element m_matA[6][6] to the number of measurements
	MagCal->m_matA[6][6] = (float) iCount;

	// store the number of measurements accumulated
	MagCal->m_cBpIsValid = iCount;

	// copy the above diagonal elements of m_matA to below the diagonal
	for (m = 1; m < 7; m++) {
		for (n = 0; n < m; n++) {
			MagCal->m_matA[m][n] = MagCal->m_matA[n][m];
		}
	}

	// set tmpA7x1 to the unsorted eigenvalues and m_matB to the unsorted eigenvectors of m_matA
	eigencompute(MagCal->m_matA, MagCal->m_vecA, MagCal->m_matB, 7);

	// find the smallest eigenvalue
	j = 0;
	for (i = 1; i < 7; i++) {
		if (MagCal->m_vecA[i] < MagCal->m_vecA[j]) {
			j = i;
		}
	}

	// set ellipsoid matrix A to the solution vector with smallest eigenvalue,
	// compute its determinant and the hard iron offset (scaled and offset)
	f3x3matrixAeqScalar(MagCal->m_A, 0.0F);
	det = 1.0F;
	for (k = X; k <= Z; k++) {
		MagCal->m_A[k][k] = MagCal->m_matB[k][j];
		det *= MagCal->m_A[k][k];
		MagCal->m_calNext_V[k] = -0.5F * MagCal->m_matB[k + 3][j] / MagCal->m_A[k][k];
	}

	// negate A if it has negative determinant
	if (det < 0.0F) {
		f3x3matrixAeqMinusA(MagCal->m_A);
		MagCal->m_matB[6][j] = -MagCal->m_matB[6][j];
		det = -det;
	}

	// set ftmp to the square of the trial geomagnetic field strength B
	// (counts times FMATRIXSCALING)
	ftmp = -MagCal->m_matB[6][j];
	for (k = X; k <= Z; k++) {
		ftmp += MagCal->m_A[k][k] * MagCal->m_calNext_V[k] * MagCal->m_calNext_V[k];
	}

	// calculate the trial normalized fit error as a percentage
	MagCal->m_errorFitNext = 50.0F *
		sqrtf(fabs(MagCal->m_vecA[j]) / (float) MagCal->m_cBpIsValid) / fabs(ftmp);

	// normalize the ellipsoid matrix A to unit determinant
	f3x3matrixAeqAxScalar(MagCal->m_A, powf(det, -(ONETHIRD)));

	// convert the geomagnetic field strength B into uT for normalized
	// soft iron matrix A and normalize
	MagCal->m_calNext_B = sqrtf(fabs(ftmp)) * DEFAULTB * powf(det, -(ONESIXTH));

	// compute trial m_cal_invW from the square root of A also with normalized
	// determinant and hard iron offset in uT
	f3x3matrixAeqI(MagCal->m_calNext_invW);
	for (k = X; k <= Z; k++) {
		MagCal->m_calNext_invW[k][k] = sqrtf(fabs(MagCal->m_A[k][k]));
		MagCal->m_calNext_V[k] = MagCal->m_calNext_V[k] * DEFAULTB + (float)iOffset[k] * FXOS8700_UTPERCOUNT;
	}
}





// 10 element calibration using direct eigen-decomposition
static void fUpdateCalibration10EIG(MagCalibration_t *MagCal)
{
	float det;					// matrix determinant
	float fscaling;				// set to FUTPERCOUNT * FMATRIXSCALING
	float ftmp;					// scratch variable
	int16_t iOffset[3];			// offset to remove large DC hard iron bias in matrix
	int16_t iCount;				// number of measurements counted
	int i, j, k, m, n;			// loop counters

	// compute fscaling to reduce multiplications later
	fscaling = FXOS8700_UTPERCOUNT / DEFAULTB;

	// the offsets are guaranteed to be set from the first element but to avoid compiler error
	iOffset[X] = iOffset[Y] = iOffset[Z] = 0;

	// zero the on and above diagonal elements of the 10x10 symmetric measurement matrix m_matA
	for (m = 0; m < 10; m++) {
		for (n = m; n < 10; n++) {
			MagCal->m_matA[m][n] = 0.0F;
		}
	}

	// sum between MINEQUATIONS to MAXEQUATIONS entries into the 10x10 product matrix m_matA
	iCount = 0;
	for (j = 0; j < MAGBUFFSIZE; j++) {
		if (MagCal->m_aBpIsValid[j]) {
			// use first valid magnetic buffer entry as estimate for offset
			// to help solution (bit counts)
			if (iCount == 0) {
				for (k = X; k <= Z; k++) {
					iOffset[k] = MagCal->m_aBpFast[k][j];
				}
			}

			// apply the fixed offset and scaling and enter into m_vecA[6-8]
			for (k = X; k <= Z; k++) {
				MagCal->m_vecA[k + 6] = (float)((int32_t)MagCal->m_aBpFast[k][j]
					- (int32_t)iOffset[k]) * fscaling;
			}

			// compute measurement vector elements m_vecA[0-5] from m_vecA[6-8]
			MagCal->m_vecA[0] = MagCal->m_vecA[6] * MagCal->m_vecA[6];
			MagCal->m_vecA[1] = 2.0F * MagCal->m_vecA[6] * MagCal->m_vecA[7];
			MagCal->m_vecA[2] = 2.0F * MagCal->m_vecA[6] * MagCal->m_vecA[8];
			MagCal->m_vecA[3] = MagCal->m_vecA[7] * MagCal->m_vecA[7];
			MagCal->m_vecA[4] = 2.0F * MagCal->m_vecA[7] * MagCal->m_vecA[8];
			MagCal->m_vecA[5] = MagCal->m_vecA[8] * MagCal->m_vecA[8];

			// accumulate the on-and above-diagonal terms of m_matA=Sigma{m_vecA^T * m_vecA}
			// with the exception of m_matA[9][9] which equals the number of measurements
			// update the right hand column [9] of m_matA[0-8][9] ignoring m_matA[9][9]
			for (m = 0; m < 9; m++) {
				MagCal->m_matA[m][9] += MagCal->m_vecA[m];
			}
			// update the on and above diagonal terms of m_matA ignoring right hand column 9
			for (m = 0; m < 9; m++) {
				for (n = m; n < 9; n++) {
					MagCal->m_matA[m][n] += MagCal->m_vecA[m] * MagCal->m_vecA[n];
				}
			}

			// increment the measurement counter for the next iteration
			iCount++;
		}
	}

	// set the last element m_matA[9][9] to the number of measurements
	MagCal->m_matA[9][9] = (float) iCount;

	// store the number of measurements accumulated
	MagCal->m_cBpIsValid = iCount;

	// copy the above diagonal elements of symmetric product matrix m_matA to below the diagonal
	for (m = 1; m < 10; m++) {
		for (n = 0; n < m; n++) {
			MagCal->m_matA[m][n] = MagCal->m_matA[n][m];
		}
	}

	// set MagCal->m_vecA to the unsorted eigenvalues and m_matB to the unsorted
	// normalized eigenvectors of m_matA
	eigencompute(MagCal->m_matA, MagCal->m_vecA, MagCal->m_matB, 10);

	// set ellipsoid matrix A from elements of the solution vector column j with
	// smallest eigenvalue
	j = 0;
	for (i = 1; i < 10; i++) {
		if (MagCal->m_vecA[i] < MagCal->m_vecA[j]) {
			j = i;
		}
	}
	MagCal->m_A[0][0] = MagCal->m_matB[0][j];
	MagCal->m_A[0][1] = MagCal->m_A[1][0] = MagCal->m_matB[1][j];
	MagCal->m_A[0][2] = MagCal->m_A[2][0] = MagCal->m_matB[2][j];
	MagCal->m_A[1][1] = MagCal->m_matB[3][j];
	MagCal->m_A[1][2] = MagCal->m_A[2][1] = MagCal->m_matB[4][j];
	MagCal->m_A[2][2] = MagCal->m_matB[5][j];

	// negate entire solution if A has negative determinant
	det = f3x3matrixDetA(MagCal->m_A);
	if (det < 0.0F) {
		f3x3matrixAeqMinusA(MagCal->m_A);
		MagCal->m_matB[6][j] = -MagCal->m_matB[6][j];
		MagCal->m_matB[7][j] = -MagCal->m_matB[7][j];
		MagCal->m_matB[8][j] = -MagCal->m_matB[8][j];
		MagCal->m_matB[9][j] = -MagCal->m_matB[9][j];
		det = -det;
	}

	// compute the inverse of the ellipsoid matrix
	f3x3matrixAeqInvSymB(MagCal->m_invA, MagCal->m_A);

	// compute the trial hard iron vector in offset bit counts times FMATRIXSCALING
	for (k = X; k <= Z; k++) {
		MagCal->m_calNext_V[k] = 0.0F;
		for (m = X; m <= Z; m++) {
			MagCal->m_calNext_V[k] += MagCal->m_invA[k][m] * MagCal->m_matB[m + 6][j];
		}
		MagCal->m_calNext_V[k] *= -0.5F;
	}

	// compute the trial geomagnetic field strength B in bit counts times FMATRIXSCALING
	MagCal->m_calNext_B = sqrtf(fabs(MagCal->m_A[0][0] * MagCal->m_calNext_V[X] * MagCal->m_calNext_V[X] +
			2.0F * MagCal->m_A[0][1] * MagCal->m_calNext_V[X] * MagCal->m_calNext_V[Y] +
			2.0F * MagCal->m_A[0][2] * MagCal->m_calNext_V[X] * MagCal->m_calNext_V[Z] +
			MagCal->m_A[1][1] * MagCal->m_calNext_V[Y] * MagCal->m_calNext_V[Y] +
			2.0F * MagCal->m_A[1][2] * MagCal->m_calNext_V[Y] * MagCal->m_calNext_V[Z] +
			MagCal->m_A[2][2] * MagCal->m_calNext_V[Z] * MagCal->m_calNext_V[Z] - MagCal->m_matB[9][j]));

	// calculate the trial normalized fit error as a percentage
	MagCal->m_errorFitNext = 50.0F * sqrtf(
		fabs(MagCal->m_vecA[j]) / (float) MagCal->m_cBpIsValid) /
		(MagCal->m_calNext_B * MagCal->m_calNext_B);

	// correct for the measurement matrix offset and scaling and
	// get the computed hard iron offset in uT
	for (k = X; k <= Z; k++) {
		MagCal->m_calNext_V[k] = MagCal->m_calNext_V[k] * DEFAULTB + (float)iOffset[k] * FXOS8700_UTPERCOUNT;
	}

	// convert the trial geomagnetic field strength B into uT for
	// un-normalized soft iron matrix A
	MagCal->m_calNext_B *= DEFAULTB;

	// normalize the ellipsoid matrix A to unit determinant and
	// correct B by root of this multiplicative factor
	f3x3matrixAeqAxScalar(MagCal->m_A, powf(det, -(ONETHIRD)));
	MagCal->m_calNext_B *= powf(det, -(ONESIXTH));

	// compute trial m_cal_invW from the square root of fA (both with normalized determinant)
	// set m_vecA to the unsorted eigenvalues and m_matB to the unsorted eigenvectors of m_matA
	// where m_matA holds the 3x3 matrix fA in its top left elements
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			MagCal->m_matA[i][j] = MagCal->m_A[i][j];
		}
	}
	eigencompute(MagCal->m_matA, MagCal->m_vecA, MagCal->m_matB, 3);

	// set MagCal->m_matB to be eigenvectors . diag(sqrt(sqrt(eigenvalues))) =
	//   m_matB . diag(sqrt(sqrt(m_vecA))
	for (j = 0; j < 3; j++) { // loop over columns j
		ftmp = sqrtf(sqrtf(fabs(MagCal->m_vecA[j])));
		for (i = 0; i < 3; i++) { // loop over rows i
			MagCal->m_matB[i][j] *= ftmp;
		}
	}

	// set m_calNext_invW to eigenvectors * diag(sqrt(eigenvalues)) * eigenvectors^T =
	//   m_matB * m_matB^T = sqrt(fA) (guaranteed symmetric)
	// loop over rows
	for (i = 0; i < 3; i++) {
		// loop over on and above diagonal columns
		for (j = i; j < 3; j++) {
			MagCal->m_calNext_invW[i][j] = 0.0F;
			// accumulate the matrix product
			for (k = 0; k < 3; k++) {
				MagCal->m_calNext_invW[i][j] += MagCal->m_matB[i][k] * MagCal->m_matB[j][k];
			}
			// copy to below diagonal element
			MagCal->m_calNext_invW[j][i] = MagCal->m_calNext_invW[i][j];
		}
	}
}





