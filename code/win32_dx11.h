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

    r32 arcBallRadius;
    r32 fovY;
    DirectX::XMMATRIX rotation;
    DirectX::XMVECTOR pivot;

    DirectX::XMVECTOR currRotation;
    DirectX::XMVECTOR lastRotation;
    r32 distance;

    r32 targetZoom, currZoom, lag, zoomingTo;
    DirectX::XMMATRIX viewInverted;
    
    DirectX::XMVECTOR currQRot;
    DirectX::XMVECTOR targetPos;

    DirectX::XMVECTOR targetQRot;

    

    DirectX::XMVECTOR positionTo;


    DirectX::XMVECTOR qRotationTo;

    DirectX::XMVECTOR viewCenter;
    DirectX::XMVECTOR eye;
    DirectX::XMVECTOR upDir;
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
