#include "game_layer.h"

#define in(a) (*(u32*)&(a))


#if 1

struct lowest_root_result
{
    bool32 hit;
    r32 root;
};

internal lowest_root_result
GetLowestRoot(r32 a, r32 b, r32 c, r32 maxR)
{
    lowest_root_result result = {};

    r32 determinant = b * b - 4.0f * a * c;

    if (determinant < 0.0f)
    {
	result.hit = false;
	return(result);
    }

    r32 sqrtD = (r32)sqrt(determinant);
    r32 r1 = (-b - sqrtD) / (2 * a);
    r32 r2 = (-b + sqrtD) / (2 * a);

    if (r1 > r2)
    {
	r32 temp = r2;
	r2 = r1;
	r1 = temp;
    }

    if ((r1 > 0) && (r1 < maxR))
    {
	result.root = r1;
	result.hit = true;
	return(result);
    }

    if ((r2 > 0) && (r2 < maxR))
    {
	result.root = r2;
	result.hit = true;
	return(result);
    }

    result.hit = false;
    return(result);
}




struct primitive_collision_result
{
    r32 t;
    bool32 collisionFound;
    v3 collisionPoint;
};

internal primitive_collision_result
CheckEdge(v3 p1, v3 p2, v3 base, v3 vector, r32 t)
{
    primitive_collision_result result = {};

    v3 edge = p2 - p1;
    v3 baseToVertex = p1 - base;
    r32 vectorLenSq = Dot(vector, vector);
    r32 edgeLenSq = Dot(edge, edge);
    r32 edgeDotVec = Dot(edge, vector);
    r32 edgeDotBaseToVertex = Dot(edge, baseToVertex);
    r32 a, b, c;



    a = edgeLenSq * -vectorLenSq + edgeDotVec * edgeDotVec;
    b = edgeLenSq * (2 * Dot(vector, baseToVertex)) - 2.0f * edgeDotVec * edgeDotBaseToVertex;
    c = edgeLenSq * (1 - Dot(baseToVertex, baseToVertex)) + edgeDotBaseToVertex * edgeDotBaseToVertex;

    lowest_root_result lResult = GetLowestRoot(a, b, c, t);
    if (lResult.hit)
    {
	r32 f = (edgeDotVec * lResult.root - edgeDotBaseToVertex) / edgeLenSq;

	if ((f >= 0.0f) && (f <= 1.0f))
	{
	    result.t = lResult.root;
	    result.collisionFound = true;
	    result.collisionPoint = p1 + (edge * f);
	}
	    
    }

    return(result);
}

internal primitive_collision_result
CheckVertice(v3 vector, v3 base, v3 vertex, r32 t)
{
    primitive_collision_result result = {};
    //Might sound crazy but this could be wrong, just check it incase
    r32 vectorSquaredLen = Dot(vector, vector);

    r32 a, b, c;


    a = vectorSquaredLen;
    b = 2.0f * (Dot(vector, (base - vertex)));
    v3 v3c = (vertex - base);
    c = Dot(v3c, v3c) - 1.0f;
    

    lowest_root_result lRoot = GetLowestRoot(a, b, c, t);
    if (lRoot.hit)
    {
	result.t = t;
	result.collisionFound = true;
	result.collisionPoint = vertex;
    }

    return(result);
}

bool32 CheckPointInTriangle(v3 point, m3 plane)
{
    v3 pa = plane.r1;
    v3 pb = plane.r2;
    v3 pc = plane.r3;

    v3 e10 = pb - pa;
    v3 e20 = pc - pa;

    r32 a = Dot(e10, e10);
    r32 b = Dot(e10, e20);
    r32 c = Dot(e20, e20);
    r32 ac_bb = (a * c) - (b * b);

    v3 vp = v3{point.x - pa.x, point.y - pa.y, point.z - pa.z};

    r32 d = Dot(vp, e10);
    r32 e = Dot(vp, e20);
    r32 x = (d * c) - (e * b);
    r32 y = (e * a) - (d * b);
    r32 z = x + y - ac_bb;

    return (( in(z))& ~(in(x) | in(y)) & 0x80000000);    
}
    

