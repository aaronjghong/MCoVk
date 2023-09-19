#include  "World.h"

#define WORLD_BLOCK_INCREMENT 0.0625f
#define WBI WORLD_BLOCK_INCREMENT

#define WORLD_BLOCK_INCREMENT_DOUBLE 0.0625
#define WBID WORLD_BLOCK_INCREMENT_DOUBLE

#define MAX(x ,y) ((x) > (y) ? (x) : (y))
#define MIN(x ,y) ((x) < (y) ? (x) : (y))


static unsigned int numVertices = 0;

static void s_CallVertexGenCommands(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, bool& top, bool& bot, bool& l, bool& r, bool& front, bool& back, int x, int y, int z);
static void s_SetVertexGenFlags(const World::Chunk* chunk, bool& top, bool& bot, bool& l, bool& r, bool& front, bool& back, int x, int y, int z);
static void s_ResetVertexGenFlags(bool& top, bool& bot, bool& l, bool& r, bool& front, bool& back);
static void s_GenerateHorizontalFace(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, int x, int y, int z, World::DrawFace face);
static void s_GenerateVerticalFaceLR(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, int x, int y, int z, World::DrawFace face);
static void s_GenerateVerticalFaceFB(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, int x, int y, int z, World::DrawFace face);
static void s_IncrementRingBufferPointer(int& idx);
static void s_DecrementRingBufferPointer(int& idx);
static void s_SetAndCheckConnectingFaces(World::RenderGroup* group, World::DrawFace connFaces[RENDER_DISTANCE][RENDER_DISTANCE][16][256][16], int i, int j, bool doX, bool doZ);

World::World() {
	srand(time(NULL));
	randSeed = rand();
	noise = new PerlinNoise(randSeed);

	// Init vectors
	meshVertices = new std::vector<World::VertData>;
	meshIndices = new std::vector<unsigned int>;
	renderMeshVertices = new std::vector<std::vector<VertData>>;
	renderMeshVerticesFlattened = new std::vector<World::VertData>;
	renderMeshIndices = new std::vector<std::vector<unsigned int>>;
	renderMeshIndicesFlattened = new std::vector<unsigned int>;

	
	//initTestChunk();
	//createRenderMesh(testChunk);
	testGroup = new RenderGroup();
	testGroup->xOrigin = 3;
	testGroup->zOrigin = 3;

	testGroup->xHead = 0;
	testGroup->xTail = RENDER_DISTANCE - 1;
	testGroup->zHead = 0;
	testGroup->zTail = RENDER_DISTANCE - 1;

	generateRenderGroup(testGroup);

	vFlattenedDirty = true;
	iFlattenedDirty = true;
}

World::~World() {
	delete meshVertices;
	delete meshIndices;
	delete renderMeshVertices;
	delete renderMeshVerticesFlattened;
	delete renderMeshIndices;
	delete renderMeshIndicesFlattened;

	delete testGroup;
	delete noise;

	delete testChunk;
}

float* World::releaseVertexData() {
	if ( vFlattenedDirty ) {
		renderMeshVerticesFlattened->clear();
		for ( auto& v : *renderMeshVertices ) {
			renderMeshVerticesFlattened->insert(renderMeshVerticesFlattened->end(), v.begin(), v.end());
		}
		vFlattenedDirty = false;
	}

	return (float*)renderMeshVerticesFlattened->data();
}

unsigned int World::getVertexCount() {
	if ( vFlattenedDirty ) {
		renderMeshVerticesFlattened->clear();
		for ( auto& v : *renderMeshVertices ) {
			renderMeshVerticesFlattened->insert(renderMeshVerticesFlattened->end(), v.begin(), v.end());
		}
		vFlattenedDirty = false;
	}

	return renderMeshVerticesFlattened->size();
}

unsigned int* World::releaseIndices() {
	if ( iFlattenedDirty ) {
		for ( auto& v : *renderMeshIndices ) {
			renderMeshIndicesFlattened->insert(renderMeshIndicesFlattened->end(), v.begin(), v.end());
		}
		iFlattenedDirty = false;
	}

	return renderMeshIndicesFlattened->data();
}

unsigned int World::getIndexCount() {
	if ( iFlattenedDirty ) {
		for ( auto& v : *renderMeshIndices ) {
			renderMeshIndicesFlattened->insert(renderMeshIndicesFlattened->end(), v.begin(), v.end());
		}
		iFlattenedDirty = false;
	}

	return renderMeshIndicesFlattened->size();
}

