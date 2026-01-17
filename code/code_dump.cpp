
internal void
UpdateCameraArc(dx_camera* camera)
{
    DirectX::XMVECTOR currRotationQ = GetCurrentArcRotation(camera);

    DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationQuaternion(currRotationQ);

    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.view,
	rotMatrix);
}


internal DirectX::XMVECTOR GetCurrentArcRotation(dx_camera* camera)
{
    DirectX::XMVECTOR result = DirectX::XMVectorMultiply(camera->currentRotation, camera->lastRotation);
    return(result);
}


internal DirectX::XMVECTOR
ComputeScreenZCoord(r32 x, r32 y)
{
    r32 z = 0.0f;
    r32 xSq = (r32)pow(x, 2);
    r32 ySq = (r32)pow(y, 2);
    if ((xSq + ySq) <= 1)
    {
//	z = (r32)sqrt((1 - pow(xSq, 2) - pow(ySq, 2)));
	z = (r32)sqrt((1 - xSq - ySq));	
    }
    else
    {
	z = 0;
    }
    DirectX::XMVECTOR result = DirectX::XMVectorSet(x, y, z, 1.0f);
    return(result);
}

internal void
ProcessMouseButtonPress(game_input* input, mouse_movements* mouse)
{
    if (input->mouseButtons[0].started)
    {
	mouse->mouseArcStart = ComputeScreenZCoord(mouse->x, mouse->y);
    }
}


internal r32
CalculateXMVectorMagnitude(DirectX::XMVECTOR vec)
{
    r32 result = 0.f;
    r32 x, y, z;
    x = DirectX::XMVectorGetX(vec);
    y = DirectX::XMVectorGetY(vec);
    z = DirectX::XMVectorGetZ(vec);

    result = (r32)sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    return(result);
}

internal DirectX::XMMATRIX
ComputeRotationMatrix(DirectX::XMVECTOR curr, DirectX::XMVECTOR start)
{
    DirectX::XMVECTOR u = DirectX::XMVector3Cross(curr, start);
    u = DirectX::XMVector3Normalize(u);
    DirectX::XMVECTOR dxDot = DirectX::XMVector3Dot(curr, start);
    r32 dot = DirectX::XMVectorGetX(dxDot);

    r32 currMag = CalculateXMVectorMagnitude(curr);
    r32 startMag = CalculateXMVectorMagnitude(start);    

    
    
    r32 theta = (r32)acos((min(1, dot) / (currMag * startMag)));

    DirectX::XMMATRIX result = DirectX::XMMatrixRotationAxis(u, theta);
    return(result);
}

internal DirectX::XMVECTOR
ComputeRotationQuaternion(DirectX::XMVECTOR curr, DirectX::XMVECTOR start)
{
    //Compute unit rotation axis and rotation angle based on passed points and get the representing quaternion
    DirectX::XMVECTOR u = DirectX::XMVector3Cross(curr, start);
    u = DirectX::XMVector3Normalize(u);
    DirectX::XMVECTOR dxDot = DirectX::XMVector3Dot(curr, start);
    r32 dot = DirectX::XMVectorGetX(dxDot);
    r32 theta = (r32)acos(min(1, dot));

    r32 q_s = (r32)cos(theta / 2);


    //I imagine this could be optmized a little better using DirectX functions for SIMD
    r32 s, x, y, z;
    r32 sT2 = (r32)sin(theta / 2);
    s = q_s; 
    x = DirectX::XMVectorGetX(u) * sT2;
    y = DirectX::XMVectorGetY(u) * sT2;
    z = DirectX::XMVectorGetZ(u) * sT2;
    
    DirectX::XMVECTOR result = DirectX::XMVectorSet(s, x, y, z);
    return(result);
}

internal void
ProcessMouseControlArc(game_input* input, mouse_movements* mouse, dx_camera* camera)
{

    mouse->mouseArcCurrent = ComputeScreenZCoord(mouse->x, mouse->y);

    //Then create rotation information
    DirectX::XMMATRIX rotationMat = ComputeRotationMatrix(mouse->mouseArcCurrent, mouse->mouseArcStart);
//    camera->currentRotation = ComputeRotationQuaternion(mouse->mouseArcCurrent, mouse->mouseArcStart);
    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.view,
	DirectX::XMMatrixTranspose(rotationMat)
	);
	
}
