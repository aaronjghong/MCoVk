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

	threadList.clear();
	
	//initTestChunk();
	//createRenderMesh(testChunk);
	testGroup = new RenderGroup();
	testGroup->xOrigin = 16;
	testGroup->zOrigin = 16;

	testGroup->xHead = 0;
	testGroup->xTail = RENDER_DISTANCE - 1;
	testGroup->zHead = 0;
	testGroup->zTail = RENDER_DISTANCE - 1;

	for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
		for ( int j = 0; j < RENDER_DISTANCE; j++ ) {
			testGroup->chunksToUpdate[i][j] = true;
		}
	}

	generateRenderGroup2(testGroup);

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

	for ( auto& thread : threadList ) {
		thread->join();
		delete thread;
	}
}

float* World::releaseVertexData() {
	if ( vFlattenedDirty ) {
		for ( auto& thread : threadList ) {
			thread->join();
			delete thread;
		}
		threadList.clear();

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
		for ( auto& thread : threadList ) {
			thread->join();
			delete thread;
		}
		threadList.clear();

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
		for ( auto& thread : threadList ) {
			thread->join();
			delete thread;
		}
		threadList.clear();


		for ( auto& v : *renderMeshIndices ) {
			renderMeshIndicesFlattened->insert(renderMeshIndicesFlattened->end(), v.begin(), v.end());
		}
		iFlattenedDirty = false;
	}

	return renderMeshIndicesFlattened->data();
}

unsigned int World::getIndexCount() {
	if ( iFlattenedDirty ) {
		for ( auto& thread : threadList ) {
			thread->join();
			delete thread;
		}
		threadList.clear();

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

template <typename t>
void s_CopyVector(std::vector<t>& dest, std::vector<t>& target)
{
	dest.clear();
	dest.resize(target.size());
	for ( size_t i = 0; i < target.size(); i++ ) {
		dest[i] = target[i];
	}
}

void World::generateRenderGroup2(RenderGroup* group) {
	// Reset num verts
	currNumVertices = 0;

	// For now will only run once, but if render distances are to be a user
	// setting, this will allow rescaling of these vertices
	if ( renderMeshVertices->size() < RENDER_DISTANCE * RENDER_DISTANCE ) {
		renderMeshVertices->resize(RENDER_DISTANCE * RENDER_DISTANCE);
		renderMeshIndices->resize(RENDER_DISTANCE * RENDER_DISTANCE);
	}

	// Need to iter from xHead->xTail, zHead->zTail to keep track of proper 
	// relative space due to ring buffer sys
	bool doneX = false;
	int i = group->xHead;
	while ( !doneX ) {
		bool doneZ = false;
		int j = group->zHead;
		while ( !doneZ ) {
			// Subtract by radius since middle is treated as 0,0 
			int xOffset = ((i - RENDER_RADIUS) * 16) + group->xOrigin;
			int zOffset = ((j - RENDER_RADIUS) * 16) + group->zOrigin;

			generateChunk(&group->chunks[i][j], xOffset, 0, zOffset);
			createRenderMesh2(&group->chunks[i][j], xOffset, zOffset);

			currNumVertices += meshVertices->size();

			// Not sure if it's correct to push values instead of addresses
			int renderMeshIdx = i * RENDER_DISTANCE + j;

			(*renderMeshVertices)[renderMeshIdx].assign(meshVertices->begin(), meshVertices->end());
			(*renderMeshIndices)[renderMeshIdx].assign(meshIndices->begin(), meshIndices->end());

			s_IncrementRingBufferPointer(j);
			if ( j == group->zHead )
				doneZ = true;
		}
		s_IncrementRingBufferPointer(i);
		if ( i == group->xHead )
			doneX = true;
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
	std::thread *t = new std::thread(&World::updateRenderGroup, this, testGroup, xChunk * 16.0f, zChunk * 16.0f);
	threadList.push_back(t);

	// Flag that changes were made
	vFlattenedDirty = true;
	iFlattenedDirty = true;
	worldUpdated = true;
}

void World::updateRenderGroup(RenderGroup* group, float newXOrigin, float newZOrigin) {
	// Update Rendergroup ring buffer with new chunk data
	// Need to remove unused chunks, and introduce new ones
	float prevXOrigin = group->xOrigin;
	float prevZOrigin = group->zOrigin;

	group->xOrigin = newXOrigin;
	group->zOrigin = newZOrigin;

	// WIP
	if ( prevXOrigin != group->xOrigin ) {
		if ( prevXOrigin > group->xOrigin ) {
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

	if ( prevZOrigin != group->zOrigin ) {
		if ( prevZOrigin > group->zOrigin ) {
			// Push front (add to head)
			s_DecrementRingBufferPointer(group->zHead);
			s_DecrementRingBufferPointer(group->zTail);
			for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
				group->chunksToUpdate[i][group->zHead] = true;
			}

		}
		else {
			// Push back (add to tail)
			s_IncrementRingBufferPointer(group->zHead);
			s_IncrementRingBufferPointer(group->zTail);
			for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
				group->chunksToUpdate[i][group->zTail] = true;
			}
		}
	}

	generateRenderGroup2(group);
}

void World::generateChunksInBackground(RenderGroup* group) {
	// To be called by a separate thread
	// For each chunk to be background generated, create a thread that contains
	// it's own mesh and indices lists

	return;
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