bool World::processBlockBreak(float x, float y, float z) {
	/* Need to get current chunk within render group */
	return true; // For now

	/* Clamp intercept */
	x = std::floor(x * 16);
	y = std::floor(y * 16);
	z = std::floor(z * 16);

	/* First shouuld have code to get target chunk */
	/* Then x/y/z should be clamped to within the chunk */
	// For now:
	if ( x < 0 || x > 15 || y < 0 || y > 255 || z < 0 || z > 15) {
		return false;
	}


	/* Then calls should be to target chunk, not test chunk */
	/* Test chunk is being used for testing purposes */
	if ( !testChunk->data[(int)x][(int)y][(int)z].blockID ) {
		worldUpdated = false;
		return false;
	}

	else {
		testChunk->data[(int)x][(int)y][(int)z].blockID = 0;

		std::cout << "Block broken at: " << x << " " << y << " " << z << std::endl;

		createRenderMesh(testChunk, DRAW_FACE_NONE, 0, 0);
		worldUpdated = true;
		return true;
	}
}

bool World::processBlockPlace(float x, float y, float z, float lookX, float lookY, float lookZ, unsigned int ID) {
	/* Need to get current chunk within render group */
	return true; // For now

	/* Clamp intercept */
	x = std::floor(x * 16);
	y = std::floor(y * 16);
	z = std::floor(z * 16);

	/* First shouuld have code to get target chunk */
	/* Then x/y/z should be clamped to within the chunk */
	// For now:
	if ( x < 0 || x > 15 || y < 0 || y > 255 || z < 0 || z > 15 ) {
		return false;
	}

	/* Need to detect which face is being intersected by the view ray */
	/* For now, detect if block is present, then pick the most prevalent view direction */
	/* And place block in that view direction if there is no other block */
	if ( !testChunk->data[(int)x][(int)y][(int)z].blockID ) {
		worldUpdated = false;
		return false;
	}

	else {
		float tmpX, tmpY, tmpZ;
		tmpX = std::abs(lookX);
		tmpY = std::abs(lookY);
		tmpZ = std::abs(lookZ);

		float dir = MAX(tmpZ, MAX(tmpX, tmpY));

		if ( dir == tmpX || dir == -tmpX ) {
			dir = lookX;
			if ( dir > 0 ) {
				x -= 1;
			}
			else {
				x += 1;
			}
		}
		else if ( dir == tmpY || dir == -tmpY ) {
			dir = lookY;
			if ( dir > 0 ) {
				y -= 1;
			}
			else {
				y += 1;
			}
		}
		else {
			dir = lookZ;
			if ( dir > 0 ) {
				z -= 1;
			}
			else {
				z += 1;
			}
		}

		if ( x < 0 || x > 15 || y < 0 || y > 255 || z < 0 || z > 15 ) {
			return false;
		}

		if ( !testChunk->data[(int)x][(int)y][(int)z].blockID ) {
			testChunk->data[(int)x][(int)y][(int)z].blockID = ID;
			createRenderMesh(testChunk, DRAW_FACE_NONE, 0, 0);
			worldUpdated = true;
			return true;
		}

		worldUpdated = false;
		return false;
		
	}
}


