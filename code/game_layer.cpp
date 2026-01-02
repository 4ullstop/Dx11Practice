#include "game_layer.h"

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
	-voxelSize, -voxelSize, -voxelSize,
	-voxelSize, -voxelSize, voxelSize,
	-voxelSize, voxelSize, -voxelSize,
	-voxelSize, voxelSize, voxelSize,
	voxelSize, -voxelSize, -voxelSize,
	voxelSize, -voxelSize, voxelSize,
	voxelSize, voxelSize, -voxelSize,
	voxelSize, voxelSize, voxelSize,
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
	0,5,4,          //1, 6, 4,
	
	2,6,7, //Top //3, 7, 8,
	2,7,3,       //3, 8, 4,

	0,4,6, //Front //1, 5, 7,
	0,6,2,         //1, 7, 3,

	1,3,7, //Back //2, 5, 8,
	1,7,5,	      //2, 8, 6
    };

    result->faceLastIndex = 36;
    result->vertexIndices = (u16*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(u16) * result->faceLastIndex));
    for (int i = 0; i < result->faceLastIndex; i++)
    {
	u16 currIndex = unassignedIndices[i] + 1;
	result->vertexIndices[i] = currIndex;
    }

    result->faceCount = 6;
    result->renderFace = (bool32*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(bool32) * result->faceCount));

    return(result);
}