struct set_t_result
{
    bool32 hit;
    r64 t0, t1;
    bool32 embeddedInPlane;
};

internal set_t_result
SetT(r32 normalDotComparison, r32 signedDistToPlane, bool32 checkEmbed)
{
    set_t_result result = {};
    
    if (normalDotComparison == 0.0f)
    {
	if (fabs(signedDistToPlane) >= 1.0f)
	{
	    result.hit = false;
	    return(result);
	}
	else
	{
	    if (checkEmbed)
	    {
		result.embeddedInPlane = true;
	    }
	    result.t0 = 0.0;
	    result.t1 = 1.0;
	}
    }
    else
    {
	//we are going to collide
	result.t0 = (-1.0 - signedDistToPlane) / normalDotComparison;
	result.t1 = (1.0 - signedDistToPlane) / normalDotComparison;

	if (result.t0 > result.t1)
	{
	    r64 temp = result.t1;
	    result.t1 = result.t0;
	    result.t0 = temp;
	}

	if (result.t0 > 1.0f || result.t1 < 0.0f)
	{
	    //No collision possible return
	    result.hit = false;
	}

	if (result.t0 < 0.0) result.t0 = 0.0;
	if (result.t1 < 0.0) result.t1 = 0.0;
	if (result.t0 > 1.0) result.t0 = 1.0;
	if (result.t1 > 1.0) result.t1 = 1.0;
    }

    return(result);
}

internal m3
GetVertsFromIndexedVoxel(voxel* currVoxel, i32 currIndex, voxel_chunk* chunk)
{
    //face i, {0, 2, 1}
    //0: x, y, z;
    //2: x, y, z...
    m3 result = {};

    //We are looking for verts at face i
    //currIndex += 3 each loop, {0, 2, 1}
    u16 i1 = currVoxel->renderedIndices[currIndex]; //0
    u16 i2 = currVoxel->renderedIndices[currIndex + 1]; //2 
    u16 i3 = currVoxel->renderedIndices[currIndex + 2]; // 1

    v3 vec1 =
    {
	chunk->verts[i1 * 3], 
	chunk->verts[i1 * 3 + 1], 
	chunk->verts[i1 * 3 + 2]
    };

    v3 vec2 =
    {
	chunk->verts[i2 * 3],
	chunk->verts[i2 * 3 + 1],
	chunk->verts[i2 * 3 + 2]
    };

    v3 vec3 =
    {
	chunk->verts[i3 * 3],
	chunk->verts[i3 * 3 + 1],
	chunk->verts[i3 * 3 + 2]
    };

    result.r1 = vec1;
    result.r2 = vec2;
    result.r3 = vec3;

    return(result);
}