void World::createRenderMesh( Chunk *chunk, DrawFace omittedFaces, int xOffset, int zOffset ) {

	/* omittedFaces indicates which faces should NEVER be drawn within this run */
	/* should only be used for drawing connected chunks */

	bool startedMesh = false;

	/* Here for pseudocode purposes */

	/*
	*	Within a 16x256x16 area:
	*	We can normalize 16x16x16 into 0->1 space
	*	Which gives 16 separate sub-chunks which stack on top of each other
	*	Therefore, we work in increments of 0.0625f
	* 
	*	
	*/

	/*
	* Do we need a 16x16 buffer to double-pass 
	 
	*/

	meshVertices->clear();
	meshIndices->clear();
	numVertices = currNumVertices;

	for ( int y = 255; y > 0; y-- ) {
		/* From Top-Down */
		/* Start uninitialized and mark start when block data is found */

		for ( int x = 0; x < 16; x++ ) {
			for ( int z = 0; z < 16; z++ ) {
				/* For each block within this horizontal slice */

				if ( chunk->data[x][y][z].blockID != 0 ) {

					/*
					* For each block, there are 6 possible faces that need to be drawn
					* but it all boils down to a few cases
					* If there is nothing above, which is the case when mesh isn't started yet, the top face needs to be drawn
					*	When dealing with this case, index chunk slice at y + 1
					* If there is nothing to the sides, then there is no need for a side face to be drawn
					*	When dealing with this case, index chunk slice at x +/- 1 and z +/- 1
					* If there is nothing below, which is the case at y = 0, the bottom face needs to be drawn
					*	When dealing with this case, index chunk slice at y - 1
					*/

					bool top, bot, l, r, front, back;
					/* Going to assume that front -> back goes from 0 -> 16 in z */
					/* Vertex indices MUST go CCW  */

					if ( !startedMesh ) {
						/* If a block is found and mesh wasn't started */
						/* We are almost guaranteed that we're dealing with a horizontal face */
						startedMesh = true;

						/* Create and push vec3s, need to also push correct indices to draw */
					}

					/*
					* Once we obtain the vertex data:
					* If we follow a Bottom left -> upper left -> bottom right -> upper right convention for quads
					* We can simply push indices in the vertex gen functions in a CCW order
					* The issue would be getting the correct index for each vertex within a "global" context
					* To deal with this, we could keep a static member variable that keeps track of the "global" mesh vertices vector size
					* We can then reference the index numbers when generating each face from that
					*/

					s_SetVertexGenFlags(chunk, top, bot, l, r, front, back, x, y, z);


					/* Check ommitedFaces flags */
					//top = !(omittedFaces & DRAW_FACE_TOP);
					//bot = !(omittedFaces & DRAW_FACE_BOTTOM);


					if ( x == 0 ) {
						l = !(omittedFaces & DRAW_FACE_LEFT);
					}
					if ( x == 15 ) {
						r = !(omittedFaces & DRAW_FACE_RIGHT);
					}
					if ( z == 0 ) {
						front = !(omittedFaces & DRAW_FACE_FRONT);
					}
					if ( z == 15 ) {
						back = !(omittedFaces & DRAW_FACE_BACK);
					}


					int j = connectingFacesIdx % RENDER_DISTANCE;
					int i = (connectingFacesIdx - j) / RENDER_DISTANCE;
					// Poses issue as is since only one face is represented at a time
					// Commented out for generateRenderGroup2
					/*if ( connFaces[i][j][x][y][z] != DRAW_FACE_NONE ) {
						DrawFace face = connFaces[i][j][x][y][z];
						if ( (face & DRAW_FACE_TOP) == DRAW_FACE_TOP ) {
							top = true;
						}
						if ( (face & DRAW_FACE_BOTTOM) == DRAW_FACE_BOTTOM ) {
							bot = true;
						}
						if ( (face & DRAW_FACE_LEFT) == DRAW_FACE_LEFT ) {
							l = true;
						}
						if ( (face & DRAW_FACE_RIGHT) == DRAW_FACE_RIGHT ) {
							r = true;
						}
						if ( (face & DRAW_FACE_FRONT) == DRAW_FACE_FRONT ) {
							front = true;
						}
						if ( (face & DRAW_FACE_BACK) == DRAW_FACE_BACK ) {
							back = true;
						}
					}*/


					s_CallVertexGenCommands(meshVertices, meshIndices, top, bot, l, r, front, back, x + xOffset, y, z + zOffset);
					s_ResetVertexGenFlags(top, bot, l, r, front, back);

				}
			}
		}
	}

	/* Reset index to not re-trigger unless explicitly specified */
	connectingFacesIdx = -1;
}

void World::createRenderMesh2(Chunk* chunk, int xOffset, int zOffset) {
	bool startedMesh = false;
	meshVertices->clear();
	meshIndices->clear();
	numVertices = currNumVertices;


	for ( int y = 255; y >= 0; y-- ) {
		for ( int x = 0; x < 16; x++ ) {
			for ( int z = 0; z < 16; z++ ) {
				if ( chunk->data[x][y][z].blockID != 0 ) {
					bool top, bot, l, r, front, back;

					if ( !startedMesh )
					{
						startedMesh = true;
					}

					s_SetVertexGenFlags(chunk, top, bot, l, r, front, back, x, y, z);

					s_CallVertexGenCommands(meshVertices, meshIndices, top, bot, l, r, front, back, x + xOffset, y, z + zOffset);
					s_ResetVertexGenFlags(top, bot, l, r, front, back);
				}
			}
		}
	}
}

void World::initTestChunk() {

	//return;

	testChunk = new Chunk{};
	//memset((void*)testChunk, 0, sizeof(Chunk));

	randSeed = rand();

	generateChunk(testChunk, 0, randSeed / rand(), 0);

	meshVertices = new std::vector<World::VertData>;
	meshIndices = new std::vector<unsigned int>;

	return;


	for ( int y = 255; y > 0; y-- ) {
		for ( int x = 0; x < 16; x++ ) {
			for ( int z = 0; z < 16; z++ ) {
				if ( (y > 15) && (y < 33) ) {
					testChunk->data[x][y][z].blockID = 1;
					//testchunk->data[x][y][z].blockCoordinates = glm::vec3( x, y, z ); // For now
				}
				testChunk->data[x][y][z].blockCoordinates = glm::vec3(x, y, z);
			}
		}
	}

}

