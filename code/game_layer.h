#if !defined (GAME_LAYER_H)

#include "D:/ExternalCustomAPIs/Types/direct_x_typedefs.h"
#include "D:/ExternalCustomAPIs/MemoryPools/code/memory_pool_dll_include.h"
#include "D:/ExternalCustomAPIs/OBJLoader/code/obj_parser_dll_include.h"




struct v3
{
    r32 x, y, z;
};

struct bounding_box
{
    i32 vertexNum, indicesNum;


    r32* verts; //24
    u16* indices; //36x
};

struct voxel_chunk
{
    r32 length, width, height, voxelSize;
    r32 voxelResolution;
    v3 voxelChunkExtent;
};

struct game_initialize_data
{
    obj* allObjs;
    voxel_chunk voxels;
};

/*
  Rather than storing the instances' location via struct in the cpu, we can use the index
  of the cube in the array as the value according to the location in voxel space and run our calculations
  for determining the location of a cube in the world when calculating the position. Could this be done in
  a specific shader? So we store the id based on the location in the array? Is this something that is already stored?

  Checkout SV_InstanceID?
  Is it something that is worth calculating every frame?
   - Probably not?
 */


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
