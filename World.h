#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <vector>
#include "glm/glm.hpp"
#include "Vertex.h"
#include <utility>
#include <map>
#include "PerlinNoise.h"
#include <time.h>
#include <thread>
#include <functional>

#include <iostream>	// Remove this later

#define RENDER_RADIUS 3
#define RENDER_DISTANCE ((2 * RENDER_RADIUS) + 1)
#define GEN_DISTANCE (RENDER_DISTANCE + 2)

static int WORLD_MAX_INDICES{};// = 36 * 8 * 8 * 128;

class World {
public:
	World();
	~World();
	void updateBlocks();

	//Vertex* releaseVertexData();
	float* releaseVertexData();
	unsigned int getVertexCount();

	unsigned int* releaseIndices();
	unsigned int getIndexCount();

	void updateRender(int xChunk, int zChunk);

	struct Block {
		int blockID;
		glm::vec3 blockCoordinates;
	};

	typedef struct Chunk {
		Block data[16][256][16];
	} Chunk;

	typedef struct RenderGroup {
		Chunk chunks[RENDER_DISTANCE][RENDER_DISTANCE];
		bool chunksToUpdate[RENDER_DISTANCE][RENDER_DISTANCE];
		float xOrigin;
		float zOrigin;

		// Ringbuffer indices
		int xHead;
		int xTail;
		int zHead;
		int zTail;
	};

	typedef struct VertData{
		float x;
		float y;
		float z;
		float r;
		float g;
		float b;
		float u;
		float v;
	} VertData;

	typedef enum DrawFace {
		DRAW_FACE_NONE = 0,
		DRAW_FACE_TOP = 1,
		DRAW_FACE_BOTTOM = 2,
		DRAW_FACE_LEFT = 4,
		DRAW_FACE_RIGHT = 8,
		DRAW_FACE_FRONT = 16,
		DRAW_FACE_BACK = 32,
		
	} DrawFace;
	
	bool processBlockBreak(float x, float y, float z);
	bool processBlockPlace(float x, float y, float z, float lookX, float lookY, float lookZ, unsigned int ID);

	bool worldUpdated = false;

private:
	Chunk* testChunk;

	RenderGroup* testGroup;

	unsigned int currNumVertices = 0;

	std::vector<std::vector<std::pair<glm::vec3, DrawFace>>> connectingFaces{};
	int connectingFacesIdx = -1;	// Index for above vector

	DrawFace connFaces[RENDER_DISTANCE][RENDER_DISTANCE][16][256][16]{}; // TEST

	std::vector<VertData>* meshVertices{};
	std::vector<unsigned int>* meshIndices{};

	std::vector<std::vector<VertData>>* renderMeshVertices{};
	std::vector<VertData>* renderMeshVerticesFlattened{};

	std::vector<std::vector<unsigned int>>* renderMeshIndices{};
	std::vector<unsigned int>* renderMeshIndicesFlattened{};

	bool vFlattenedDirty;
	bool iFlattenedDirty;

	std::map<std::pair<unsigned int, unsigned int>, RenderGroup> worldData{}; // Maybe this should include meshes too

	int numChunks{};

	double randSeed{};

	PerlinNoise* noise;

	std::vector<std::thread *> threadList;

	void generateChunk( Chunk* chunk ); // SHould delete this one
	void generateChunk(Chunk* chunk, int x, int y, int z);

	void createRenderMesh( Chunk* chunk, DrawFace omittedFaces, int xOffset, int zOffset );
	void handleUpdate(); //-> TBD
	void initTestChunk();

	void generateRenderGroup(RenderGroup* group);

	/*
	* KISS
	* It will take more computing time to omit storing chunk faces than it is 
	* to render them out
	* 
	* I.e. 8 dist, worst case is 16*16*255 -> 65536 faces per chunk side
	* 65536 * 4 sides * 64 chunks * 8 floats per vert data * 4 vert per face = 270 mb
	* For now, worst case will not happen and minimal cases are even less significant
	* For the sake of development, return to optimizing rendering *LATER*
	* 
	* TODO:
	*	Optimize so that adjacent chunks are cleaned up
	* 
	*/
	void generateRenderGroup2(RenderGroup* group); // Testing cause og function is trash
	void createRenderMesh2(Chunk* chunk, int xOffset, int zOffset);

	void updateRenderGroup(RenderGroup* group, float newXOrigin, float newZOrigin);

	void generateChunksInBackground(RenderGroup* group);
};

#endif