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

obj* CreateSingleVoxel(memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena)
{
    //we have to take this and translate it into the specific api, similar to how we load in the obj file
    obj* result = (obj*)memoryPoolCode->PushStruct(objLocationArena, sizeof(obj));
    result->vertexCount = 8;
    result->vertices = (r32*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(r32) * result->vertexCount) * 3); 

    //Eventually this should be the resolution
    r32 unassignedVerts[24] =
    {
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, 0.5f,
	-0.5f, 0.5f, -0.5f,
	-0.5f, 0.5f, 0.5f,
	0.5f, -0.5f, -0.5f,
	0.5f, -0.5f, 0.5f,
	0.5f, 0.5f, -0.5f,
	0.5f, 0.5f, 0.5f,
    };

    for (int i = 0; i < (result->vertexCount) * 3; i++)
    {
	result->vertices[i] = unassignedVerts[i];
    }

    u16 unassignedIndices[] =
    {
	0,2,1,
	1,2,3,

	4,5,6,
	5,7,6,

	0,1,5,
	0,5,4,
	
	2,6,7,
	2,7,3,

	0,4,6,
	0,6,2,

	1,3,7,
	1,7,5,	
    };

    result->faceLastIndex = 36;
    result->vertexIndices = (u16*)memoryPoolCode->PushArraySized(objLocationArena, (sizeof(u16) * result->faceLastIndex));
    for (int i = 0; i < result->faceLastIndex; i++)
    {
	u16 currIndex = unassignedIndices[i] + 1;
	result->vertexIndices[i] = currIndex;
    }

    

    return(result);
}

voxel_chunk CreateVoxelChunk(v3 location, r32 voxelSize, memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena)
{
    //l * w * h = bounding box extents

    //Step through here to see if our voxelResolution value is a value that will evenly round out to
    //a normal value for the resolution
    voxel_chunk result;
    result.length = 2;
    result.width = 0.5;
    result.height = 2;

    result.length = (result.length * 2) / voxelSize;
    result.width = (result.width * 2) / voxelSize;
    result.height = (result.height * 2) / voxelSize;
    
    result.voxelChunkExtent = v3{result.length, result.width, result.height};
    bounding_box voxelBounds = CreateBoundingBox(location, result.length, result.width, result.height, memoryPoolCode, objLocationArena);

    result.voxelResolution = result.length * result.width * result.height; 

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
    result.allObjs = CreateSingleVoxel(memoryPoolCode, objLocationArena);
    v3 location = v3{0.0f, 0.0f, 0.0f};
    result.voxels = CreateVoxelChunk(location, 0.5f, memoryPoolCode, objLocationArena);
#endif    
    return(result);
}

extern "C" GAME_UPDATE(GameUpdate)
{
    //Loading in objects?
}
