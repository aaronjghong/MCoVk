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
static void s_GenerateHorizontalFace(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, unsigned int x, unsigned int y, unsigned int z, World::DrawFace face);
static void s_GenerateVerticalFaceLR(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, unsigned int x, unsigned int y, unsigned int z, World::DrawFace face);
static void s_GenerateVerticalFaceFB(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, unsigned int x, unsigned int y, unsigned int z, World::DrawFace face);

World::World() {
	srand(time(NULL));
	randSeed = 30878;//rand();
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


void World::createRenderMesh( Chunk *chunk, DrawFace omittedFaces, unsigned int xOffset, unsigned int zOffset ) {

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

					bool onEdge = false;

					if ( x == 0 ) {
						l = !(omittedFaces & DRAW_FACE_LEFT);
						onEdge = true;
					}
					if ( x == 15 ) {
						r = !(omittedFaces & DRAW_FACE_RIGHT);
						onEdge = true;
					}
					if ( z == 0 ) {
						front = !(omittedFaces & DRAW_FACE_FRONT);
						onEdge = true;
					}
					if ( z == 15 ) {
						back = !(omittedFaces & DRAW_FACE_BACK);
						onEdge = true;
					}

					/* Check and set any connecting chunk edge faces */
					//if ( connectingFacesIdx >= 0 ) {
					//	if ( onEdge ) {
					//		std::vector<std::pair<glm::vec3, DrawFace>>& v = connectingFaces[connectingFacesIdx];
					//		for ( auto& e : v ) {
					//			glm::vec3 coord = std::get<0>(e);
					//			if ( coord.x == x + xOffset && coord.y == y && coord.z == z + zOffset ) {
					//				/* Coordinate found, set flag */
					//				switch ( std::get<1>(e) )
					//				{
					//				case DRAW_FACE_LEFT:
					//					l = true;
					//					break;
					//				case DRAW_FACE_RIGHT:
					//					r = true;
					//					break;
					//				case DRAW_FACE_FRONT:
					//					front = true;
					//					break;
					//				case DRAW_FACE_BACK:
					//					back = true;
					//					break;
					//				default:
					//					break;
					//				}
					//			}
					//		}
					//	}
					//}

					int j = connectingFacesIdx % RENDER_DISTANCE;
					int i = (connectingFacesIdx - j) / RENDER_DISTANCE;
					// Poses issue as is since only one face is represented at a time
					if ( connFaces[i][j][x][y][z] != DRAW_FACE_NONE ) {
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
					}


					s_CallVertexGenCommands(meshVertices, meshIndices, top, bot, l, r, front, back, x + xOffset, y, z + zOffset);
					s_ResetVertexGenFlags(top, bot, l, r, front, back);

				}
			}
		}
	}

	/* Reset index to not re-trigger unless explicitly specified */
	connectingFacesIdx = -1;
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
	group->xOrigin = 0;
	group->zOrigin = 0;
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
			unsigned int xOffset = (i * 16) + group->xOrigin;
			unsigned int zOffset = (j * 16) + group->zOrigin;
			generateChunk(&group->chunks[i][j], xOffset, 0, zOffset);

			if ( i > 0 ) {
				// Check edges with chunk to the left
				for ( int k = 0; k < 16; k++ ) {
					for ( int y = 255; y > 0; y-- ) {
						if ( group->chunks[i][j].data[0][y][k].blockID && !(group->chunks[i - 1][j].data[15][y][k].blockID) ) {
							// Right is higher than left, signal to right group to gen leftfaces
							//connectingFaces[j + (RENDER_DISTANCE * i)].push_back(std::make_pair(glm::vec3(xOffset, y, k + zOffset), DRAW_FACE_LEFT));
							connFaces[i][j][0][y][k] = (DrawFace)(connFaces[i][j][0][y][k] | DRAW_FACE_LEFT);
						}
						else if ( !(group->chunks[i][j].data[0][y][k].blockID) && group->chunks[i - 1][j].data[15][y][k].blockID ) {
							// Left is higher than right, signal to left group to gen right faces
							//connectingFaces[j + (RENDER_DISTANCE * (i - 1))].push_back(std::make_pair(glm::vec3(xOffset - 1, y, k + zOffset), DRAW_FACE_RIGHT));
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
							//connectingFaces[j + (RENDER_DISTANCE * i)].push_back(std::make_pair(glm::vec3(k + xOffset, y, zOffset), DRAW_FACE_FRONT));
							connFaces[i][j][k][y][0] = (DrawFace)(connFaces[i][j][k][y][0] | DRAW_FACE_FRONT);
						}
						else if ( !(group->chunks[i][j].data[k][y][0].blockID) && group->chunks[i][j - 1].data[k][y][15].blockID ) {
							// Back is higher, signal to back to gen front faces
							//connectingFaces[(j - 1) + (RENDER_DISTANCE * i)].push_back(std::make_pair(glm::vec3(k + xOffset, y,  zOffset - 1), DRAW_FACE_BACK));
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

			unsigned int xOffset = (i * 16) + group->xOrigin;
			unsigned int zOffset = (j * 16) + group->zOrigin;

			connectingFacesIdx = j + (RENDER_DISTANCE * i);

			createRenderMesh(&group->chunks[i][j], ommitedFaces, xOffset, zOffset);

			currNumVertices += meshVertices->size();

			// Not sure if it's correct to push values instead of addresses
			renderMeshVertices->push_back(*meshVertices);
			renderMeshIndices->push_back(*meshIndices);


		}
	} 



	//for ( int i = 0; i < RENDER_DISTANCE; i++ ) {
	//	for ( int j = 0; j < RENDER_DISTANCE; j++ ) {
	//		// Call generate chunk
	//		// THen generate the render mesh
	//		// But instead of 1 chunk at a time, do the entire group collective
	//		
	//		DrawFace ommitedFaces =(DrawFace)( DRAW_FACE_LEFT | DRAW_FACE_RIGHT | DRAW_FACE_FRONT | DRAW_FACE_BACK );
	//		uint8_t unset = UINT8_MAX;
	//		/* Un-set omitted faces flag */
	//		if ( i == 0 ) {
	//			// Draw left?
	//			unset ^= DRAW_FACE_LEFT;
	//		}
	//		else if ( i == (RENDER_DISTANCE - 1) ) {
	//			// Draw right?
	//			unset ^= DRAW_FACE_RIGHT;
	//		}

	//		if ( j == 0 ) {
	//			// Draw front?
	//			unset ^= DRAW_FACE_FRONT;
	//		}
	//		else if ( j == (RENDER_DISTANCE - 1) ) {
	//			// Draw back?
	//			unset ^= DRAW_FACE_BACK;
	//		}

	//		// Unset unwated ommisions
	//		ommitedFaces = (DrawFace)(unset & ommitedFaces);

	//		unsigned int xOffset = (i * 16) + group->xOrigin;
	//		unsigned int zOffset = (j * 16) + group->zOrigin;

	//		generateChunk(&group->chunks[i][j], xOffset, 0, zOffset);

	//		createRenderMesh(&group->chunks[i][j], ommitedFaces, xOffset, zOffset);

	//		/* Need something here to connect chunks */

	//		currNumVertices += meshVertices->size();

	//		// Not sure if it's correct to push values instead of addresses
	//		renderMeshVertices->push_back(*meshVertices);
	//		renderMeshIndices->push_back(*meshIndices);


	//	}
	//} 

	
}

void World::generateChunk(Chunk* chunk, int x, int y, int z) {
	/* Where x,y,z are the coordinates where the chunk "begins" */
	/* well, y doesn't really matter, but still */
	/* y could be used for generating features on a second pass or smth */

	int heights[16][16]{};

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

static void s_GenerateHorizontalFace(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, unsigned int x, unsigned int y, unsigned int z, World::DrawFace face) {
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

static void s_GenerateVerticalFaceLR(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, unsigned int x, unsigned int y, unsigned int z, World::DrawFace face) {
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

static void s_GenerateVerticalFaceFB(std::vector<World::VertData>* meshVertices, std::vector<unsigned int>* meshIndices, unsigned int x, unsigned int y, unsigned int z, World::DrawFace face) {
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