void World::generateRenderGroup(RenderGroup* group) {
	//group = new RenderGroup{};
	// For now, origins will be later implemented when proper chunking is done
	currNumVertices = 0;
	connectingFaces.clear();
	connectingFaces.resize(RENDER_DISTANCE * RENDER_DISTANCE);

	/*
	* Maybe do 2 passes?
	* First pass gens block data, iterates through the data at chunk edges, and stores coordinates + face of draws that should be done
	* Second pass calls the render command
	*/

	for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
		for ( int j = 0; j < RENDER_DISTANCE; j++ ) {
			int xOffset = (i * 16) + group->xOrigin;
			int zOffset = (j * 16) + group->zOrigin;
			generateChunk(&group->chunks[i][j], xOffset, 0, zOffset);

			if ( i > 0 ) {
				// Check edges with chunk to the left
				for ( int k = 0; k < 16; k++ ) {
					for ( int y = 255; y > 0; y-- ) {
						if ( group->chunks[i][j].data[0][y][k].blockID && !(group->chunks[i - 1][j].data[15][y][k].blockID) ) {
							// Right is higher than left, signal to right group to gen leftfaces
							connFaces[i][j][0][y][k] = (DrawFace)(connFaces[i][j][0][y][k] | DRAW_FACE_LEFT);
						}
						else if ( !(group->chunks[i][j].data[0][y][k].blockID) && group->chunks[i - 1][j].data[15][y][k].blockID ) {
							// Left is higher than right, signal to left group to gen right faces
							connFaces[i - 1][j][15][y][k] = (DrawFace)(connFaces[i - 1][j][15][y][k] | DRAW_FACE_RIGHT);
						}
					}
				}
			}
			if ( j > 0 ) {
				// Check edges with chunk behind
				for ( int k = 0; k < 16; k++ ) {
					for ( int y = 255; y > 0; y-- ) {
						if ( group->chunks[i][j].data[k][y][0].blockID && !(group->chunks[i][j - 1].data[k][y][15].blockID) ) {
							// Front is higher than back, signal to front to gen back faces
							connFaces[i][j][k][y][0] = (DrawFace)(connFaces[i][j][k][y][0] | DRAW_FACE_FRONT);
						}
						else if ( !(group->chunks[i][j].data[k][y][0].blockID) && group->chunks[i][j - 1].data[k][y][15].blockID ) {
							// Back is higher, signal to back to gen front faces
							connFaces[i][j - 1][k][y][15] = (DrawFace)(connFaces[i][j - 1][k][y][15] | DRAW_FACE_BACK);
						}
					}
				}
			}
		}
	}

	for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
		for ( int j = 0; j < RENDER_DISTANCE; j++ ) {
			
			DrawFace ommitedFaces =(DrawFace)( DRAW_FACE_LEFT | DRAW_FACE_RIGHT | DRAW_FACE_FRONT | DRAW_FACE_BACK );
			uint8_t unset = UINT8_MAX;
			/* Un-set omitted faces flag */
			if ( i == 0 ) {
				// Draw left?
				unset ^= DRAW_FACE_LEFT;
			}
			else if ( i == (RENDER_DISTANCE - 1) ) {
				// Draw right?
				unset ^= DRAW_FACE_RIGHT;
			}

			if ( j == 0 ) {
				// Draw front?
				unset ^= DRAW_FACE_FRONT;
			}
			else if ( j == (RENDER_DISTANCE - 1) ) {
				// Draw back?
				unset ^= DRAW_FACE_BACK;
			}

			// Unset unwated ommisions
			ommitedFaces = (DrawFace)(unset & ommitedFaces);

			int xOffset = (i * 16) + group->xOrigin;
			int zOffset = (j * 16) + group->zOrigin;

			connectingFacesIdx = j + (RENDER_DISTANCE * i);

			createRenderMesh(&group->chunks[i][j], ommitedFaces, xOffset, zOffset);

			currNumVertices += meshVertices->size();

			// Not sure if it's correct to push values instead of addresses
			renderMeshVertices->push_back(*meshVertices);
			renderMeshIndices->push_back(*meshIndices);


		}
	} 
}

void World::generateRenderGroup2(RenderGroup* group) {
	// Reset num verts
	currNumVertices = 0;

	for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
		for ( int j = 0; j < RENDER_DISTANCE; j++ ) {

			int xOffset = (i * 16) + group->xOrigin;
			int zOffset = (j * 16) + group->zOrigin;

			generateChunk(&group->chunks[i][j], xOffset, 0, zOffset);
			createRenderMesh2(&group->chunks[i][j], xOffset, zOffset);

			currNumVertices += meshVertices->size();

			// Not sure if it's correct to push values instead of addresses
			renderMeshVertices->push_back(*meshVertices);
			renderMeshIndices->push_back(*meshIndices);
		}
	}
}

void World::generateChunk(Chunk* chunk, int x, int y, int z) {
	/* Where x,y,z are the coordinates where the chunk "begins" */
	/* well, y doesn't really matter, but still */
	/* y could be used for generating features on a second pass or smth */

	int heights[16][16]{};

	memset(chunk, 0, sizeof(Chunk));

	for ( int i = 0; i < 16; i++ ) {
		for ( int j = 0; j < 16; j++ ) {
			int height = std::floor(40.0 * noise->noise((i + x) * WBID, y * WBID, (j + z) * WBID));
			for ( int k = 0; k < height; k++ ) {
				chunk->data[i][k][j].blockID = 1;// 1 for now
				chunk->data[i][k][j].blockCoordinates = glm::vec3(x + i, y + k, z + j);
			}
		}
	}


}

