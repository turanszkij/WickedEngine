/*
Copyright (C) 2008 Martin Storsjo

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

/*
* add '#define FORSYTH_IMPLEMENTATION' before including to create the implementation
*
* C99 compatibility
* forsyth prefix
* outIndices parameter
* convert to header file single-file library
*
* Copyright (c) 2014, Ivan Vashchaev
*/

/*Modifications for Wicked Engine:
	- Removed warnings
	- 32 bit index type instead of 16 bit
*/

#ifndef FORSYTH_H
#define FORSYTH_H

#include <stdint.h>

//typedef uint16_t ForsythVertexIndexType;
typedef uint32_t ForsythVertexIndexType;

#ifdef __cplusplus
extern "C" {
#endif

	ForsythVertexIndexType *forsythReorderIndices(ForsythVertexIndexType *outIndices, const ForsythVertexIndexType *indices, int nTriangles, int nVertices);

#ifdef __cplusplus
}
#endif

#endif // FORSYTH_H

#ifdef FORSYTH_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

	// Set these to adjust the performance and result quality
#define FORSYTH_VERTEX_CACHE_SIZE 24
#define FORSYTH_CACHE_FUNCTION_LENGTH 32

	// The size of these data types affect the memory usage
	typedef uint16_t ForsythScoreType;
#define FORSYTH_SCORE_SCALING 7281

	typedef uint8_t ForsythAdjacencyType;
#define FORSYTH_MAX_ADJACENCY UINT8_MAX

	typedef int8_t ForsythCachePosType;
	typedef int32_t ForsythTriangleIndexType;
	typedef int32_t ForsythArrayIndexType;

	// The size of the precalculated tables
#define FORSYTH_CACHE_SCORE_TABLE_SIZE 32
#define FORSYTH_VALENCE_SCORE_TABLE_SIZE 32
#if FORSYTH_CACHE_SCORE_TABLE_SIZE < FORSYTH_VERTEX_CACHE_SIZE
#error Vertex score table too small
#endif

	// Precalculated tables
	static ForsythScoreType forsythCachePositionScore[FORSYTH_CACHE_SCORE_TABLE_SIZE];
	static ForsythScoreType forsythValenceScore[FORSYTH_VALENCE_SCORE_TABLE_SIZE];

#define FORSYTH_ISADDED(x) (triangleAdded[(x) >> 3] & (1 << (x & 7)))
#define FORSYTH_SETADDED(x) (triangleAdded[(x) >> 3] |= (1 << (x & 7)))

	// Score function constants