internal ray_cast
RayCastHitDetect(v3 start, v3 end, voxel_chunk* chunk)
{
    ray_cast result = {};

    //WELL: we compile 4/6/26 9:28pm but who knows what happens after that
    
    //NOTE: At some point create the ability to visualize these tris, bc I need 
    //to see if the triangles are being built correctly for hitting
    i32 indicesPerTri = 3;
    for (i32 i = 0; i < chunk->numOfRenderedVoxels; i++)
    {
	voxel* currVoxel = &chunk->voxels[chunk->renderedVoxelIndex[i]];
	
	//You should probably only test the rendered voxel faces, not the whole mesh

	i32 numOfRenderedTris = currVoxel->renderedIndiceCount / indicesPerTri;

	i32 currIndex = 0;
	
	for (i32 j = 0; j < numOfRenderedTris; j++)
	{
	    m3 plane = GetVertsFromIndexedVoxel(currVoxel, currIndex, chunk);
	    currIndex += 3;

	    v3 normal = Cross((plane.r2 - plane.r1), (plane.r3 - plane.r1));
	    normal = Normalize(normal);


	    r64 equationVal = -(normal.x * plane.r1.x + normal.y * plane.r1.y + normal.z * plane.r1.z);
	    r64 signedDistToTriPlane = Dot(start, normal) + equationVal;
	    r32 normalDotEnd = Dot(normal, end);


	    //Maybe make this function as opposed to magical
	    set_t_result tResult = SetT(normalDotEnd, (r32)signedDistToTriPlane, false);
	    if (!tResult.hit)
	    {
		continue;
	    }

	    bool32 collisionFound = false;
	    r32 t = 1.0f;
	    v3 collisionPoint = {};

	    v3 intersectionPoint = start + ((end - start) * (r32)tResult.t0);

	    if (CheckPointInTriangle(intersectionPoint, plane))
	    {
		collisionPoint = intersectionPoint;
		t = (r32)tResult.t0;
		collisionFound = true;
	    }
	    else
	    {
		primitive_collision_result collisionResult;
		collisionResult = CheckVertice(end, start, plane.r1, t);
		collisionResult = CheckVertice(end, start, plane.r2, t);
		collisionResult = CheckVertice(end, start, plane.r3, t);



		collisionResult = CheckEdge(plane.r1, plane.r2, start, end, t);
		collisionResult = CheckEdge(plane.r2, plane.r3, start, end, t);
		collisionResult = CheckEdge(plane.r3, plane.r1, start, end, t);

		intersectionPoint = collisionResult.collisionPoint;
		collisionFound = collisionResult.collisionFound;
		t = collisionResult.t;
	    }

	    if (collisionFound)
	    {
		r32 distToCollision = t * (r32)sqrt(Dot(end, end));
		if (result.hit == false || distToCollision < result.nearestCollision)
		{
		    result.nearestCollision = distToCollision;
		    result.hitLocation = intersectionPoint;
		    result.hit = true;
		}
		
	    }
	}
    }

    return(result);
}
#endif

bounding_box CreateBoundingBox(v3 location, r32 length, r32 width, r32 height, memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena)
{
    bounding_box result = {};

    
    
    result.vertexNum = 24;
    result.indicesNum = 36;

    result.verts = (r32*)memoryPoolCode->PushStruct(objLocationArena, (sizeof(r32) * result.vertexNum));
    result.indices = (u16*)memoryPoolCode->PushStruct(objLocationArena, (sizeof(u16) * result.indicesNum));
    
    r32 unassignedVerts[] =
    {
        location.x - width/2, location.y - height/2, location.z - length/2, // bottom left back (0)
        location.x + width/2, location.y - height/2, location.z - length/2, // bottom right back (1)
        location.x + width/2, location.y + height/2, location.z - length/2, // top right back (2)
        location.x - width/2, location.y + height/2, location.z - length/2, // top left back (3)
    
        location.x - width/2, location.y - height/2, location.z + length/2, // bottom left front (4)
        location.x + width/2, location.y - height/2, location.z + length/2, // bottom right front (5)
        location.x + width/2, location.y + height/2, location.z + length/2, // top right front (6)
        location.x - width/2, location.y + height/2, location.z + length/2  // top left front (7)
    };

    for (int i = 0; i < result.vertexNum; i++)
    {
	result.verts[i] = unassignedVerts[i];
    }

    u16 unassignedIndices[] =
    {
	0, 1, 2,  2, 3, 0,
	4, 5, 6,  6, 7, 4,
	0, 3, 7,  7, 4, 0,
	1, 5, 6,  6, 2, 1,
	3, 2, 6,  6, 7, 3,
	0, 1, 5,  5, 4, 0,
    };

    for (int i = 0; i < result.indicesNum; i++)
    {
	result.indices[i] = unassignedIndices[i];
    }
    
    return(result);
}

