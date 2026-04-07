#if !defined WIN32_DX11_H

#include "D:/ExternalCustomAPIs/Types/direct_x_typedefs.h"
#include "game_layer.h"

struct dx_camera
{
    constant_buffer_struct constantBufferData;
    
    DirectX::XMVECTOR position;
    DirectX::XMVECTOR right;
    DirectX::XMVECTOR worldUp;
    DirectX::XMVECTOR up;
    DirectX::XMVECTOR front;
    r32 yaw, pitch, movementSpeed, turnSpeed;

    
    DirectX::XMVECTOR startEye;
    DirectX::XMVECTOR startAt;
    DirectX::XMVECTOR startUp;

    r32 fovY;

    r32 targetZoom, currZoom, lag, zoomingTo;
    DirectX::XMMATRIX viewInverted;
    DirectX::XMVECTOR targetPos;
    DirectX::XMVECTOR positionTo;
    
    DirectX::XMVECTOR targetQRot;
    DirectX::XMVECTOR qRotationTo;
    DirectX::XMVECTOR currQRot;
    
    DirectX::XMVECTOR viewCenter;
    DirectX::XMVECTOR eye;
    DirectX::XMVECTOR upDir;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{
    char exeFilename[WIN32_STATE_FILE_NAME_COUNT];
    char* onePastExeFilenameSlash;

    ID3D11Buffer* worldObjectConstants;
};

struct win32_game_code
{
    HMODULE gameCodeDLL;
    FILETIME dllLastWriteTime;

    
    game_update* GameUpdate;
    game_initialize* GameInitialize;

    bool32 isValid;
};

struct program_state
{
    memory_arena setupArena;
    memory_arena perFrameArena;

    memory_arena debugVectorArena;

    
    u8* arenaBase;
};

struct direct_x_loaded_buffers
{
    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;
    ID3D11Buffer* vertInstanceBuffer;
    i32 indexCount;
    i32 instanceCount;
};

struct object_constants
{
    DirectX::XMFLOAT4 worldPos;
};

struct voxel_chunk_world_constant
{
    DirectX::XMFLOAT4X4 world;
};

        
struct r32_3
{
    r32 x;
    r32 y;
    r32 z;    
};

struct shaders
{
    ID3D11VertexShader* voxelVertexShader;
    ID3D11VertexShader* debugVertexShader;
    ID3D11InputLayout* debugInputLayout;
    
    ID3D11InputLayout* voxelInputLayout;
    ID3D11PixelShader* pixelShader;
    ID3D11Buffer* voxelConstantBuffer;
    ID3D11Buffer* debugConstantBuffer;
    
    ID3D11Buffer* instanceBuffer;
};

struct dx_instance_data
{
    DirectX::XMFLOAT3 pos;
};

struct obj_conversion
{
    vertex_position_color* objVerts;
    u16* indices;
    u32 objVertsSize;
    u32 indexCount;
};

struct aspect_ratio
{
    r32 aspectX, aspectY;
};


struct win32_voxel_chunk
{
    voxel_chunk* chunk;
    ID3D11Buffer** indexBuffers;
    ID3D11Buffer* vertexBuffers;

    //I imagine this isn't the best way to store this so you'll prob be back here later
    r32** drawnVoxelIndices;

    //This is a direct correlation to the vertex and index buffers, avoiding Dx11 implementation in game layer
    DirectX::XMFLOAT4* drawnVoxelPositions; //

    vertex_position_color* vsInput; //XMFloat3 types


    ID3D11Buffer* voxelChunkWorldCB;
};

struct win32_debug_vector
{
    DirectX::XMVECTOR start;
    DirectX::XMVECTOR end;
    DirectX::XMVECTOR color;
};

struct win32_debug_vectors
{

    
    ID3D11Buffer* indexBuffer;
    ID3D11Buffer* vertexBuffer;

//future me could you make this any smaller?
    i32 numOfDebugIndices;
    u16 debugVectorIndices[2];

    vertex_position_color* vsInput;

    r32 debugVectorLength;
    i32 currDrawnVectors;
};

struct shader_code
{
    ID3DBlob* vertexShaderCode;
    ID3DBlob* pixelShaderCode;    
};

void* MemCpy(void* dest, void* src, size_t n)
{
    if (dest == nullptr) return(nullptr);
    u8* uDest = (u8*)dest;
    u8* uSrc = (u8*)src;

    for (i32 i = 0; i < n; i++)
    {
	uDest[i] = uSrc[i];
    }

    return(dest);
}

#define WIN32_DX11_H
#endif
