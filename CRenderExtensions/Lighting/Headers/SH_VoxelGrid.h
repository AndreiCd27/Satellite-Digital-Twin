#pragma once

#include "SH.h"
#include "GeometryBasics.h"

template <int MaxL>
class SHVoxelGrid {

	const int gridX;
	const int gridY;
	const int gridZ;

	const AVector3 WorldMin;
	const AVector3 WorldMax;

	const AVector3 WorldDelta;

	// Weighted shadow functions for soft shadows
	/*
	const float w[16] = {
		1.0f,                                     // L0
		0.95f, 0.95f, 0.95f,                      // L1
		0.75f, 0.75f, 0.75f, 0.75f, 0.75f,        // L2
		0.50f, 0.50f, 0.50f, 0.50f, 0.50f, 0.50f, 0.50f // L3
	};
	*/

public:

	SHVoxelGrid(AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax) :
		gridX((int)VoxelResolution.x), gridY((int)VoxelResolution.y), gridZ((int)VoxelResolution.z),
		WorldMin(WorldMin), WorldMax(WorldMax), WorldDelta(WorldMax - WorldMin) {}

	int GetGridX() { return gridX; }
	int GetGridY() { return gridY; }
	int GetGridZ() { return gridZ; }

	AVector3 GetWorldMin() {
		return WorldMin;
	}
	AVector3 GetWorldMax() {
		return WorldMax;
	}

	void ComputeSHEXP(float* f) {
		float f0 = f[0];
		// f0 is DC component of LogSH
		// Multiply it with Y<0,0> (0.282)
		float g0 = exp(f0 * 0.282095f);

		// DC Component of linear visibility vector:
		f[0] = g0 * 3.544907f; // sqrt(4pi)

		for (int i = 1; i < 16; i++) {
			// Scale Visibility on higher directions (L>0) with average g0
			f[i] = f[i] * g0;
		}
	}

	int worldToGrid(float val, float minW, float maxW, int gridRes) {
		float t = (val - minW) / (maxW - minW);
		return std::clamp((int)(t * (gridRes - 1)), 0, gridRes - 1);
	};

};