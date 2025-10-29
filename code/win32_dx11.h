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
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{
    char exeFilename[WIN32_STATE_FILE_NAME_COUNT];
    char* onePastExeFilenameSlash;
};

struct win32_game_code
{
    HMODULE gameCodeDLL;
    FILETIME dllLastWriteTime;


    game_update* GameUpdate;
    game_initialize* GameInitialize;

    bool32 isValid;
};

#define WIN32_DX11_H
#endif