#define FORSYTH_CACHE_DECAY_POWER 1.5
#define FORSYTH_LAST_TRI_SCORE 0.75
#define FORSYTH_VALENCE_BOOST_SCALE 2.0
#define FORSYTH_VALENCE_BOOST_POWER 0.5

	// Precalculate the tables
	static void forsythInit() {
		for (int i = 0; i < FORSYTH_CACHE_SCORE_TABLE_SIZE; i++) {
			float score = 0;
			if (i < 3) {
				// This vertex was used in the last triangle,
				// so it has a fixed score, which ever of the three
				// it's in. Otherwise, you can get very different
				// answers depending on whether you add
				// the triangle 1,2,3 or 3,1,2 - which is silly
				score = FORSYTH_LAST_TRI_SCORE;
			}
			else {
				// Points for being high in the cache.
				const float scaler = 1.0f / (FORSYTH_CACHE_FUNCTION_LENGTH - 3);
				score = 1.0f - (i - 3) * scaler;
				score = powf(score, FORSYTH_CACHE_DECAY_POWER);
			}
			forsythCachePositionScore[i] = (ForsythScoreType)(FORSYTH_SCORE_SCALING * score);
		}

		for (int i = 1; i < FORSYTH_VALENCE_SCORE_TABLE_SIZE; i++) {
			// Bonus points for having a low number of tris still to
			// use the vert, so we get rid of lone verts quickly
			float valenceBoost = powf((float)i, -FORSYTH_VALENCE_BOOST_POWER);
			float score = (float)FORSYTH_VALENCE_BOOST_SCALE * valenceBoost;
			forsythValenceScore[i] = (ForsythScoreType)(FORSYTH_SCORE_SCALING * score);
		}
	}

	// Calculate the score for a vertex
	static ForsythScoreType forsythFindVertexScore(int numActiveTris, int cachePosition) {
		if (numActiveTris == 0) {
			// No triangles need this vertex!
			return 0;
		}

		ForsythScoreType score = 0;
		if (cachePosition < 0) {
			// Vertex is not in LRU cache - no score
		}
		else {
			score = forsythCachePositionScore[cachePosition];
		}

		if (numActiveTris < FORSYTH_VALENCE_SCORE_TABLE_SIZE)
			score += forsythValenceScore[numActiveTris];
		return score;
	}

	// The main reordering function
	ForsythVertexIndexType *forsythReorderIndices(ForsythVertexIndexType *outIndices, const ForsythVertexIndexType *indices, int nTriangles, int nVertices) {
		static int init = 1;
		if (init) {
			forsythInit();
			init = 0;
		}

		ForsythAdjacencyType *numActiveTris = (ForsythAdjacencyType *)malloc(sizeof(ForsythAdjacencyType) * nVertices);
		memset(numActiveTris, 0, sizeof(ForsythAdjacencyType) * nVertices);

		// First scan over the vertex data, count the total number of
		// occurrances of each vertex
		for (int i = 0; i < 3 * nTriangles; i++) {
			if (numActiveTris[indices[i]] == FORSYTH_MAX_ADJACENCY) {
				// Unsupported mesh,
				// vertex shared by too many triangles
				free(numActiveTris);
				return NULL;
			}
			numActiveTris[indices[i]]++;
		}

		// Allocate the rest of the arrays
		ForsythArrayIndexType *offsets = (ForsythArrayIndexType *)malloc(sizeof(ForsythArrayIndexType) * nVertices);
		ForsythScoreType *lastScore = (ForsythScoreType *)malloc(sizeof(ForsythScoreType) * nVertices);
		ForsythCachePosType *cacheTag = (ForsythCachePosType *)malloc(sizeof(ForsythCachePosType) * nVertices);

		uint8_t *triangleAdded = (uint8_t *)malloc((nTriangles + 7) / 8);
		ForsythScoreType *triangleScore = (ForsythScoreType *)malloc(sizeof(ForsythScoreType) * nTriangles);
		ForsythTriangleIndexType *triangleIndices = (ForsythTriangleIndexType *)malloc(sizeof(ForsythTriangleIndexType) * 3 * nTriangles);
		memset(triangleAdded, 0, sizeof(uint8_t) * ((nTriangles + 7) / 8));
		memset(triangleScore, 0, sizeof(ForsythScoreType) * nTriangles);
		memset(triangleIndices, 0, sizeof(ForsythTriangleIndexType) * 3 * nTriangles);

		// Count the triangle array offset for each vertex,
		// initialize the rest of the data.
		int sum = 0;
		for (int i = 0; i < nVertices; i++) {
			offsets[i] = sum;
			sum += numActiveTris[i];
			numActiveTris[i] = 0;
			cacheTag[i] = -1;
		}

		// Fill the vertex data structures with indices to the triangles
		// using each vertex
		for (int i = 0; i < nTriangles; i++) {
			for (int j = 0; j < 3; j++) {
				int v = indices[3 * i + j];
				triangleIndices[offsets[v] + numActiveTris[v]] = i;
				numActiveTris[v]++;
			}
		}

		// Initialize the score for all vertices
		for (int i = 0; i < nVertices; i++) {
			lastScore[i] = forsythFindVertexScore(numActiveTris[i], cacheTag[i]);
			for (int j = 0; j < numActiveTris[i]; j++)
				triangleScore[triangleIndices[offsets[i] + j]] += lastScore[i];
		}

		// Find the best triangle
		int bestTriangle = -1;
		int bestScore = -1;

		for (int i = 0; i < nTriangles; i++) {
			if (triangleScore[i] > bestScore) {
				bestScore = triangleScore[i];
				bestTriangle = i;
			}
		}

		// Allocate the output array
		ForsythTriangleIndexType *outTriangles = (ForsythTriangleIndexType *)malloc(sizeof(ForsythTriangleIndexType) * nTriangles);
		int outPos = 0;

		// Initialize the cache
		int cache[FORSYTH_VERTEX_CACHE_SIZE + 3];
		for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE + 3; i++)
			cache[i] = -1;

		int scanPos = 0;

		// Output the currently best triangle, as long as there
		// are triangles left to output
		while (bestTriangle >= 0) {
			// Mark the triangle as added
			FORSYTH_SETADDED(bestTriangle);
			// Output this triangle
			outTriangles[outPos++] = bestTriangle;
			for (int i = 0; i < 3; i++) {
				// Update this vertex
				int v = indices[3 * bestTriangle + i];

				// Check the current cache position, if it
				// is in the cache
				int endpos = cacheTag[v];
				if (endpos < 0)
					endpos = FORSYTH_VERTEX_CACHE_SIZE + i;
				// Move all cache entries from the previous position
				// in the cache to the new target position (i) one
				// step backwards
				for (int j = endpos; j > i; j--) {
					cache[j] = cache[j - 1];
					// If this cache slot contains a real
					// vertex, update its cache tag
					if (cache[j] >= 0)
						cacheTag[cache[j]]++;
				}
				// Insert the current vertex into its new target
				// slot
				cache[i] = v;
				cacheTag[v] = i;

				// Find the current triangle in the list of active
				// triangles and remove it (moving the last
				// triangle in the list to the slot of this triangle).
				for (int j = 0; j < numActiveTris[v]; j++) {
					if (triangleIndices[offsets[v] + j] == bestTriangle) {
						triangleIndices[offsets[v] + j] = triangleIndices[offsets[v] + numActiveTris[v] - 1];
						break;
					}
				}
				// Shorten the list
				numActiveTris[v]--;
			}
			// Update the scores of all triangles in the cache
			for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE + 3; i++) {
				int v = cache[i];
				if (v < 0)
					break;
				// This vertex has been pushed outside of the
				// actual cache
				if (i >= FORSYTH_VERTEX_CACHE_SIZE) {
					cacheTag[v] = -1;
					cache[i] = -1;
				}
				ForsythScoreType newScore = forsythFindVertexScore(numActiveTris[v], cacheTag[v]);
				ForsythScoreType diff = newScore - lastScore[v];
				for (int j = 0; j < numActiveTris[v]; j++)
					triangleScore[triangleIndices[offsets[v] + j]] += diff;
				lastScore[v] = newScore;
			}
			// Find the best triangle referenced by vertices in the cache
			bestTriangle = -1;
			bestScore = -1;
			for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE; i++) {
				if (cache[i] < 0)
					break;
				int v = cache[i];
				for (int j = 0; j < numActiveTris[v]; j++) {
					int t = triangleIndices[offsets[v] + j];
					if (triangleScore[t] > bestScore) {
						bestTriangle = t;
						bestScore = triangleScore[t];
					}
				}
			}
			// If no active triangle was found at all, continue
			// scanning the whole list of triangles
			if (bestTriangle < 0) {
				for (; scanPos < nTriangles; scanPos++) {
					if (!FORSYTH_ISADDED(scanPos)) {
						bestTriangle = scanPos;
						break;
					}
				}
			}
		}

		// Convert the triangle index array into a full triangle list
		outPos = 0;
		for (int i = 0; i < nTriangles; i++) {
			int t = outTriangles[i];
			for (int j = 0; j < 3; j++) {
				int v = indices[3 * t + j];
				outIndices[outPos++] = v;
			}
		}

		// Clean up
		free(triangleIndices);
		free(offsets);
		free(lastScore);
		free(numActiveTris);
		free(cacheTag);
		free(triangleAdded);
		free(triangleScore);
		free(outTriangles);

		return outIndices;
	}

#ifdef __cplusplus
}
#endif

#endif //FORSYTH_IMPLEMENTATION