obj* CreateSingleVoxel(memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena, r32 voxelSize)
{
    //we have to take this and translate it into the specific api, similar to how we load in the obj file
    obj* result = (obj*)memoryPoolCode->PushStruct(objLocationArena, sizeof(obj));
    result->vertexCount = 8;
    result->vertices = (r32*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(r32) * result->vertexCount) * 3);
    

    r32 unassignedVerts[24] =
    {
	-voxelSize, -voxelSize, -voxelSize, // 0
	-voxelSize, -voxelSize, voxelSize, // 1
	-voxelSize, voxelSize, -voxelSize, // 2
	-voxelSize, voxelSize, voxelSize, // 3
	voxelSize, -voxelSize, -voxelSize, // 4
	voxelSize, -voxelSize, voxelSize, // 5 
	voxelSize, voxelSize, -voxelSize, // 6
	voxelSize, voxelSize, voxelSize, // 7
    };


    
    for (int i = 0; i < (result->vertexCount) * 3; i++)
    {
	result->vertices[i] = unassignedVerts[i];
    }

    u16 unassignedIndices[] =
    {
	0,2,1, //right //1, 3, 2,
	1,2,3,         //2, 3, 4,

	4,5,6, //left //5, 6, 7,
	5,7,6,        //6, 8, 7,

	0,1,5, //bottom //1, 2, 6,
	0,5,4,          //1, 6, 5,
	
	2,6,7, //Top //3, 7, 8,
	2,7,3,       //3, 8, 4,

	0,4,6, //Front //1, 5, 7,
	0,6,2,         //1, 7, 3,

	1,3,7, //Back //2, 5, 8,
	1,7,5,	      //2, 8, 6
    };

    result->faceLastIndex = 36;
    result->vertexIndices = (u16*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(u16) * result->faceLastIndex));
#if 0
    for (int i = 0; i < result->faceLastIndex; i++)
    {
	u16 currIndex = unassignedIndices[i] - 1;
	result->vertexIndices[i] = currIndex;
    }
#else
    for (i32 i = 0; i < result->faceLastIndex; i++)
    {
	result->vertexIndices[i] = unassignedIndices[i];
    }
#endif
    result->faceCount = 6;
    result->renderFace = (bool32*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(bool32) * result->faceCount));

    return(result);
}

internal bool32
IsVoxelOnSurface(v3 pos, v3 minCorner, v3 maxCorner)
{
    if (((pos.x == maxCorner.x) || (pos.x == minCorner.x)) ||
	((pos.y == maxCorner.y) || (pos.y == minCorner.y)) ||
	((pos.z == maxCorner.z) || (pos.z == minCorner.z)))
    {
	return(true);
    }
    return(false);
}

enum face_locations
{
    e_left = 0, //x+
    e_right, //x-
    e_front, //y+
    e_back, //y-
    e_top, //z+
    e_bottom, //z-
};

internal void
DetermineDrawnIndices(voxel* currVoxel, i32 face)
{

    
    currVoxel->renderedFaces[face] = true;
    currVoxel->numOfRenderedFaces = currVoxel->numOfRenderedFaces + 1;

    currVoxel->isSolid = true;
    //Per num of verts per face
    for (int i = (face * 6); i < (face * 6) + 6; i++)
    {
	currVoxel->renderedIndices[currVoxel->renderedIndiceCount] = currVoxel->indices[i];

	currVoxel->renderedIndiceCount = currVoxel->renderedIndiceCount + 1;
    }

}

