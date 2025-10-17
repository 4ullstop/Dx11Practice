#if !defined WIN32_DX11_H

#include "D:/ExternalCustomAPIs/Types/direct_x_typedefs.h"

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


#define WIN32_DX11_H
#endif