internal void
InitVoxelLocations(voxel_chunk* voxelChunk, memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena)
{
    voxelChunk->voxelPositions =
	(v3*)memoryPoolCode->PushArraySized(objLocationArena, (size_t)(sizeof(v3) * voxelChunk->voxelResolution));

    voxelChunk->voxelFaceInfo =
	(voxel_face_info*)memoryPoolCode->PushArraySized(objLocationArena, (size_t)(sizeof(voxel_face_info) * voxelChunk->voxelResolution));


    v3 colors[] = 
    {
	{0, 0, 0}, //0 Black
	{1, 0, 0}, //1 Red
	{0, 1, 0}, //2 Green
	{0, 0, 1}, //3 Blue
	{1, 0, 1}, //4 Magenta
	{0, 1, 1}, //5 Cyan
	{1, 1, 0}, //6 Yellow
	{1, 1, 1}, //7 White
    };

#if 0
    
    for (int i = 0; i < 8; i++)
    {
	voxelChunk->vertexColors[i] = colors[i];
    }
    //voxelVertCount should be 8?
#endif    
    
    voxelChunk->renderedVoxelCount = 0;
    voxelChunk->voxelFaceInfo[0].voxelType = voxel_type::vt_solid;
    for (int i = 0; i < voxelChunk->voxelResolution; i++)
    {
	//Current position
	v3 pos = {};
	pos.x = (r32)fmod(i, voxelChunk->width);
	pos.y = (r32)floor(fmod((i / voxelChunk->width), voxelChunk->height));
	pos.z = (r32)floor(i / (voxelChunk->width * voxelChunk->height));

	pos = pos * (voxelChunk->voxelSize * 2);
	pos -= voxelChunk->voxelChunkExtent;
	voxelChunk->voxelPositions[i] = pos;

#if 0	
	if (i < voxelChunk->voxelResolution)
	{
	    v3 nextVoxel;
	    nextVoxel.x = (r32)fmod((i + 1), voxelChunk->width);
	    nextVoxel.y = (r32)floor(fmod(((i + 1) / voxelChunk->width), voxelChunk->height));
	    nextVoxel.z = (r32)floor((i + 1) / (voxelChunk->width * voxelChunk->height));	    

	}
#endif	
	//Check the voxel ahead
    }

    
    for (int i = 0, j = 0; i < voxelChunk->voxelResolution; i++)
    {
	
    }
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
    //Run through neighbors, test for solidity, determine faces drawn

    /*
      x + 1, y, z
      x - 1, y, z
      x, y + 1, z
      x, y - 1, z
      x, y, z + 1
      x, y, z -1
      
     */
    //Curr loc + 1
    //Curr loc + 20
    //Curr loc + 160

    //Since y =u/d, z = l/r and x = f/b, these are in order based on the construction of the faces
    i32 indexLocations[6] =
    {
	currIndex - 1, currIndex + 1, //x	
	(i32)(currIndex - chunk->height), (i32)(currIndex + chunk->height), //y 20 or 8?
	(i32)(currIndex - (chunk->width * chunk->height)), (i32)(currIndex + (chunk->width * chunk->height)), //z 160
    };

    currVoxel->numOfRenderedFaces = 0;
    currVoxel->renderedIndiceCount = 0;

    currVoxel->isSolid = false;

    i32 stride = (i32)(chunk->height * chunk->width);
    
    for (int i = 0; i < 6; i++)
    {
	if (((indexLocations[i] <= chunk->voxelResolution) && (indexLocations[i] >= 0)))
	{
	    //Don't render face

	    //Check our neighbor's location
	    //So we need to also check the ends of the voxels to ensure that they aren't reaching down into another
	    //row and counting it as a rendered voxel
	    
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
    chunk->voxels = (voxel*)memoryPoolCode->PushArraySized(arena, (size_t)(sizeof(v3) * chunk->voxelResolution));

    chunk->voxelVertCount = voxelObjInfo->vertexCount;
    
    chunk->numOfRenderedVoxels = 0;

    chunk->renderedVoxelIndex = (i32*)memoryPoolCode->PushArraySized(arena, (size_t)(sizeof(i32) * chunk->numOfRenderedVoxels));
    
    for (int i = 0; i < (voxelObjInfo->vertexCount * 3); i++)
    {
	chunk->verts[i] = voxelObjInfo->vertices[i];
    }
    
    for (int i = 0; i < chunk->voxelResolution; i++)
    {
	
	v3 pos = {};
	pos.x = (r32)fmod(i, chunk->width);
	pos.y = (r32)floor(fmod((i / chunk->width), chunk->height));
	pos.z = (r32)floor(i / (chunk->width * chunk->height));

	pos = pos * (chunk->voxelSize * 2);
	pos -= chunk->voxelChunkExtent;

	chunk->voxels[i].pos = pos;
	//Is voxel on surface? Yes - solid, No - mt
	
	//Remove this, instead we'll just check our faces to be rendered and determine off of that
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
    //Now for the determination of which voxels to render and what faces will be rendered

    /*
      - Take a voxel, check each face of the voxel to it's neighboring cube to see if it is solid or exists
      - 
     */

    chunk->renderedVoxelIndex = (i32*)memoryPoolCode->PushArraySized(arena, (size_t)(sizeof(i32) * chunk->numOfRenderedVoxels));

    //then store the ones that are going to be rendered here and loop through them, don't do draw faces functions

    for (int i = 0, j = 0; i < chunk->voxelResolution; i++)
    {
	if (chunk->voxels[i].isSolid)
	{
	    chunk->renderedVoxelIndex[j] = i;
	    j++;
	}
	//Check to see if j is increasing correctly?
    }
}

voxel_chunk CreateVoxelChunk(v3 location, r32 voxelSize, memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena, obj* voxelObjInfo)
{
    //l * w * h = bounding box extents

    //Step through here to see if our voxelResolution value is a value that will evenly round out to
    //a normal value for the resolution
    voxel_chunk result;
    result.length = 5;
    result.height = 2;
    result.width = 5;

    //This assumes that the cube is located @ 0 0 0 in local space
    result.maxCorner = v3{result.length, result.width, result.height};
    result.minCorner = -result.maxCorner;
    
    result.voxelChunkExtent = v3{result.length, result.width, result.height};
    
    bounding_box voxelBounds = CreateBoundingBox(location, result.length, result.width, result.height, memoryPoolCode, objLocationArena);


    result.length = (result.length * 2) / voxelSize;
    result.width = (result.width * 2) / voxelSize;
    result.height = (result.height * 2) / voxelSize;
    result.voxelResolution = result.length * result.width * result.height;

//    result.visibleVoxels = (bool32*)memoryPoolCode.PushArraySized(objLocationArena, (sizeof(bool32) * result.voxelResolution));

 

    result.voxelSize = voxelSize;


    

//Instanced pending removal, call InitVoxels() instead    
//    InitVoxelLocations(&result, memoryPoolCode, objLocationArena);
//
    InitVoxels(memoryPoolCode, objLocationArena, &result, voxelObjInfo);
    return(result);
    
    //Get a calculation for the location in the world, add it by the verts of the instanced cubes
}

extern "C" GAME_INITIALIZE(GameInitialize)
{
    //Call our OBJ loader function here from the dll such that it is anonymouse and fills out
    //the necessary data for (in this case) dx11 to read

    //LoadOBJ();
    //ParseOBJ;
    //Write your file locations here

    game_initialize_data result = {};
#if 0    
    char* fileLocation = "D:/ExternalCustomAPIs/OBJLoader/misc/cubetester_normals.obj";
    obj* result = parseOBJCode->ParseOBJData(fileLocation, objLocationArena, mainProgramMemory);
#else
    r32 voxelSize = 0.5f;
    result.allObjs = CreateSingleVoxel(memoryPoolCode, objLocationArena, voxelSize);
    v3 location = v3{0.0f, 0.0f, 0.0f};
    result.chunk = CreateVoxelChunk(location, voxelSize, memoryPoolCode, objLocationArena, result.allObjs);
#endif    
    return(result);
}

extern "C" GAME_UPDATE(GameUpdate)
{
    //Loading in objects?
}