internal void
DetermineVoxelDrawFaces(voxel_chunk* chunk, voxel* currVoxel, i32 currIndex, memory_pool_dll_code* memoryPoolCode, memory_arena* arena)
{
    i32 div = (i32)(currIndex / (chunk->width * chunk->height));
    

    i32 indexLocations[6] =
    {
	currIndex - 1, currIndex + 1, //x	
	(i32)(currIndex - chunk->width - ((chunk->width * chunk->height) * div)), (i32)-(currIndex + chunk->width - ((chunk->width * chunk->height) * (div + 1))),
	(i32)(currIndex - (chunk->width * chunk->height)), (i32)(currIndex + (chunk->width * chunk->height)), //z 160
    };    

    currVoxel->numOfRenderedFaces = 0;
    currVoxel->renderedIndiceCount = 0;

    currVoxel->isSolid = false;

    i32 stride = (i32)(chunk->height * chunk->width);



    for (int i = 0; i < 6; i++)
    {
	//Taking care of some special cases
	bool32 iGreaterThanEqualToZero = indexLocations[i] >= 0;
	if (i == 3)
	{
	    iGreaterThanEqualToZero = indexLocations[3] > 0;
	}

	if (((indexLocations[i] < chunk->voxelResolution) && (iGreaterThanEqualToZero)))
	{

	    if (((i == 0) && (((indexLocations[i] + 1) % (i32)chunk->width == 0))))
	    {
		DetermineDrawnIndices(currVoxel, i);
		continue;
	    }

	    if (((i == 1) && (indexLocations[i] % (i32)chunk->width == 0)))
	    {
		DetermineDrawnIndices(currVoxel, i);
		continue;
	    }

	    currVoxel->renderedFaces[i] = false;

	}
	else
	{
	    //Render face
	    DetermineDrawnIndices(currVoxel, i);
	}
    }

    if (currVoxel->isSolid)
    {
	chunk->numOfRenderedVoxels = chunk->numOfRenderedVoxels + 1;
    }

}

internal void
InitVoxels(memory_pool_dll_code* memoryPoolCode, memory_arena* arena, voxel_chunk* chunk, obj* voxelObjInfo)
{
    chunk->voxels = (voxel*)memoryPoolCode->PushArraySized(arena, (size_t)(sizeof(voxel) * chunk->voxelResolution));

    chunk->voxelVertCount = voxelObjInfo->vertexCount;
    
    chunk->numOfRenderedVoxels = 0;


    for (int i = 0; i < (voxelObjInfo->vertexCount * 3); i++)
    {
	chunk->verts[i] = voxelObjInfo->vertices[i];
    }


    v3 totalPos = {};
    for (int i = 0; i < chunk->voxelResolution; i++)
    {

	
	v3 pos = {};
#if 0	
	pos.x = (r32)fmod(i, chunk->width);
	pos.y = (r32)floor(fmod((i / chunk->width), chunk->height));
	pos.z = (r32)floor(i / (chunk->width * chunk->height));

	pos = pos * (chunk->voxelSize * 2);
	pos -= chunk->voxelChunkExtent;
#else
	//The lesson here, was that you weren't factoring your specified location, you were just
	//starting at the location, and then growing it out to one direction as opposed to all directions
	r32 ix = (r32)((i32)i % (i32)chunk->width);
	r32 iy = (r32)(((i32)i / (i32)chunk->width) % (i32)chunk->height);
	r32 iz = (r32)((i32)i / ((i32)chunk->width * (i32)chunk->height));

	pos.x = (ix - (chunk->width - 1) * 0.5f) * (chunk->voxelSize * 2.0f);
	pos.y = (iy - (chunk->height - 1) * 0.5f) * (chunk->voxelSize * 2.0f);
	pos.z = (iz - (chunk->length - 1) * 0.5f) * (chunk->voxelSize * 2.0f);
#endif	
	
	chunk->voxels[i].pos = (pos + chunk->chunkWorldLocation);

	totalPos += chunk->voxels[i].pos;
	
	for (int j = 0; j < voxelObjInfo->faceLastIndex; j++)
	{
	    chunk->voxels[i].indices[j] = voxelObjInfo->vertexIndices[j];
	}


	r32 vertColor = i / chunk->voxelResolution;
	
	for (i32 k = 0; k < 8; k++)
	{
	    chunk->voxels[i].vertColors = v3{vertColor, vertColor, 0.5f};
	}
	
	DetermineVoxelDrawFaces(chunk, &chunk->voxels[i], i, memoryPoolCode, arena);
	chunk->voxels[i].voxelIndex = i;

    }

    chunk->centoid = totalPos / (r32)chunk->voxelResolution;
    
    chunk->renderedVoxelIndex = (i32*)memoryPoolCode->PushArraySized(arena, (size_t)(sizeof(i32) * chunk->numOfRenderedVoxels));

    for (int i = 0, j = 0; i < chunk->voxelResolution; i++)
    {
	if (chunk->voxels[i].isSolid)
	{
	    chunk->renderedVoxelIndex[j] = i;
	    j++;
	}
    }
    
}