void World::updateRender(int xChunk, int zChunk) {
	updateRenderGroup(testGroup, xChunk * 16.0f, zChunk * 16.0f);

	// Flag that changes were made
	vFlattenedDirty = true;
	iFlattenedDirty = true;
	worldUpdated = true;
}

void World::updateRenderGroup(RenderGroup* group, float newXOrigin, float newZOrigin) {
	// Update Rendergroup ring buffer with new chunk data
	// Need to remove unused chunks, and introduce new ones
	// Need to get new connecting faces as well
	// Maybe can have createRenderGroup go from Head -> Target (separate variable)
	//	Where target = RENDER_DISTANCE - 1 initially but is updated to appropriate indices
	//	Swap i = 0, j = 0 checks to i = head, j = head?


	float prevXOrigin = group->xOrigin;
	float prevZOrigin = group->zOrigin;

	group->xOrigin = newXOrigin - 8;
	group->zOrigin = newZOrigin - 8;

	// new_Origin determines player's current location
	// We want rendergroup to center the player
	// IF render distance is even, the new origin should be the input origin minus half of renderdistance * 16
	// IF odd, new origin should be the input origin minus (floor(renderdistance / 2) plus eight)

	int isOdd = ((RENDER_DISTANCE % 2) != 0) ? 1 : 0;

	group->xOrigin = newXOrigin - (((RENDER_DISTANCE / 2) * 16) + (8 * isOdd));
	group->zOrigin = newZOrigin - (((RENDER_DISTANCE / 2) * 16) + (8 * isOdd));

	// THIS IS ABSOLUTE TRASH
	renderMeshVertices->clear();
	renderMeshIndices->clear();

	generateRenderGroup2(group);
	return;
	// THIS IS ABSOLUTE TRASH
	// TODO: Work on ring buffer using *new* (unoptimized) render funcs
	// 

	// WIP
	if ( prevXOrigin != newXOrigin ) {
		if ( prevXOrigin < newXOrigin ) {
			// Push front (Add element at decremented head)
			s_DecrementRingBufferPointer(group->xHead);
			s_DecrementRingBufferPointer(group->xTail);
			for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
				group->chunksToUpdate[group->xHead][i] = true;
			}
		}
		else {
			// Push back (Add element at incremented tail)
			s_IncrementRingBufferPointer(group->xHead);
			s_IncrementRingBufferPointer(group->xTail);
			for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
				group->chunksToUpdate[group->xTail][i] = true;
			}
		}
	}
	// Then need to repeat the same for z and in generateRenderGroup2, need to
	// only generate chunks marked for updating
	// The issue is that meshdata is stored altogether so we need to update
	// How we store the verts & inds to render cause we need to selectively
	// clear verts and indices instead of re-creating all
	// WIP

	// This assumes that only a movement of a chunk at a time is allowed
	// RenderGroup stores chunks as chunks[col][col item]
	// Need to call generate chunk on correct indices
	// Need to generate connecting faces on correct indices
	// Need to rewrite correct indices of rendermeshvertices and rendermeshindices after calling createrendermesh

	if ( prevXOrigin != newXOrigin ) {
		int xStart, xEnd; // Pointers for con face generation
		bool generateChunkFlag = false;
		if ( prevXOrigin < newXOrigin ) {
			// Push front (Add element at decremented head)
			s_DecrementRingBufferPointer(group->xHead);
			s_DecrementRingBufferPointer(group->xTail);
			xStart = group->xHead;
			xEnd = group->xHead;
			s_IncrementRingBufferPointer(xEnd);
			generateChunkFlag = true; // xStart is new Chunk
		}
		else {
			// Push back (Add element at incremented tail)
			s_IncrementRingBufferPointer(group->xHead);
			s_IncrementRingBufferPointer(group->xTail);
			xStart = group->xTail;
			xEnd = group->xTail;
			s_DecrementRingBufferPointer(xStart);
			generateChunkFlag = false; // xStart is not new Chunk, wait until second iteration
		}

		// After setting X ringbuffer operation pointers iterate and generate new chunk data
		for ( int i = xStart; i != xEnd; s_IncrementRingBufferPointer(i) ) {
			for ( int j = 0; j < RENDER_DISTANCE; j++ ) {
				if ( generateChunkFlag ) {
					//Need to figure out offsets for chunks
					//generateChunk(&group->chunks[i][j], , 0, );
				}
				else{
					// If chunk was not generated in this pass, it must be generated in the next
					generateChunkFlag = true;
				}

				if ( i == xEnd) {
					// If we've reached the end pointer, we know that there is a chunk prior to generate connecting faces with
					s_SetAndCheckConnectingFaces(group, connFaces, i, j, true, true);
				}
			}
		}
	}
	if ( prevZOrigin != newZOrigin ) {
		int zStart, zEnd;
		for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
			for ( int j = zStart; j != zEnd; s_IncrementRingBufferPointer(j) ) {
				
			}
		}
	}
}

