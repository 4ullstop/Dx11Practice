#if !defined (GAME_LAYER_H)

#include "D:/ExternalCustomAPIs/Types/direct_x_typedefs.h"
#include "D:/ExternalCustomAPIs/MemoryPools/code/memory_pool_dll_include.h"
#include "D:/ExternalCustomAPIs/OBJLoader/code/obj_parser_dll_include.h"

//#include "L:/code/game_layer_math.h"
#include "D:/ExternalCustomAPIs/Math/forty_math.h"

#define screenW 1280
#define screenH 720

enum voxel_type
{
    vt_empty = 0,
    vt_solid = 1,
};

struct mouse_movements
{
    v2 arcBallStart;
    v2 arcBallCurrent;

    v2 arcBallPrevNDC;

    bool32 recMousePos;
    r32 x, y;    
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
    v3 chunkWorldLocation, chunkWorldRotation, chunkWorldScale;
	

    r32 length, width, height, voxelSize;
    r32 voxelResolution;
    v3 voxelChunkExtent;

    v3 maxCorner, minCorner;

    i32* renderedVoxelIndex;        
    voxel* voxels;

    i32 numOfRenderedVoxels;


    union
    {
	struct
	{
	    r32 e0, e1, e2,
		e3, e4, e5,
		e6, e7, e8,
		e9, e10, e11,
		e12, e13, e14,
		e15, e16, e17, 
		e18, e19, e20,
		e21, e22, e23;
		
	};
	r32 verts[24];
	v3 vecVerts[8];
    };

    i32 voxelVertCount;

    v3 vertexColors[8];
    
//Instanced chunk stuff pending removal    
    v3* voxelPositions;
    voxel_face_info* voxelFaceInfo;

    i32 renderedVoxelCount;
    v3* renderedVoxelPositions;

    v3 centoid;
//    
};

struct game_debug_vector
{
    v3 start, end, color;
};

struct game_camera
{
    v3 pos, forward;

    m4 proj, viewInverted;
};

struct game_state
{
    game_camera gameCamera;
    r32 debugVectorLength;

    listed_memory* debugVectorMemory;
    listed_memory_node* debugVectorNodes;
    i32 numOfDrawnVectors;

    i32 windowW, windowH;
};

struct game_initialize_data
{
    obj* allObjs;
    voxel_chunk chunk;
    game_state* gameState;
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
    bool32 released;
    
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
	game_button_state buttons[8];
	struct
	{
	    game_button_state moveForward;
	    game_button_state moveBackward;
	    game_button_state moveRight;
	    game_button_state moveLeft;
	    game_button_state moveUp;
	    game_button_state moveDown;
	    
	    game_button_state testKey;

	    game_button_state terminator;
	};
    };
};

enum e_mouse_buttons
{
    middle_mouse,
    left_mouse,
    right_mouse,
    middle_mouse_up,
    middle_mouse_down,
};

struct game_input
{
    game_button_state mouseButtons[5];
    i32 mouseXUnbounded, mouseYUnbounded, mouseZUnbounded;

    i32 mouseXBounded, mouseYBounded, mouseZBounded;

    bool32 mouseButtonIsDown, mouseButtonReleased;

    r32 mouseWheelDelta;
    
    v2 arcBallStart;
    v2 arcBallCurrent;

    
    r32 dTime;
    game_controller_input controllers[5];
};

struct ray_cast
{
    bool32 hit;
    v3 pClose, pFar;
    r32 tMin;
};

struct voxel_cast
{
    ray_cast ray;
    voxel* hitVoxel;
};

inline game_controller_input* GetController(game_input* input, u32 controllerIndex)
{
    game_controller_input* result = &input->controllers[controllerIndex];
    return(result);
}

internal v2
GetMouseScreenCoords(game_input* input)
{
    v2 result = {};
    result.x = (r32)input->mouseXBounded;
    result.y = (r32)input->mouseYBounded;
    return(result);
}

internal v2
ScreenToCoordNDC(v2 loc)
{
    v2 windowSize = v2{screenW, screenH};
    v2 one = v2{1.0f, 1.0f};
    
    v2 result = loc * 2.0f / windowSize - one;
    return(result);
}

#define GAME_UPDATE(name) void name(program_memory* memory, game_input* input, game_state* gameState, memory_pool_dll_code* memoryPoolCode, memory_arena* debugVectorArena, voxel_chunk* chunk)
typedef GAME_UPDATE(game_update);

#define GAME_INITIALIZE(name) game_initialize_data name(memory_pool_dll_code* memoryPoolCode, memory_arena* objLocationArena, program_memory* mainProgramMemory, i32* numOfGameObjects, parse_obj_data_code* parseOBJCode, memory_arena* debugVectorArena)
typedef GAME_INITIALIZE(game_initialize);

#define GAME_LAYER_H
#endif