voxel_chunk CreateVoxelChunk(v3 location, r32 voxelSize, memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena, obj* voxelObjInfo, game_state* gameState)
{
    //l * w * h = bounding box extents

    voxel_chunk result;
    result.length = 5;
    result.height = 2;
    result.width = 5;

    v3 lwh = v3{result.length, result.height, result.width};
    
    //This isn't right
    //Get the voxel in the center's position ie: halfRes = voxelResolution / 2; halfRes = 


//    result.centoid = v3{result.width/2, result.width/2, result.height/2};

    
    result.chunkWorldLocation = location;
    result.chunkWorldRotation = v3{0.0f, 0.0f, 0.0f};
    result.chunkWorldScale = v3{1.0f, 1.0f, 1.0f};
    
    //This assumes that the cube is located @ 0 0 0 in local space
    result.maxCorner = v3{result.length, result.width, result.height};
    result.minCorner = -result.maxCorner;
    
    result.voxelChunkExtent = v3{result.length, result.width, result.height};
    
    bounding_box voxelBounds = CreateBoundingBox(location, result.length, result.width, result.height, memoryPoolCode, objLocationArena);


    result.length = (result.length * 2) / voxelSize;
    result.width = (result.width * 2) / voxelSize;
    result.height = (result.height * 2) / voxelSize;
    result.voxelResolution = result.length * result.width * result.height;


    result.voxelSize = voxelSize;
    result.numOfRenderedVoxels = 0;

    

    InitVoxels(memoryPoolCode, objLocationArena, &result, voxelObjInfo);

    //Determine Centoid of the whole chunk here
    v3 first = result.voxels[0].pos;
    v3 last = result.voxels[(i32)(result.voxelResolution - 1)].pos;
    
    result.centoid = (first + last) * 0.5f;


    
    return(result);
    
}

extern "C" GAME_INITIALIZE(GameInitialize)
{
    game_initialize_data result = {};

    r32 voxelSize = 0.5f;
    result.allObjs = CreateSingleVoxel(memoryPoolCode, objLocationArena, voxelSize);
    v3 location = v3{0.0f, 0.0f, 0.0f};

    result.gameState = (game_state*)memoryPoolCode->PushStruct(objLocationArena, sizeof(game_state));
    result.chunk = CreateVoxelChunk(location, voxelSize, memoryPoolCode, objLocationArena, result.allObjs, result.gameState);


    
    result.gameState->debugVectorMemory =
	(listed_memory*)memoryPoolCode->PushStruct(debugVectorArena, sizeof(listed_memory));
    memoryPoolCode->InitListedMemory(result.gameState->debugVectorMemory,
				     debugVectorArena, sizeof(game_debug_vector));
    result.gameState->debugVectorLength = 12.0f;

#if 1    
    game_debug_vector newVec = {};
    newVec.start = result.chunk.centoid;
//    newVec.start = ;
    newVec.end = v3{10.0f, 10.0f, 10.0f};
    newVec.color = v3{0.0f, 0.0f, 1.0f};
    memoryPoolCode->AddListedItem(result.gameState->debugVectorMemory,
				  (void*)&newVec,
				  sizeof(game_debug_vector),
				  &result.gameState->debugVectorNodes
	);
    result.gameState->numOfDrawnVectors = result.gameState->numOfDrawnVectors + 1;
#endif
    
    return(result);
}