void World::generateConnectingBlocks(Chunk& cUL, Chunk& cUR, Chunk& cBL, Chunk& cBR) {
	// TBD
}





static void s_CallVertexGenCommands(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, bool& top, bool& bot, bool& l, bool& r, bool& front, bool& back, int x, int y, int z) {
	if ( top ) {
		//meshVertices->resize(numVertices + 4);
		//meshIndices->resize(numVertices / 4 * 6);
		s_GenerateHorizontalFace(meshVertices, meshIndices, x, y + 1, z, World::DRAW_FACE_TOP);	// y + 1 as LR/FB gen using yMesh2 = y + 1;
		numVertices += 4;
	}
	if ( bot ) {
		s_GenerateHorizontalFace(meshVertices, meshIndices, x, y, z, World::DRAW_FACE_BOTTOM);
		numVertices += 4;
	}
	if ( l ) {
		s_GenerateVerticalFaceLR(meshVertices, meshIndices, x, y, z, World::DRAW_FACE_LEFT);
		numVertices += 4;
	}
	if ( r ) {
		s_GenerateVerticalFaceLR(meshVertices, meshIndices, x + 1, y, z, World::DRAW_FACE_RIGHT);
		numVertices += 4;
	}
	if ( front ) {
		s_GenerateVerticalFaceFB(meshVertices, meshIndices, x, y, z, World::DRAW_FACE_FRONT);
		numVertices += 4;
	}
	if ( back ) {
		s_GenerateVerticalFaceFB(meshVertices, meshIndices, x, y, z + 1, World::DRAW_FACE_BACK);
		numVertices += 4;
	}
}

static void s_SetVertexGenFlags(const World::Chunk* chunk, bool &top, bool &bot, bool &l, bool &r, bool &front, bool &back, int x, int y, int z) {
	/* Checking left */
	if ( x == 0 ) {
		l = true;
	}
	else {
		l = chunk->data[x - 1][y][z].blockID ? false : true;
	}

	/* Checking right */
	if ( x == 15 ) {
		r = true;
	}
	else {
		r = chunk->data[x + 1][y][z].blockID ? false : true;
	}

	/* Checking front */
	if ( z == 0 ) {
		front = true;
	}
	else {
		front = chunk->data[x][y][z - 1].blockID ? false : true;
	}

	/* Checking back */
	if ( z == 15 ) {
		back = true;
	}
	else {
		back = chunk->data[x][y][z + 1].blockID ? false : true;
	}

	/* Checking top */
	if ( y == 255 ) {
		top = true;
	}
	else {
		top = chunk->data[x][y + 1][z].blockID ? false : true;
	}

	/* Checking bottom */
	if ( y == 0 ) {
		bot = true;
	}
	else {
		bot = chunk->data[x][y - 1][z].blockID ? false : true;
	}
}

static void s_ResetVertexGenFlags(bool& top, bool& bot, bool& l, bool& r, bool& front, bool& back) {
	back = false;
	front = false;
	top = false;
	bot = false;
	l = false;
	r = false;
}

