#if !defined (GAME_LAYER_H)

#include "D:/ExternalCustomAPIs/Types/direct_x_typedefs.h"
#include "D:/ExternalCustomAPIs/MemoryPools/code/memory_pool_dll_include.h"
#include "D:/ExternalCustomAPIs/OBJLoader/code/obj_parser_dll_include.h"
#include "L:/code/game_layer_math.h"


enum voxel_type
{
    vt_empty = 0,
    vt_solid = 1,
};

struct bounding_box
{
    i32 vertexNum, indicesNum;


    r32* verts; //24
    u16* indices; //36x
};

struct voxel_face_info
{
    voxel_type voxelType;
    bool32 renderWholeVoxel;
    bool32 renderedFaces[6];
    
};

struct voxel
{
    v3 pos;
    v3 gridPos;
    voxel_face_info* voxelFaceInfo;
    bool32 isSolid;
    i32 voxelIndex;

    u16 indices[36]; 
    
    v3 vertColors;

    voxel_type voxelType;
    bool32 renderedFaces[6];
    i32 numOfRenderedFaces;    

    i32 indexCount;
    i32 renderedIndiceCount;
    u16 renderedIndices[36];    
};

struct voxel_chunk
{
    r32 length, width, height, voxelSize;
    r32 voxelResolution;
    v3 voxelChunkExtent;

    v3 maxCorner, minCorner;

    i32* renderedVoxelIndex;        
    voxel* voxels;

    i32 numOfRenderedVoxels;


    r32 verts[24];
    i32 voxelVertCount;

    v3 vertexColors[8];
    
//Instanced chunk stuff pending removal    
    v3* voxelPositions;
    voxel_face_info* voxelFaceInfo;

    i32 renderedVoxelCount;
    v3* renderedVoxelPositions;


//    
};

struct game_initialize_data
{
    obj* allObjs;
    voxel_chunk chunk;
};

struct instance_data
{
    v3 pos;
};

struct game_button_state
{
    i32 halfTransitionCount;
    bool32 endedDown;
    bool32 wasDown;
    bool32 started;
    bool32 held;

    i32 heldTime;
};

struct game_controller_input
{
    bool32 isAnalog;
    bool32 isConnected;

    bool32 started;
    bool32 inputPreviousFrame;

    union
    {
	game_button_state buttons[7];
	struct
	{
	    game_button_state moveForward;
	    game_button_state moveBackward;
	    game_button_state moveRight;
	    game_button_state moveLeft;
	    game_button_state moveUp;
	    game_button_state moveDown;

	    game_button_state terminator;
	};
    };
};

struct game_input
{
    game_button_state mouseButtons[5];
    i32 mouseX, mouseY, mouseZ;

    r32 dTime;
    game_controller_input controllers[5];
};

inline game_controller_input* GetController(game_input* input, u32 controllerIndex)
{
    game_controller_input* result = &input->controllers[controllerIndex];
    return(result);
}


#define GAME_UPDATE(name) void name(program_memory* memory, game_input* input)
typedef GAME_UPDATE(game_update);

#define GAME_INITIALIZE(name) game_initialize_data name(memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena, program_memory* mainProgramMemory, i32* numOfGameObjects, parse_obj_data_code* parseOBJCode)
typedef GAME_INITIALIZE(game_initialize);

#define GAME_LAYER_H
#endif