extern "C" GAME_UPDATE(GameUpdate)
{
    //Mouse update and create debug vectors on mouse click


    


	//THIS:
//I do believe this is not working right, your math isn't correct, ScreenToCoordNDC also only seems to be
	//returning 1.0 to -1.0

	//the matricies are currently not being filled out, do so please
//	v2 ndc = ScreenToCoordNDC(GetMouseScreenCoords(input));


    //NOTE: This is still broken as of 3/12/25, you need to fix this very soon... Gemini got no where
#if 0
    if (input->mouseButtons[e_mouse_buttons::left_mouse].started)
    {	
	v3 ndc = {};
	ndc.x = ((2.0f * (r32)input->mouseXBounded) / screenW) - 1.0f;
	ndc.y = (((2.0f * (r32)input->mouseYBounded) / screenH) - 1.0f) * -1.0f;
	ndc.z = 0.0f;
		 
	
	v4 rClip = v4{ndc.x, ndc.y, 1.0f, 1.0f};
	m4 invProj = Inverse(gameState->gameCamera.proj);
	v4 rEye = TransformVec(rClip, invProj);
	

	v3 eyePoint = v3{rEye.x / rEye.w, rEye.y / rEye.w, rEye.z / rEye.w};
	

	v4 rWorld = TransformVec(v4{eyePoint.x, eyePoint.y, eyePoint.z, 1.0f},
				 gameState->gameCamera.viewInverted);


	v3 rWorld3 = v3{rWorld.x, rWorld.y, rWorld.z};
	v3 start = gameState->gameCamera.pos;
	rWorld3 = Normalize(rWorld3 - start);
	v3 end = start + (rWorld3 * gameState->debugVectorLength);



	v3 color = {};

	game_debug_vector newVec = {};
	newVec.start = start;
	newVec.end = end;
	newVec.color = color;
	memoryPoolCode->AddListedItem(gameState->debugVectorMemory,
				      (void*)&newVec,
				      sizeof(game_debug_vector),
				      &gameState->debugVectorNodes
	    );
	gameState->numOfDrawnVectors = gameState->numOfDrawnVectors + 1;
    }

#else
    if (input->mouseButtons[e_mouse_buttons::left_mouse].started)
    {
	// Use the actual backbuffer dimensions from your programState
	// This is the only way to guarantee the math matches the pixels on screen
	// 1. NDC Calculation
	// We use the actual backbuffer width/height to ensure the ratio is perfect
	v3 ndc = {};
	ndc.x = (2.0f * (r32)input->mouseXBounded) / gameState->windowW - 1.0f;
	ndc.y = 1.0f - (2.0f * (r32)input->mouseYBounded) / gameState->windowH; 

	// 2. Inverse Matrices
	// Using your operator* (Row-Major/Row-Vector style)
	m4 invProj = Inverse(gameState->gameCamera.proj);
	m4 invVP = invProj * gameState->gameCamera.viewInverted;

	// 3. Unproject 
	// In DX11, the near plane is 0.0f. Using 1.0f for the far plane.
	v4 nearC = v4{ndc.x, ndc.y, 0.0f, 1.0f}; 
	v4 farC  = v4{ndc.x, ndc.y, 1.0f, 1.0f};

	v4 nearW = TransformVec(nearC, invVP);
	v4 farW  = TransformVec(farC, invVP);

	if (nearW.w != 0.0f && farW.w != 0.0f)
	{
	    // 4. Perspective Divide
	    v3 pNear = v3{nearW.x / nearW.w, nearW.y / nearW.w, nearW.z / nearW.w};
	    v3 pFar  = v3{farW.x / farW.w, farW.y / farW.w, farW.z / farW.w};

	    // 5. Final Ray
	    v3 start = pNear; 
	    v3 dir = Normalize(pFar - pNear);
	    v3 end = start + (dir * gameState->debugVectorLength);

	    v3 color = {};
	    
	    game_debug_vector newVec = {};
	    newVec.start = start;
	    newVec.end = end;
	    newVec.color = color;
	    memoryPoolCode->AddListedItem(gameState->debugVectorMemory,
					  (void*)&newVec,
					  sizeof(game_debug_vector),
					  &gameState->debugVectorNodes
		);
	    gameState->numOfDrawnVectors = gameState->numOfDrawnVectors + 1;	    
#if 1
	    ray_cast cast = RayCastHitDetect(start, end, chunk);
	    if (cast.hit)
	    {
		//Here is where we dirty the voxel chunk and rebuild at the location of the hit
	    }
#endif	    
	}
    }


#endif
    
}