static void s_GenerateHorizontalFace(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, int x, int y, int z, World::DrawFace face) {
	/* x,y,z should NOT be normalized */

	float xMesh{}, yMesh{}, zMesh{};
	float xMesh2{}, zMesh2{};

	unsigned int iBL{}, iBR{}, iUL{}, iUR{};

	xMesh = WBI * x;
	yMesh = WBI * y;
	zMesh = WBI * z;

	xMesh2 = WBI * (x + 1);
	zMesh2 = WBI * (z + 1);

	glm::vec3 BL{ xMesh, yMesh, zMesh };
	glm::vec3 UL{ xMesh, yMesh, zMesh2 };
	glm::vec3 BR{ xMesh2, yMesh, zMesh };
	glm::vec3 UR{ xMesh2, yMesh, zMesh2 };

	Vertex vBL{ BL, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f} };
	Vertex vUL{ UL, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} };
	Vertex vBR{ BR, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f} };
	Vertex vUR{ UR, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f} };

	World::VertData dBL{ xMesh, yMesh, zMesh, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	World::VertData dUL{ xMesh, yMesh, zMesh2, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	World::VertData dBR{ xMesh2, yMesh, zMesh, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	World::VertData dUR{ xMesh2, yMesh, zMesh2, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f };

	meshVertices->insert(meshVertices->end(), { dBL, dUL, dBR, dUR });

	switch ( face )
	{
		case World::DRAW_FACE_BOTTOM:
			iBL = 0 + numVertices;
			iUL = 1 + numVertices;
			iBR = 2 + numVertices;
			iUR = 3 + numVertices;
			break;

		case World::DRAW_FACE_TOP:
			iBL = 2 + numVertices;
			iUL = 3 + numVertices;
			iBR = 0 + numVertices;
			iUR = 1 + numVertices;
			break;

		default:
			iBL = 0 + numVertices;
			iUL = 1 + numVertices;
			iBR = 2 + numVertices;
			iUR = 3 + numVertices;
			break;
	}

	meshIndices->insert(meshIndices->end(), {iBL, iBR, iUR, iUR, iUL, iBL});
}

static void s_GenerateVerticalFaceLR(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, int x, int y, int z, World::DrawFace face) {
	/* Generates Left/Right faces */
	/* x,y,z should NOT be normalized */

	float xMesh{}, yMesh{}, zMesh{};
	float yMesh2{}, zMesh2{};

	unsigned int iBL{}, iBR{}, iUL{}, iUR{};

	xMesh = WBI * x;
	yMesh = WBI * y;
	zMesh = WBI * z;

	yMesh2 = WBI * (y + 1);	
	zMesh2 = WBI * (z + 1);

	glm::vec3 BL{ xMesh, yMesh, zMesh2 };
	glm::vec3 UL{ xMesh, yMesh2, zMesh2 };
	glm::vec3 BR{ xMesh, yMesh, zMesh };
	glm::vec3 UR{ xMesh, yMesh2, zMesh };

	Vertex vBL{ BL, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f} };
	Vertex vUL{ UL, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} };
	Vertex vBR{ BR, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f} };
	Vertex vUR{ UR, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f} };

	World::VertData dBL{ xMesh, yMesh, zMesh2, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	World::VertData dUL{ xMesh, yMesh2, zMesh2, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	World::VertData dBR{ xMesh, yMesh, zMesh, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	World::VertData dUR{ xMesh, yMesh2, zMesh, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f };

	meshVertices->insert(meshVertices->end(), { dBL, dUL, dBR, dUR });

	switch ( face )
	{
	case World::DRAW_FACE_RIGHT:
		iBL = 0 + numVertices;
		iUL = 1 + numVertices;
		iBR = 2 + numVertices;
		iUR = 3 + numVertices;
		break;

	case World::DRAW_FACE_LEFT:
		iBL = 2 + numVertices;
		iUL = 3 + numVertices;
		iBR = 0 + numVertices;
		iUR = 1 + numVertices;
		break;

	default:
		iBL = 0 + numVertices;
		iUL = 1 + numVertices;
		iBR = 2 + numVertices;
		iUR = 3 + numVertices;
		break;
	}

	meshIndices->insert(meshIndices->end(), { iBL, iBR, iUR, iUR, iUL, iBL });
}

static void s_GenerateVerticalFaceFB(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, int x, int y, int z, World::DrawFace face) {
	/* Generates Front/Back faces */
	/* x,y,z should NOT be normalized */

	float xMesh{}, yMesh{}, zMesh{};
	float xMesh2{}, yMesh2{};

	unsigned int iBL{}, iBR{}, iUL{}, iUR{};

	xMesh = WBI * x;
	yMesh = WBI * y;
	zMesh = WBI * z;

	xMesh2 = WBI * (x + 1);
	yMesh2 = WBI * (y + 1);

	glm::vec3 BL{ xMesh, yMesh, zMesh };
	glm::vec3 UL{ xMesh, yMesh2, zMesh };
	glm::vec3 BR{ xMesh2, yMesh, zMesh };
	glm::vec3 UR{ xMesh2, yMesh2, zMesh };

	Vertex vBL{ BL, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f} };
	Vertex vUL{ UL, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} };
	Vertex vBR{ BR, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f} };
	Vertex vUR{ UR, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f} };

	World::VertData dBL{ xMesh, yMesh, zMesh, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	World::VertData dUL{ xMesh, yMesh2, zMesh, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	World::VertData dBR{ xMesh2, yMesh, zMesh, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	World::VertData dUR{ xMesh2, yMesh2, zMesh, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f };

	meshVertices->insert(meshVertices->end(), { dBL, dUL, dBR, dUR });

	switch ( face )
	{
	case World::DRAW_FACE_BACK:
		iBL = 0 + numVertices;
		iUL = 1 + numVertices;
		iBR = 2 + numVertices;
		iUR = 3 + numVertices;
		break;

	case World::DRAW_FACE_FRONT:
		iBL = 2 + numVertices;
		iUL = 3 + numVertices;
		iBR = 0 + numVertices;
		iUR = 1 + numVertices;
		break;

	default:
		iBL = 0 + numVertices;
		iUL = 1 + numVertices;
		iBR = 2 + numVertices;
		iUR = 3 + numVertices;
		break;
	}

	meshIndices->insert(meshIndices->end(), { iBL, iBR, iUR, iUR, iUL, iBL });
}

static void s_IncrementRingBufferPointer(int& idx) {
	idx = (idx == (RENDER_DISTANCE - 1)) ? 0 : (idx + 1);
}

static void s_DecrementRingBufferPointer(int& idx) {
	idx = (idx == 0) ? (RENDER_DISTANCE - 1) : (idx - 1);
}

static void s_SetAndCheckConnectingFaces(World::RenderGroup* group, World::DrawFace connFaces[RENDER_DISTANCE][RENDER_DISTANCE][16][256][16], int i, int j, bool doX, bool doZ) {
	if ( ( i > 0 )  &&  doX ) {
		// Check edges with chunk to the left
		for ( int k = 0; k < 16; k++ ) {
			for ( int y = 255; y > 0; y-- ) {
				if ( group->chunks[i][j].data[0][y][k].blockID && !(group->chunks[i - 1][j].data[15][y][k].blockID) ) {
					// Right is higher than left, signal to right group to gen leftfaces
					connFaces[i][j][0][y][k] = (World::DrawFace)(connFaces[i][j][0][y][k] | World::DRAW_FACE_LEFT);
				}
				else if ( !(group->chunks[i][j].data[0][y][k].blockID) && group->chunks[i - 1][j].data[15][y][k].blockID ) {
					// Left is higher than right, signal to left group to gen right faces
					connFaces[i - 1][j][15][y][k] = (World::DrawFace)(connFaces[i - 1][j][15][y][k] | World::DRAW_FACE_RIGHT);
				}
			}
		}
	}
	if ( ( j > 0 ) && doZ ) {
		// Check edges with chunk behind
		for ( int k = 0; k < 16; k++ ) {
			for ( int y = 255; y > 0; y-- ) {
				if ( group->chunks[i][j].data[k][y][0].blockID && !(group->chunks[i][j - 1].data[k][y][15].blockID) ) {
					// Front is higher than back, signal to front to gen back faces
					connFaces[i][j][k][y][0] = (World::DrawFace)(connFaces[i][j][k][y][0] | World::DRAW_FACE_FRONT);
				}
				else if ( !(group->chunks[i][j].data[k][y][0].blockID) && group->chunks[i][j - 1].data[k][y][15].blockID ) {
					// Back is higher, signal to back to gen front faces
					connFaces[i][j - 1][k][y][15] = (World::DrawFace)(connFaces[i][j - 1][k][y][15] | World::DRAW_FACE_BACK);
				}
			}
		}
	}
}

/*

	Realistically, I don't need to care about edges for a render group (i.e. first chunk 0,0 or last chunk 15,15
	Whenever I move a chunk, I should re-generate a render group with the x,z offsets
		However, this is non-ideal as that means that i may lose connecting block faces and need to re-find them
		I may also lose chunk data itself and need to re-retrieve it from the perlin noise function
			This issue persists for both when regenerating chunks that will stay in the render group as well as for returning chunks that have been rendered before

	Chunk heightmap lookup is fairly quick so for now, no need to store chunk data itself?
		But what if a change is made within the chunk and it needs to be re-rendered after exiting the render group
			Changes to chunks should be stored -> For later?

	If a chunk is already rendered and will continue to be rendered after a new render group is generated, its data should not be released
	Any chunk leaving the rendergroup should have connecting face data stored

	Rendergroups can behave like 2 ring buffers, updating the X/Z origins to "load in" new chunks 
		That way persisting chunks stay
		Need to keep separate head and tail pointers/indices for x and z axes
		Should re-modify connecting faces such that:
			A) Chunk iteration uses head and tail pointers/indices 
			B) Pre-existing connecting faces do not get re-generated 
				Maybe have a flag for each chunk?
					0 - No faces generated
					1 - Face generated in the x direction
					2 - Face generated in the z direction
					3 - All faces generated

		Realistically, a change of more than 1 chunk at a time cannot happen meaning if we store the previous offsets, we should know which chunks are new
			If more than 1 chunk change is made, we can either re-gen or keep a flag with each chunk 

	Store changes to chunks within a file with the seed as the header


	We could have a map with a lookup for rendergroups / chunk data?
		That means we still need to store and find some way to get rid of it?
			Store nearest Z x Z render groups? and discard and re-gen


	World
	-> Render Group
	-> World Data 
		-> Changes to chunks and their offset locations?
	-> Vertices
	-> Indices
	-> Seed








	OR
	
	A render group must be centered around the player coordinates

	Every 16 blocks, we update the render group origins

	When grabbing world data, first grab from perlin noise, then grab from external file / local cache of modifications

	The renderbuffer should still be a 2 ring buffer system?


*/