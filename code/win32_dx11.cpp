#define DIRECTXLOAD 0
#include <windows.h>
#include <stdio.h>
#include <math.h>

#include <DirectXMath.h>


#include "D:/ExternalCustomAPIs/MemoryPools/code/memory_pool_dll_include.h"

//If you separate the program out in the future the file_reader.h file can be included in the separated dll file
//and the file_reader.cpp should stay here in our win32 implementation

#include "D:/ExternalCustomAPIs/FileReader/file_reader.h"
#include "D:/ExternalCustomAPIs/FileReader/file_reader.cpp"

#if DIRECTXLOAD
#include "D:/ExternalCustomAPIs/OBJLoader/code/directx_obj_loader_dll_include.h"
#else
#include "D:/ExternalCustomAPIs/OBJLoader/code/obj_parser_dll_include.h"
#endif

#include "D:/ExternalCustomAPIs/Types/typedefs.h"
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include "win32_dx11.h"


#include <xinput.h>
#include <d3dcompiler.h>

#include "game_intrinsics.h"

#define PI 3.14159265


global_variable memory_pool_dll_code memoryPoolCode;
#if DIRECTXLOAD
global_variable direct_x_load_obj_code directXOBJCode;
#else
global_variable parse_obj_data_code parseObjCode;
#endif
global_variable bool32 running;
global_variable program_state* programState;
global_variable ID3D11Texture2D* backBuffer;
global_variable u32 frameCount;
global_variable i64 perfCountFrequency;
global_variable constant_buffer_struct constantBufferData;

global_variable bool32 freeCam;

#include "D:/ExternalCustomAPIs/Math/forty_directx_math.h"
#include "game_layer.h"


global_variable ID3D11DeviceContext* context;
global_variable ID3D11Device* d3dDevice;
global_variable WINDOWPLACEMENT windowPosition = {sizeof(windowPosition)};
global_variable IDXGISwapChain1* swapChain;
global_variable ID3D11RenderTargetView* renderTarget;
global_variable ID3D11DepthStencilView* depthStencilView;
global_variable ID3D11Texture2D* depthStencil;
global_variable D3D11_TEXTURE2D_DESC bbDesc;


#if 0
internal DirectX::XMVECTOR
FromV3ToXMVECTOR(v3 vec)
{
    DirectX::XMVECTOR result = DirectX::XMVectorSet(vec.x, vec.y, vec.z, 0.0f);
    return(result);
}

internal v3
FromXMVECTORToV3(DirectX::XMVECTOR vec)
{
    v3 result = {};
    result.x = DirectX::XMVectorGetX(vec);
    result.y = DirectX::XMVectorGetY(vec);
    result.z = DirectX::XMVectorGetZ(vec);

    return(result);
}
#endif
internal void
ConfigureBackBuffer(void)
{
    HRESULT hr = {};
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    hr = d3dDevice->CreateRenderTargetView(backBuffer,
				      nullptr,
				      &renderTarget);

    backBuffer->GetDesc(&bbDesc);

    CD3D11_TEXTURE2D_DESC depthStencilDesc(
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	(UINT)bbDesc.Width,
	(UINT)bbDesc.Height,
	1, //The depth stencil view has only one texture
	1, //Use a single mipmap levelx
	D3D11_BIND_DEPTH_STENCIL
	);

    hr = d3dDevice->CreateTexture2D(
	&depthStencilDesc,
	nullptr,
	&depthStencil);

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);

    hr = d3dDevice->CreateDepthStencilView(depthStencil,
				      &depthStencilViewDesc,
				      &depthStencilView);
    D3D11_VIEWPORT viewport = {};
    viewport.Height = (r32)bbDesc.Height;
    viewport.Width = (r32)bbDesc.Width;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;

    context->RSSetViewports(
	1,
	&viewport);
    
}

internal void
ToggleFullscreen(HWND hwnd)
{
    HRESULT hr = 0;


    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (dwStyle & WS_OVERLAPPEDWINDOW)
    {
	MONITORINFO mi = {sizeof(mi)};

	if (GetWindowPlacement(hwnd, &windowPosition) &&
	    GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY),
			   &mi))
	{
	    hr = swapChain->SetFullscreenState(TRUE, NULL);

	    context->ClearState();
	
	    //Release buffers
	    renderTarget->Release();
	    backBuffer->Release();
	    depthStencilView->Release();
	    depthStencil = 0;

	    hr = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	    ConfigureBackBuffer();
	    
	}
    }
    else
    {
	hr = swapChain->SetFullscreenState(FALSE, NULL);


	
#if 0	
	SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPlacement(hwnd, &windowPosition);
	SetWindowPos(hwnd, 0, 0, 0, 0, 0,
		     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
#endif	
    }


}

internal void
Win32UnloadGameCode(win32_game_code* gameCode)
{
    if (gameCode->gameCodeDLL)
    {
	FreeLibrary(gameCode->gameCodeDLL);
	gameCode->gameCodeDLL = 0;
    }
    gameCode->isValid = false;
    gameCode->GameUpdate = 0;
    gameCode->GameInitialize = 0;
}

inline FILETIME
Win32GetLastWriteTime(char* filename)
{
    FILETIME lastWriteTime = {};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
    {
	lastWriteTime = data.ftLastWriteTime;
    }
    return(lastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char* sourceDLLName, char* tempDLLName, char* lockFilename)
{
    win32_game_code result = {};
    WIN32_FILE_ATTRIBUTE_DATA ignored;
    DWORD lastError = 0;

#if 0
    GetFileAttributesEx(lockFilename, GetFileExInfoStandard, &ignored);
    lastError = GetLastError();    
#else    
    
    if (!GetFileAttributesEx(lockFilename, GetFileExInfoStandard, &ignored))
    {
	result.dllLastWriteTime = Win32GetLastWriteTime(sourceDLLName);
	CopyFile(sourceDLLName, tempDLLName, FALSE);
	result.gameCodeDLL = LoadLibrary(tempDLLName);
	if (result.gameCodeDLL)
	{
	    result.GameUpdate = (game_update*)GetProcAddress(result.gameCodeDLL, "GameUpdate");

	    
	    result.GameInitialize = (game_initialize*)GetProcAddress(result.gameCodeDLL, "GameInitialize");
	    result.isValid = (result.GameUpdate && result.GameInitialize);
	}
    }
#endif    

    if (!result.isValid)
    {
	result.GameUpdate = 0;
	result.GameInitialize = 0;
    }
    return(result);
}
        
internal void
Win32GetEXEFilename(win32_state* state)
{
    DWORD sizeOfFilename = GetModuleFileName(0, state->exeFilename, sizeof(state->exeFilename));
    state->onePastExeFilenameSlash = state->exeFilename;
    for (char* scan = state->exeFilename; *scan; ++scan)
    {
	if (*scan == '\\')
	{
	    state->onePastExeFilenameSlash = scan + 1;
	}
    }
}

internal void
CatStrings(size_t sourceACount, char* sourceA,
	   size_t sourceBCount, char* sourceB,
	   size_t destCount, char* dest)
{
    for (i32 index = 0; index < sourceACount; ++index)
    {
	*dest++ = *sourceA++;
    }
    for (i32 index = 0; index < sourceBCount; ++index)
    {
	*dest++ = *sourceB++;
    }
    *dest++ = 0;
}

internal i32
StringLength(char* string)
{
    i32 count = 0;
    while (*string++)
    {
	++count;
    }
    return(count);
}

internal void
Win32BuildExePathFilename(win32_state* state, char* filename, i32 destCount, char* dest)
{
    CatStrings(state->onePastExeFilenameSlash - state->exeFilename, state->exeFilename,
	       StringLength(filename), filename,
	       destCount, dest);
}

internal void
CreateInstanceBuffer(direct_x_loaded_buffers* loadedBuffers, shaders* shaderResources, voxel_chunk* voxelChunk, memory_arena* arena)
{
    //Assumes we've called CreateVoxelChunk from our game code
    //Converting all of voxel space in to world space for the instand_data buffer

    HRESULT hr = {};
    DirectX::XMVECTOR boundsExtent = DirectX::XMVectorSet(voxelChunk->voxelChunkExtent.x,
							  voxelChunk->voxelChunkExtent.y,
							  voxelChunk->voxelChunkExtent.z,
							  0.0f);

    inst_buffer_struct* instBuffer = (inst_buffer_struct*)memoryPoolCode.PushArraySized(arena, (size_t)(sizeof(inst_buffer_struct) * voxelChunk->voxelResolution));

    DirectX::XMVECTOR tempPos;

    for (int i = 0; i < voxelChunk->voxelResolution; i++)
    {

	tempPos = DirectX::XMVectorSet(voxelChunk->voxelPositions[i].x,
				       voxelChunk->voxelPositions[i].y,
				       voxelChunk->voxelPositions[i].z,
				       1.0f);

	XMStoreFloat4(&instBuffer[i].instancePosition, tempPos);

    }

    //Step through this and see what happens
    D3D11_BUFFER_DESC instBufferDesc;
    ZeroMemory(&instBufferDesc, sizeof(instBufferDesc));

    instBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    instBufferDesc.ByteWidth = (UINT)(sizeof(inst_buffer_struct) * voxelChunk->voxelResolution);
    instBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instBufferDesc.CPUAccessFlags = 0;
    instBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA instData;
    ZeroMemory(&instData, sizeof(instData));
    instData.pSysMem = &instBuffer[0];

    hr = d3dDevice->CreateBuffer(&instBufferDesc, &instData, &shaderResources->instanceBuffer);


}

internal obj_conversion
ConvertGameOBJToDXOBJ(obj* currObj, memory_arena* arena)
{
    obj_conversion result;
    HRESULT hr = {};

    result.objVertsSize = sizeof(vertex_position_color) * (currObj->vertexCount);

    result.objVerts =
	(vertex_position_color*)memoryPoolCode.PushArraySized(arena, result.objVertsSize);

    size_t indexSize = sizeof(u16) * currObj->faceLastIndex;
    result.indices = (u16*)memoryPoolCode.PushArraySized(arena, indexSize);

    r32_3 testColors[] =
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



    for (i32 i = 0, j = 0; j < currObj->vertexCount; i += 3, j++)
    {
	result.objVerts[j].pos.x = currObj->vertices[i];
	result.objVerts[j].pos.y = currObj->vertices[i + 1];
	result.objVerts[j].pos.z = currObj->vertices[i + 2];

#if 0
	DirectX::XMFLOAT3 vertColor = {1.0f, 1.0f, 1.0f};

#else
	DirectX::XMFLOAT3 vertColor = {testColors[j].x, testColors[j].y, testColors[j].z};
#endif	

	result.objVerts[j].color = vertColor;

    }

    for (i32 i = 0; i < currObj->faceLastIndex; i++)
    {
	result.indices[i] = currObj->vertexIndices[i] - 1;
    }

    result.indexCount = currObj->faceLastIndex;
    return(result);
}

internal void
LoadInstancedOBJ(obj* allInstancedOBJs, memory_arena* objLocationArena, direct_x_loaded_buffers* loadedBuffers, voxel_chunk* voxelChunk, shaders* shaderResources)
{
    HRESULT hr = {};

    obj_conversion convertedObj = ConvertGameOBJToDXOBJ(allInstancedOBJs, objLocationArena);

    CreateInstanceBuffer(loadedBuffers, shaderResources, voxelChunk, objLocationArena);
    loadedBuffers->indexCount = convertedObj.indexCount;
    loadedBuffers->instanceCount = (i32)voxelChunk->voxelResolution;

    D3D11_BUFFER_DESC vertexDesc;
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.ByteWidth = convertedObj.objVertsSize;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = 0;
    vertexDesc.MiscFlags = 0;
    

    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));
    vertexData.pSysMem =  convertedObj.objVerts;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    hr =  d3dDevice->CreateBuffer(
	&vertexDesc,
	&vertexData,
	&loadedBuffers->vertexBuffer);

    CD3D11_BUFFER_DESC indexDesc(
	sizeof(u16) * convertedObj.indexCount,
	D3D11_BIND_INDEX_BUFFER);

    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
    indexData.pSysMem = convertedObj.indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    hr = d3dDevice->CreateBuffer(
	&indexDesc,
	&indexData,
	&loadedBuffers->indexBuffer);
}

internal void
LoadAllOBJs(obj* allOBJs, i32 numOfGameObjects, direct_x_loaded_buffers* loadedBuffers, memory_arena* objLocationArena)
{
    HRESULT hr = {};    
#if 0
//At some point you will loop all of these depending on the loaded objects in the game, but rn you only need to load
    //one so I'm loading one

    u32 objVertsSize = sizeof(vertex_position_color) * (allOBJs->vertexCount);

    vertex_position_color* objVerts =
	(vertex_position_color*)memoryPoolCode.PushArraySized(objLocationArena, objVertsSize);


    r32_3 testColors[] =
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



    for (i32 i = 0, j = 0; j < allOBJs->vertexCount; i += 3, j++)
    {
	objVerts[j].pos.x = allOBJs->vertices[i];
	objVerts[j].pos.y = allOBJs->vertices[i + 1];
	objVerts[j].pos.z = allOBJs->vertices[i + 2];

#if 0
	DirectX::XMFLOAT3 vertColor = {1.0f, 1.0f, 1.0f};

#else
	DirectX::XMFLOAT3 vertColor = {testColors[j].x, testColors[j].y, testColors[j].z};
#endif	

	objVerts[j].color = vertColor;

    }

    for (i32 i = 0; i < allOBJs->faceLastIndex; i++)
    {
	allOBJs->vertexIndices[i]--;
    }
#else

    //If something doesn't work here it's bc the memory has a lifetime in this function
    obj_conversion convertedObj = ConvertGameOBJToDXOBJ(allOBJs, objLocationArena);
    
#endif
    
    CD3D11_BUFFER_DESC vertexDesc(
	convertedObj.objVertsSize,
	D3D11_BIND_VERTEX_BUFFER);

    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));
    vertexData.pSysMem = convertedObj.objVerts;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    hr = d3dDevice->CreateBuffer(
	&vertexDesc,
	&vertexData,
	&loadedBuffers->vertexBuffer);

    CD3D11_BUFFER_DESC indexDesc(
	sizeof(u16) * allOBJs->faceLastIndex,
	D3D11_BIND_INDEX_BUFFER);

    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
    indexData.pSysMem = allOBJs->vertexIndices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    hr = d3dDevice->CreateBuffer(
	&indexDesc,
	&indexData,
	&loadedBuffers->indexBuffer);

    loadedBuffers->indexCount = allOBJs->faceLastIndex;
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    r32 result = ((r32)(end.QuadPart - start.QuadPart) / (r32)perfCountFrequency);
    return(result);
}

internal r32
GetAspectRatio(void)
{

    backBuffer->GetDesc(&bbDesc);
    r32 result = (r32)bbDesc.Width / (r32)bbDesc.Height;
    return(result);
}

internal void
InitCameraDefaultValues(dx_camera* camera)
{
    camera->startEye = DirectX::XMVectorSet(0.0f, 0.7f, 1.5f, 0.f);
    camera->startAt = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    camera->startUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    
    camera->up = camera->startUp;
    camera->worldUp = camera->startUp;
    camera->yaw = -90.0f;
    camera->pitch = 0.0f;
    camera->front = {0.0f, 0.0f, -1.0f, 0.0f};
    camera->movementSpeed = 5.0f;
    camera->turnSpeed = 0.2f;
    camera->position = {10.0f, 10.0f, 10.0f};
}

internal aspect_ratio
GetGameAspectRatio(void)
{
    aspect_ratio result = {};
    result.aspectX = GetAspectRatio();
    result.aspectY = result.aspectX < (16.0f / 9.0f) ? result.aspectX / (16.0f / 9.0f) : 1.0f;
    return(result);
}

internal void
CreateViewAndPerspective(dx_camera* camera)
{
    InitCameraDefaultValues(camera);
    aspect_ratio aspect = GetGameAspectRatio();


    DirectX::XMMATRIX startingViewMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtRH(camera->startEye,
												camera->startAt,
												camera->startUp));

    DirectX::XMStoreFloat4x4(&camera->constantBufferData.view, startingViewMatrix);


    camera->fovY = 2.0f * (r32)(atan(tan(DirectX::XMConvertToRadians(70) * 0.5f)) / aspect.aspectY);
    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.projection,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixPerspectiveFovRH(
		camera->fovY,
		aspect.aspectX,
		0.01f,
		1000.0f)
	    )
	);
}

internal void
CreateWindowSizeDependentResources(dx_camera* camera)
{
    CreateViewAndPerspective(camera);
}

internal void
UpdateInternalTransformations(dx_camera* camera)
{

    DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(camera->currQRot);
    DirectX::XMMATRIX trans = DirectX::XMMatrixTranslationFromVector(camera->position);
    
    DirectX::XMMATRIX zoom = DirectX::XMMatrixTranslation(0.0f, 0.0f, camera->currZoom);

    DirectX::XMMATRIX cameraWorld = zoom * rot * trans;

    DirectX::XMVECTOR det;
    DirectX::XMMATRIX view = DirectX::XMMatrixInverse(&det, cameraWorld);

    
    DirectX::XMStoreFloat4x4(&camera->constantBufferData.view, DirectX::XMMatrixTranspose(view));
    camera->viewInverted = cameraWorld;    
}

internal DirectX::XMVECTOR
NDCToArcBall(v2 loc)
{
    r32 dist = Dot(loc, loc);

    DirectX::XMVECTOR result = {};
    if (dist <= 1.0f)
    {
	result = DirectX::XMVectorSet(loc.x, -loc.y, (r32)sqrt(1.0f - dist), 0.0f);
    }
    else
    {
	DirectX::XMVECTOR locVec = DirectX::XMVectorSet(loc.x, -loc.y, 0.0f, 0.0f);
	result = DirectX::XMVector3Normalize(locVec);
	DirectX::XMVectorSetZ(result, 0.0f);
	DirectX::XMVectorSetW(result, 0.0f);
    }
    return(result);    
}

internal void
RotateArcBallCam(dx_camera* camera, mouse_movements* mouse)
{
    v2 pointerPosNDC = ScreenToCoordNDC(mouse->arcBallCurrent);
    DirectX::XMVECTOR currQRot = NDCToArcBall(pointerPosNDC);

    DirectX::XMVECTOR prevQRot = NDCToArcBall(mouse->arcBallPrevNDC);
    mouse->arcBallPrevNDC = pointerPosNDC;

    r32 dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(currQRot, prevQRot));
    if (dot < 0.9999f)
    {
	DirectX::XMVECTOR axis = DirectX::XMVector3Cross(currQRot, prevQRot);
	r32 angle = acosf(dot);

	DirectX::XMVECTOR deltaQ = DirectX::XMQuaternionRotationAxis(axis, angle);
	camera->targetQRot = DirectX::XMQuaternionMultiply(deltaQ, camera->targetQRot);
    }
}

internal void
TranslateDelta(v2 ndc, dx_camera* camera)
{
    r32 hh = (r32)(fabs(camera->targetZoom) * tanf(camera->fovY * 0.5f));
    r32 hw = hh * GetAspectRatio();

    DirectX::XMVECTOR translation = DirectX::XMVectorSet(ndc.x * hw, ndc.y * hh, 0.0f, 0.0f);

    camera->targetPos = DirectX::XMVectorAdd(DirectX::XMVector3Transform(translation, camera->viewInverted),
					     camera->targetPos);
}

internal void
TranslateArcBall(dx_camera* camera, mouse_movements* mouse)
{
    v2 mousePosNDC = ScreenToCoordNDC(mouse->arcBallCurrent);
    v2 translationNDC = mousePosNDC - mouse->arcBallPrevNDC;
    mouse->arcBallPrevNDC = mousePosNDC;
    TranslateDelta(translationNDC, camera);
}

internal void
InitTransformation(mouse_movements* mouse)
{
    mouse->arcBallPrevNDC = ScreenToCoordNDC(mouse->arcBallStart);
}

internal bool32
UpdateArcBallTransformation(dx_camera* camera)
{
    DirectX::XMVECTOR diffViewCenter = DirectX::XMVectorSubtract(camera->targetPos, camera->position);
    DirectX::XMVECTOR diffRotation  = DirectX::XMVectorSubtract(camera->targetQRot, camera->currQRot);
    r32 diffZoom = camera->targetZoom - camera->currZoom;

    r32 dViewCenter = DirectX::XMVectorGetX(DirectX::XMVector3Dot(diffViewCenter, diffViewCenter));
    r32 dRotation = DirectX::XMVectorGetX(DirectX::XMQuaternionDot(diffRotation, diffRotation));
    r32 dZooming = diffZoom * diffZoom;

    //Nothing changed
    if ((dViewCenter < 1.0e-10f) &&
	(dRotation < 1.0e-10f) &&
	(dZooming < 1.0e-10f))
    {
	return false;
    }

    if ((dViewCenter < 1.0e-6f) &&
	(dRotation < 1.0e-6f) &&
	(dZooming < 1.0e-6f))
    {

	camera->position = camera->targetPos;
	camera->currQRot = camera->targetQRot;
	camera->currZoom = camera->targetZoom;
    }
    else
    {
	//Interpolation for that smootttthhhneesss
	r32 t = 1 - camera->lag;
	camera->position = DirectX::XMVectorLerp(camera->position, camera->targetPos, t);
	camera->currQRot = DirectX::XMQuaternionSlerp(camera->currQRot, camera->targetQRot, t);
	camera->currZoom = LerpR32(camera->currZoom, camera->targetZoom, t);
    }

    UpdateInternalTransformations(camera);
    return true;
}



internal void
InitArcBall(dx_camera* camera, win32_voxel_chunk* win32VoxelChunk)
{

    camera->eye = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    camera->viewCenter = {win32VoxelChunk->chunk->centoid.x, -win32VoxelChunk->chunk->centoid.z,
	win32VoxelChunk->chunk->centoid.y, 0.0f};

    camera->up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


    DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(camera->viewCenter, camera->eye);

    DirectX::XMVECTOR zAxis = DirectX::XMVector3Normalize(dir);
    DirectX::XMVECTOR xAxis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(zAxis,
										  DirectX::XMVector3Normalize(
										      camera->up)));
    DirectX::XMVECTOR yAxis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xAxis, zAxis));
    xAxis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(zAxis, yAxis));
    camera->targetPos = camera->viewCenter;
    camera->targetZoom = 15.f;
    //(FromRotMat, transposed), normalize

    zAxis = DirectX::XMVectorScale(zAxis, -1.0f);
    DirectX::XMVECTOR w = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    DirectX::XMMATRIX rotMat = DirectX::XMMATRIX(xAxis, yAxis, zAxis, w);
    
    rotMat = DirectX::XMMatrixTranspose(rotMat);
    camera->targetQRot = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionRotationMatrix(rotMat));

    camera->positionTo = camera->position = camera->targetPos;
    camera->zoomingTo = camera->currZoom = camera->targetZoom;
    camera->qRotationTo = camera->currQRot = camera->targetQRot;
    

    UpdateInternalTransformations(camera);
    
}

internal void
ProcessMouseArcBallInputs(mouse_movements* mouse, game_input* input, dx_camera* camera, game_state* gameState)
{
    if (input->mouseButtons[0].released)
    {
	mouse->arcBallStart = mouse->arcBallCurrent;
	mouse->recMousePos = false;
	OutputDebugString("RELEASED\n");
	//My release event
	input->mouseButtons[0].endedDown = false;
    }

    if (input->mouseButtons[0].started)
    {
	mouse->arcBallStart = v2{(r32)input->mouseXBounded, (r32)input->mouseYBounded};
	mouse->recMousePos = true;
	OutputDebugString("STARTED\n");
	//(My press event)
	InitTransformation(mouse);
    }

    if (input->mouseButtons[0].endedDown)
    {
	mouse->arcBallCurrent = v2{(r32)input->mouseXBounded, (r32)input->mouseYBounded};

	RotateArcBallCam(camera, mouse);
	UpdateArcBallTransformation(camera);

	DirectX::XMVECTOR pos = camera->viewInverted.r[3];
	DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(camera->viewInverted.r[2]);
	
	gameState->gameCamera.pos = FromXMVECTORToV3(pos);

	gameState->gameCamera.forward = FromXMVECTORToV3(forward);
	
	char posBuffer[256];
	sprintf_s(posBuffer, sizeof(posBuffer), "X: %f, Y: %f, Z: %f\n",
		  DirectX::XMVectorGetX(camera->position), DirectX::XMVectorGetY(camera->position), DirectX::XMVectorGetZ(camera->position));
	OutputDebugString(posBuffer);
    }

}

internal void
UpdateGameStateInfo(game_state* gameState, dx_camera* camera)
{
   DirectX::XMMATRIX projection = XMLoadFloat4x4(&camera->constantBufferData.projection);

//    projection = DirectX::XMMatrixInverse(nullptr, projection);
    
    gameState->gameCamera.proj = FromXMMATRIXToM4(projection);
    gameState->gameCamera.viewInverted = FromXMMATRIXToM4(camera->viewInverted);
    DirectX::XMVECTOR pos = camera->viewInverted.r[3];
    
    gameState->gameCamera.pos = FromXMVECTORToV3(pos);
	    
}

internal void
UpdateCameraArc(dx_camera* camera, mouse_movements* mouse, game_input* input, game_state* gameState)
{
    //Get curr and starting positions from the mouse 
    ProcessMouseArcBallInputs(mouse, input, camera, gameState);

}

internal void
UpdateCameraFP(dx_camera* camera, game_state* gameState)
{
    //Calculate the forward vector of our camera (this is the equation)
    camera->front =
	{
	    (r32)(cos(camera->pitch) * sin(camera->yaw)),
	    (r32)(sin(camera->pitch)),
	    (r32)(cos(camera->yaw) * cos(camera->pitch)),	    	    
	};

    //Normalize the magnitude

    camera->front = DirectX::XMVector3Normalize(camera->front);

    camera->right = DirectX::XMVector4Normalize(DirectX::XMVector3Cross(camera->front, camera->worldUp));
    camera->up = DirectX::XMVector4Normalize(DirectX::XMVector3Cross(camera->right, camera->front));

    //Update our view matrix
    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.view,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixLookAtRH(
		camera->position,
		DirectX::XMVectorAdd(camera->front, camera->position),
		camera->up)
	    )
	);

	
    char posBuffer[256];
    sprintf_s(posBuffer, sizeof(posBuffer), "X: %f, Y: %f, Z: %f\n",
	      DirectX::XMVectorGetX(camera->position), DirectX::XMVectorGetY(camera->position), DirectX::XMVectorGetZ(camera->position));
    OutputDebugString(posBuffer);    
    gameState->gameCamera.pos = FromXMVECTORToV3(camera->position);

}

internal void
ProcessMouseControlFP(dx_camera* camera, r32 xChange, r32 yChange)
{
    camera->yaw += xChange;
    camera->pitch += yChange;

    if (camera->pitch > 89.0f)
    {
	camera->pitch = 89.0f;
    }
    if (camera->pitch < -89.0f)
    {
	camera->pitch = -89.0f;
    }
}

internal void
ProcessPlayerMovement(game_controller_input* controller, dx_camera* camera, r32 deltaTime)
{
    if (controller->testKey.started)
    {
	freeCam = !freeCam;
	OutputDebugString("Swapped to FreeCam\n");
	if (!freeCam)
	{
	    ShowCursor(true);
	}
    }
    
    r32 velocity = camera->movementSpeed * deltaTime;
    if (freeCam)
    {
	if (controller->moveForward.endedDown)
	{
	    camera->position = DirectX::XMVectorAdd(camera->position, DirectX::XMVectorScale(camera->front, velocity));
	}
	if (controller->moveBackward.endedDown)
	{
	    camera->position = DirectX::XMVectorSubtract(camera->position, DirectX::XMVectorScale(camera->front, velocity));
	}
	if (controller->moveRight.endedDown)
	{
	    camera->position = DirectX::XMVectorAdd(camera->position, DirectX::XMVectorScale(camera->right, velocity));
	}
	if (controller->moveLeft.endedDown)
	{
	    camera->position = DirectX::XMVectorSubtract(camera->position, DirectX::XMVectorScale(camera->right, velocity));	
	}
	if (controller->moveUp.endedDown)
	{
	    camera->position = DirectX::XMVectorAdd(camera->position, DirectX::XMVectorScale(camera->worldUp, velocity));
	}
	if (controller->moveDown.endedDown)
	{
	    camera->position = DirectX::XMVectorSubtract(camera->position, DirectX::XMVectorScale(camera->worldUp, velocity));
	}
    }    
}

internal void
Win32ProcessKeyboardMessage(game_button_state* newState, game_button_state* oldState, bool32 isDown)
{
    if (newState->endedDown != isDown)
    {
	newState->endedDown = isDown;

	newState->started = isDown;
	++newState->halfTransitionCount;
    }
    newState->wasDown = isDown;

    if (oldState->released)
    {
	oldState->released = false;
    }
    
    if (oldState->endedDown)
    {
	newState->started = false;
	
    }

    if (!newState->endedDown && oldState->endedDown)
    {
	newState->released = true;
    }
}

internal void
Win32SetMouseStates(game_input* newInput, game_input* oldInput)
{
    for (int i = 0; i < 5; i++)
    {
	game_button_state* nMouse = &newInput->mouseButtons[i];
	game_button_state* oMouse = &oldInput->mouseButtons[i];

	if (nMouse->released || oMouse->released)
	{
	    oMouse->endedDown = false;
	    nMouse->endedDown = false;
	    oMouse->released = false;
	    nMouse->released = false;
	}

	if (nMouse->started || oMouse->started)
	{
	    oMouse->started = false;
	    nMouse->started = false;
	}
    }
}

internal void
Win32ProcessMouseMessage(game_button_state* nMouse, game_button_state* oMouse, bool32 down)
{
    if (down)
    {
	OutputDebugString("Mouse button down\n");
	nMouse->started = true;
	oMouse->started = true;
	nMouse->endedDown = true;
	oMouse->endedDown = true;
    }
    else
    {
	OutputDebugString("Mouse button up\n");
	nMouse->released = true;
	oMouse->released = true;
    }
}

internal void
Win32ProcessPendingMessages(game_controller_input* keyboardController, game_controller_input* oldKeyboardController, dx_camera* camera, mouse_movements* mouse, game_input* newInput, game_input* oldInput)
{
    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
	switch(msg.message)
	{
	case WM_ACTIVATE:
	{
	    OutputDebugString("App activated\n");
	};
	case WM_QUIT:
	{
	    running = false;
	} break;
	case WM_SIZE:
	{
	    CreateWindowSizeDependentResources(camera);
	} break;
	case WM_MOUSEACTIVATE:
	{
	    OutputDebugString("I am clicking on the screen rn\n");
	} break;
	case WM_INPUT:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_KEYDOWN:
	{
	    UINT dwSize = 0;
	    
	    
	    u32 VKCode = (u32)msg.wParam;
	    bool32 wasDown = ((msg.lParam & (1 << 30)) != 0);
	    bool32 isDown = ((msg.lParam & (1 << 31)) == 0);

	    if (wasDown != isDown)
	    {
		if (VKCode == 'W')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveForward,
						&oldKeyboardController->moveForward, isDown);
		}
		else if (VKCode == 'A')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveLeft,
						&oldKeyboardController->moveLeft, isDown);
		}
		else if (VKCode == 'S')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveBackward,
						&oldKeyboardController->moveBackward, isDown);
		}
		else if (VKCode == 'D')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveRight,
						&oldKeyboardController->moveRight, isDown);
		}
		else if (VKCode == 'Q')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveDown,
						&oldKeyboardController->moveDown, isDown);
		}
		else if (VKCode == 'E')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveUp,
						&oldKeyboardController->moveUp, isDown);
		}
		else if (VKCode == 'F')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->testKey,
						&oldKeyboardController->testKey, isDown);

		}
		else if (VKCode == VK_ESCAPE)
		{
		    running = false; 
		}
		else if (VKCode == VK_LBUTTON)
		{
		    OutputDebugString("LButton in process hit override\n");
		}
	    }



	    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	    LPBYTE lpb = (LPBYTE)memoryPoolCode.PushStruct(&programState->perFrameArena, (sizeof(BYTE)) * dwSize);
	    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

	    RAWINPUT* raw = (RAWINPUT*)lpb;
	    if (raw->header.dwType == RIM_TYPEMOUSE)
	    {
		mouse->x = (r32)raw->data.mouse.lLastX;
		mouse->y = (r32)raw->data.mouse.lLastY;


		
		bool32 isMouseDown = false;

		//I imagine this could be better butttttt...
		if (raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN)
		{
		    Win32ProcessMouseMessage(&newInput->mouseButtons[e_mouse_buttons::middle_mouse],
					     &oldInput->mouseButtons[e_mouse_buttons::middle_mouse],
					     true);
		}
		else if (raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP)
		{
		    Win32ProcessMouseMessage(&newInput->mouseButtons[e_mouse_buttons::middle_mouse],
					     &oldInput->mouseButtons[e_mouse_buttons::middle_mouse],
								     false);
		}

		if (raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN)
		{
		    Win32ProcessMouseMessage(&newInput->mouseButtons[e_mouse_buttons::left_mouse],
					     &oldInput->mouseButtons[e_mouse_buttons::left_mouse],
					     true);
		}
		else if (raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP)
		{
		    Win32ProcessMouseMessage(&newInput->mouseButtons[e_mouse_buttons::left_mouse],
					     &oldInput->mouseButtons[e_mouse_buttons::left_mouse],
					     false);
		}

		if (raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN)
		{
		    Win32ProcessMouseMessage(&newInput->mouseButtons[e_mouse_buttons::right_mouse],
					     &oldInput->mouseButtons[e_mouse_buttons::right_mouse],
					     true);
		}
		else if (raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP)
		{
		    Win32ProcessMouseMessage(&newInput->mouseButtons[e_mouse_buttons::right_mouse],
					     &oldInput->mouseButtons[e_mouse_buttons::right_mouse],
					     false);
		}
		else if (raw->data.mouse.usButtonFlags == RI_MOUSE_WHEEL)
		{
		    
		}

	    }

	} break;
	default:
	{
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	} break;
	}
    }
}

LRESULT CALLBACK Win32MainWindowProc(HWND hwnd,
				     UINT uMsg,
				     WPARAM wParam,
				     LPARAM lParam)
{
    LRESULT result = 0;
    switch(uMsg)
    {
    case WM_ACTIVATEAPP:
    {
	OutputDebugString("App activated\n");
    } break;
    case WM_LBUTTONDOWN:
    {
	OutputDebugString("Mouse button in WindowProc Pressed\n");
    } break;
    default:
    {
	result = DefWindowProc(hwnd, uMsg, wParam, lParam);	
    } break;
    }
    
    return(result);
}

internal shader_code
CompileShaders(void)
{
    HRESULT hr;
    shader_code result;
    hr = D3DCompileFromFile(L"vshader.hlsl", 0, 0, "main", "vs_5_0", 0, 0, &result.vertexShaderCode, 0);
    hr = D3DCompileFromFile(L"pshader.hlsl", 0, 0, "main", "ps_5_0", 0, 0, &result.pixelShaderCode, 0);
    return(result);
}

internal void 
CreateShaders(shaders* shaderResources)
{
    /*
      Now theoretically speaking, you could create a new arena for all of the shader bytes, then empty it for each
      shader you create, but that's just a theory
     */

    HRESULT hr = {};
    
    FILE* vShader, *pShader, *debugVShader; //vertex (v) pixel (p)
    BYTE* bytes = 0;

    size_t destSize = 4096;
    size_t bytesRead = 0;
    thread_context blankThread = {};

    bytes = (BYTE*)memoryPoolCode.PushStruct(&programState->setupArena, sizeof(bytes));



    debug_read_file_result fileResult = DEBUGPlatformReadEntireFile(&blankThread, "../build/CubeVertexShader.cso");

    bytes = (BYTE*)fileResult.contents;
    hr = d3dDevice->CreateVertexShader(fileResult.contents,
				  fileResult.contentsSize,
				  nullptr,
				  &shaderResources->voxelVertexShader);



    //This is the descriptor for the vertex data layout since d3d doesn't define one
    //This project could include more than 2 elements in the struct but we would have to modify the description below
    //to fit the struct
    D3D11_INPUT_ELEMENT_DESC iaDesc[] =
    {
	{
	    "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
	    0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
	},

	{
	    "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
	    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0
	},
    };

    //What is the input for the function in our VertexShader??? This...
    
    hr = d3dDevice->CreateInputLayout(
	iaDesc,
	ArrayCount(iaDesc),
	bytes,
	fileResult.contentsSize,
	&shaderResources->voxelInputLayout
	);

    debug_read_file_result debugFileResult = DEBUGPlatformReadEntireFile(&blankThread, "../build/DebugVertexShader.cso");

    bytes = (BYTE*)debugFileResult.contents;
    hr = d3dDevice->CreateVertexShader(debugFileResult.contents,
				       debugFileResult.contentsSize,
				       nullptr,
				       &shaderResources->debugVertexShader);

    hr = d3dDevice->CreateInputLayout(iaDesc,
				      ArrayCount(iaDesc),
				      bytes,
				      debugFileResult.contentsSize,
				      &shaderResources->debugInputLayout);
    

    debug_read_file_result pixelShaderResult = DEBUGPlatformReadEntireFile(&blankThread, "../build/CubePixelShader.cso");
    
    bytes = (BYTE*)pixelShaderResult.contents;
    hr = d3dDevice->CreatePixelShader(
	pixelShaderResult.contents,
	pixelShaderResult.contentsSize,
	nullptr,
	&shaderResources->pixelShader);

    CD3D11_BUFFER_DESC cbDesc(
	sizeof(constant_buffer_struct),
	D3D11_BIND_CONSTANT_BUFFER);

    hr = d3dDevice->CreateBuffer(
	&cbDesc,
	nullptr,
	&shaderResources->voxelConstantBuffer);

    CD3D11_BUFFER_DESC cbdDesc(
	sizeof(constant_buffer_struct),
	D3D11_BIND_CONSTANT_BUFFER);

    hr = d3dDevice->CreateBuffer(
	&cbDesc,
	nullptr,
	&shaderResources->debugConstantBuffer);
	
}

//NOTE: this function should be called asynchronously, Take the time to have it execute
//on a separate thread
internal void
CreateDeviceDependentResources(shaders* shaders, direct_x_loaded_buffers* loadedBuffers, memory_arena* mainArena, program_memory* programMemory)
{

    CreateShaders(shaders);

#if DIRECTXLOAD    
#if 1
    directXOBJCode.DirectXLoadOBJ("D:/ExternalCustomAPIs/OBJLoader/misc/cubetester_normals.obj", mainArena, programMemory, d3dDevice, loadedBuffers);
#else
    directXOBJCode.DirectXLoadOBJ("D:/ExternalCustomAPIs/OBJLoader/misc/monkey.obj", mainArena, programMemory, d3dDevice, loadedBuffers);
#endif    
#endif

}

internal void
Win32InitAllDebugVectors(win32_debug_vectors* debugVectors, memory_arena* arena, memory_arena* debugVectorsArena, win32_state* win32State)
{
    //Create a single index buffer for all of our debug vectors
    //Might not have to create a separate CB for the vectors, could just utilize b1
    HRESULT hr = {};


    debugVectors->numOfDebugIndices = 2;
    debugVectors->debugVectorIndices[0] = 0;
    debugVectors->debugVectorIndices[1] = 1;
    
    CD3D11_BUFFER_DESC indexDesc(
	sizeof(u16) * debugVectors->numOfDebugIndices,
	D3D11_BIND_INDEX_BUFFER,
	D3D11_USAGE_DEFAULT);

    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
    indexData.pSysMem = debugVectors->debugVectorIndices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    hr = d3dDevice->CreateBuffer(
	&indexDesc,
	&indexData,
	&debugVectors->indexBuffer);

    //Create our vertex buffer for the vectors

    debugVectors->vsInput = (vertex_position_color*)memoryPoolCode.PushArraySized(arena, (size_t)(sizeof(vertex_position_color) * debugVectors->numOfDebugIndices));

    debugVectors->vsInput[0].pos = {};
    debugVectors->vsInput[1].pos = {};
    
    D3D11_BUFFER_DESC vertexDesc;
    vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
//    vertexDesc.ByteWidth = sizeof(vertex_position_color) * debugVectors->numOfDebugIndices;
    vertexDesc.ByteWidth = sizeof(vertex_position_color) * 2000;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));
    vertexData.pSysMem = &debugVectors->vsInput[0];
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    hr = d3dDevice->CreateBuffer(
	&vertexDesc,
	&vertexData,
	&debugVectors->vertexBuffer);
}


#if 0
internal void
CreateNewDebugVector(win32_debug_vectors* debugVectors, memory_arena* arena, game_input* input, dx_camera* camera)
{
    //You still have to create the init for vectors in the platform code, currently its in your game code which
    //we were deciding might be a little problematic
    
    v2 screenToNDC = ScreenToCoordNDC(GetMouseScreenCoords(input));
    v3 start = v3{screenToNDC.x, screenToNDC.y, 0.0f};
    v3 end = start + (FromXMVECTORToV3(camera->eye) * debugVectors->debugVectorLength);
    v3 color = v3{0.0f, 1.0f, 0.0f};

    win32_debug_vector newVec = {};
    newVec.start = FromV3ToXMVECTOR(start);
    newVec.end = FromV3ToXMVECTOR(end);
    newVec.color = FromV3ToXMVECTOR(color);

    memoryPoolCode.AddListedItem(debugVectors->debugVectorsMemory,
				 (void*)&newVec,
				 sizeof(newVec),
				 &debugVectors->debugVectorNodes);

    debugVectors->numOfDrawnVectors = debugVectors->numOfDrawnVectors + 1;
}
#endif

#if 0
//DEAD CODE? ^^^^ vvvvv
internal void
CreateNewDebugVector(win32_debug_vectors* debugVectors, game_state* gameState, memory_arena* debugTempArena)
{
    //Make an array of the lines in the scene, clear it whenever it's larger, continue to draw
    //no matter what
    if (debugVectors->currDrawnVectors < gameState->numOfDrawnVectors)
    {


	//Or we could edit a constant buffer

    }

}
#endif

internal void
Win32InitVoxelGrid(win32_voxel_chunk* win32VoxelChunk, memory_arena* arena, memory_arena* indexBufferArena, win32_state* win32State)
{
    HRESULT hr = {};
    
    i32 numOfRenderedVoxels = win32VoxelChunk->chunk->numOfRenderedVoxels;
    win32VoxelChunk->indexBuffers = (ID3D11Buffer**)memoryPoolCode.PushArraySized(arena,
										  (size_t)(sizeof(ID3D11Buffer*) * numOfRenderedVoxels)); //this is also an array that might benefit from becoming a linked list

    win32VoxelChunk->indexBufferMemory = (listed_memory*)memoryPoolCode.PushStruct(indexBufferArena, sizeof(listed_memory));
    memoryPoolCode.InitListedMemory(win32VoxelChunk->indexBufferMemory, indexBufferArena, sizeof(index_buffer_info));

    
    r32_3 testColors[] =
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

    
    win32VoxelChunk->vertexBuffers = (ID3D11Buffer*)memoryPoolCode.PushStruct(arena,
									      (size_t)(sizeof(ID3D11Buffer*)));


    win32VoxelChunk->vsInput = (vertex_position_color*)memoryPoolCode.PushArraySized(arena, (size_t)(sizeof(vertex_position_color) * win32VoxelChunk->chunk->voxelVertCount));

    for (i32 i = 0, j = 0; j < win32VoxelChunk->chunk->voxelVertCount; i += 3, j++)
    {
	win32VoxelChunk->vsInput[j].pos.x = win32VoxelChunk->chunk->verts[i];
	win32VoxelChunk->vsInput[j].pos.y = win32VoxelChunk->chunk->verts[i + 1];
	win32VoxelChunk->vsInput[j].pos.z = win32VoxelChunk->chunk->verts[i + 2];

	DirectX::XMFLOAT3 vertColor = {testColors[j].x, testColors[j].y, testColors[j].z};
	
	win32VoxelChunk->vsInput[j].color = vertColor;
    }
    
    D3D11_BUFFER_DESC vertexDesc;
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.ByteWidth = sizeof(vertex_position_color) * win32VoxelChunk->chunk->voxelVertCount;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = 0;
    vertexDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

        
    vertexData.pSysMem = &win32VoxelChunk->vsInput[0];
    
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    hr = d3dDevice->CreateBuffer(
	&vertexDesc,
	&vertexData,
	&win32VoxelChunk->vertexBuffers);

    //Create one constant buffer of the world positions
    //NOTE MOVE THIS OUT INTO A SEPARATE INITIALIZATION FUNCTION FOR OTHER CB's
    //YOU DON'T WANT TO KEEP THIS IN A FUNCTION THAT IS SUPPOSED TO INIT THE VOXEL GRID ONLY
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(object_constants);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = d3dDevice->CreateBuffer(&cbDesc, NULL, &win32State->worldObjectConstants);


    D3D11_BUFFER_DESC worldCbDesc = {};
    worldCbDesc.Usage = D3D11_USAGE_DEFAULT;
    worldCbDesc.ByteWidth = sizeof(voxel_chunk_world_constant);
    worldCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    worldCbDesc.CPUAccessFlags = 0;

    //I think this is DEAD CODE 
    DirectX::XMMATRIX rotMat = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX scaleMat = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(win32VoxelChunk->chunk->chunkWorldLocation.x,
							      win32VoxelChunk->chunk->chunkWorldLocation.y,
							      win32VoxelChunk->chunk->chunkWorldLocation.z);

    //srt

    DirectX::XMMATRIX scaleRotMat = DirectX::XMMatrixMultiply(scaleMat, rotMat);
    DirectX::XMMATRIX world = DirectX::XMMatrixMultiply(scaleRotMat, transMat);

    D3D11_SUBRESOURCE_DATA voxelWorldCBData;
    ZeroMemory(&voxelWorldCBData, sizeof(D3D11_SUBRESOURCE_DATA));
    voxelWorldCBData.pSysMem = &world;
    voxelWorldCBData.SysMemPitch = 0;
    voxelWorldCBData.SysMemSlicePitch = 0;    
    
    hr = d3dDevice->CreateBuffer(&worldCbDesc, &voxelWorldCBData, &win32VoxelChunk->voxelChunkWorldCB);
    
    //You'll also want to probably store this on a specific arena that can be wiped whenever the chunk becomes dirty

    listed_memory_node* renderedIndexNode = (listed_memory_node*)win32VoxelChunk->chunk->renderedVoxelNodes;


    
    for (i32 i = 0; i < numOfRenderedVoxels; i++)
    {

	Assert(renderedIndexNode);
	
	rendered_voxel_info* renderedVoxelData = (rendered_voxel_info*)renderedIndexNode->data;
	Assert(renderedVoxelData);
	
	
	i32 index = renderedVoxelData->index;
	
	voxel* currVoxel = &win32VoxelChunk->chunk->voxels[index];

	v3 voxelPos = currVoxel->pos;


	index_buffer_info newIndexInfo = {};
	
	CD3D11_BUFFER_DESC indexDesc(
	    sizeof(u16) * currVoxel->renderedIndiceCount,
	    D3D11_BIND_INDEX_BUFFER);

	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
	indexData.pSysMem = currVoxel->renderedIndices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

#if 0	
	hr = d3dDevice->CreateBuffer(
	    &indexDesc,
	    &indexData,
	    &win32VoxelChunk->indexBuffers[i]);
#else
	hr = d3dDevice->CreateBuffer(
	    &indexDesc,
	    &indexData,
	    &newIndexInfo.indexBuffer);
#endif
	renderedIndexNode = renderedIndexNode->next;

#if 0
	memoryPoolCode.AddListedItem(win32VoxelChunk->indexBufferMemory,
				     (void*)&newIndexInfo,
				     sizeof(index_buffer_info),
				     &win32VoxelChunk->indexBufferNodes);
#else
	memoryPoolCode.AddToEndOfList(win32VoxelChunk->indexBufferMemory,
				      (void*)&newIndexInfo,
				      sizeof(index_buffer_info),
				      &win32VoxelChunk->indexBufferNodes);
#endif	
    }
}

internal void
TRTAP(LARGE_INTEGER startTime)
{
    LARGE_INTEGER endTime = Win32GetWallClock();
    
    r32 secondsRendered = Win32GetSecondsElapsed(startTime, endTime);

    char timerBuffer[256];

    sprintf_s(timerBuffer, sizeof(timerBuffer), "Seconds Rendered %f\n", secondsRendered);
    OutputDebugString(timerBuffer);
    
}

internal void
RenderDebug(shaders* shader, win32_debug_vectors* debugVectors, game_state* gameState, dx_camera* camera)
{
    context->UpdateSubresource(shader->debugConstantBuffer, 0, nullptr, &camera->constantBufferData, 0, 0);
    
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context->IASetInputLayout(shader->debugInputLayout);
    context->VSSetConstantBuffers(0, 1, &shader->debugConstantBuffer);

    
    UINT stride = sizeof(vertex_position_color);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, &debugVectors->vertexBuffer, &stride, &offset);

    context->VSSetShader(shader->debugVertexShader, nullptr, 0);

    
    HRESULT hr = {};



    listed_memory_node* currNode = gameState->debugVectorNodes;
    vertex_position_color* dest = debugVectors->vsInput;

    
    i32 count = 0;
    while (currNode && (count < gameState->numOfDrawnVectors))
    {
	game_debug_vector* currDebugVector = (game_debug_vector*)currNode->data;

	DirectX::XMVECTOR start = FromV3ToXMVECTOR(currDebugVector->start);
	DirectX::XMVECTOR end = FromV3ToXMVECTOR(currDebugVector->end);
//	DirectX::XMVECTOR color = FromV3ToXMVECTOR(currDebugVector->color);
	DirectX::XMVECTOR sColor = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR eColor = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMStoreFloat3(&dest[0].pos, start);
	DirectX::XMStoreFloat3(&dest[0].color, sColor);

	DirectX::XMStoreFloat3(&dest[1].pos, end);
	DirectX::XMStoreFloat3(&dest[1].color, eColor);

	dest += 2;

	currNode = currNode->next;
	count++;
    }

    if (count > 0)
    {
	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = context->Map(debugVectors->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr))
	{
	    size_t bytesToCopy = sizeof(vertex_position_color) * (count * 2);
	    MemCpy(mapped.pData, debugVectors->vsInput, bytesToCopy);

	    context->Unmap(debugVectors->vertexBuffer, 0);
	}
	context->Draw(count * 2, 0);
    }
}

internal void
RenderVoxelCubes(shaders* shader, dx_camera* camera, win32_voxel_chunk* win32VoxelChunk, win32_state* win32State)
{
    //Start rendering the voxels so we can see what is going on with the cubes being rendered
//Check the speeds here and where it could be casuing issues


    r32 teal [] = {0.098f, 0.439f, 0.439f, 1.000f};

    context->UpdateSubresource(shader->voxelConstantBuffer, 0, nullptr, &camera->constantBufferData, 0, 0);

    context->ClearRenderTargetView(
	renderTarget,
	teal);

    context->ClearDepthStencilView(
	depthStencilView,
	D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
	1.0f,
	0);

    context->OMSetRenderTargets(
	1,
	&renderTarget,
	depthStencilView);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(shader->voxelInputLayout);
    context->VSSetConstantBuffers(0, 1, &shader->voxelConstantBuffer);
    context->VSSetConstantBuffers(2, 1, &win32VoxelChunk->voxelChunkWorldCB);

    UINT stride = sizeof(vertex_position_color);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &win32VoxelChunk->vertexBuffers, &stride, &offset);    

    context->VSSetShader(
	shader->voxelVertexShader,
	nullptr,
	0);

    context->PSSetShader(
	shader->pixelShader,
	nullptr,
	0);

   
    HRESULT hr = {};

    DirectX::XMVECTOR voxelOffset = DirectX::XMVectorSet(-10.0f, -10.0f, -10.0f, 0.0f);

    listed_memory_node* renderedVoxelNode = (listed_memory_node*)win32VoxelChunk->chunk->renderedVoxelNodes;

    listed_memory_node* indexBufferNode = (listed_memory_node*)win32VoxelChunk->indexBufferNodes;
    
    for (int i = 0; i < win32VoxelChunk->chunk->numOfRenderedVoxels; i++)
    {
	Assert(renderedVoxelNode);
	Assert(indexBufferNode);

	rendered_voxel_info* indexData = (rendered_voxel_info*)renderedVoxelNode->data;
	index_buffer_info* indexBufferData = (index_buffer_info*)indexBufferNode->data;

	Assert(indexData);
	Assert(indexBufferData);
	i32 index = indexData->index;
	
	context->VSSetConstantBuffers(1, 1, &win32State->worldObjectConstants);


	context->IASetIndexBuffer(indexBufferData->indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	
	//We are getting the constant buffer on the GPU and using for the CPU
	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = context->Map(win32State->worldObjectConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	object_constants* data = (object_constants*)mapped.pData;

	voxel* currVoxel = &win32VoxelChunk->chunk->voxels[index];


	DirectX::XMVECTOR vecWorld = DirectX::XMVectorSet(currVoxel->pos.x, currVoxel->pos.y, currVoxel->pos.z, 1.0f);

	DirectX::XMStoreFloat4(&data->worldPos, vecWorld);
	
	context->Unmap(win32State->worldObjectConstants, 0);
	
	context->DrawIndexed(
	    currVoxel->renderedIndiceCount,
	    0,
	    0);

	renderedVoxelNode = renderedVoxelNode->next;
	indexBufferNode = indexBufferNode->next;

    }

    //Draw lined objects (line_strip_topology)

    //Complete our conversions from game to platform code
    //Draw out the number of lines required 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#if 0     
    //I'm thinking about the fact that I might need another constant buffer for these
    //Should I write an entirely new shader?k
    for (i32 i = 0; i < gameState->numOfDrawnVectors; i++)
    {
	
    }
#endif
    
    
}

int CALLBACK WinMain(HINSTANCE hInstance,
		     HINSTANCE hPrevInstance,
		     LPSTR lpCmdLine,
		     int nCmdShow)
{
    win32_state win32State = {};

    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    perfCountFrequency = perfCountFrequencyResult.QuadPart;    

    Win32GetEXEFilename(&win32State);

    char sourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildExePathFilename(&win32State, "game_layer.dll", sizeof(sourceGameCodeDLLFullPath), sourceGameCodeDLLFullPath);

    char tempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildExePathFilename(&win32State, "game_temp.dll", sizeof(tempGameCodeDLLFullPath), tempGameCodeDLLFullPath);

    char gameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildExePathFilename(&win32State, "lock.tmp", sizeof(gameCodeLockFullPath), gameCodeLockFullPath);
/*
      Load our memory pool library
*/

    //Well if you ever decide that you need to debug past versions of the memory code, this is probably
    //what was loading in the updated version of the library instead of the old version I was trying to
    //test against...
    HMODULE memoryPoolLibrary = LoadLibrary("D:/ExternalCustomAPIs/MemoryPools/dll/memory_pools.dll");

    if (memoryPoolLibrary)
    {
	memoryPoolCode.PushStruct = (memory_pool_push_struct*)GetProcAddress(memoryPoolLibrary, "PushStruct");
	memoryPoolCode.PushArray = (memory_pool_push_array*)GetProcAddress(memoryPoolLibrary, "PushArray");
	memoryPoolCode.PoolAlloc = (memory_pool_alloc*)GetProcAddress(memoryPoolLibrary, "PoolAlloc");
	memoryPoolCode.InitArena = (memory_pool_initialize_arena*)GetProcAddress(memoryPoolLibrary, "InitializeArena");
	memoryPoolCode.InitArena2 = (memory_pool_initialize_arena2*)GetProcAddress(memoryPoolLibrary, "InitializeArena2");
	memoryPoolCode.ClearArena = (memory_pool_clear_arena*)GetProcAddress(memoryPoolLibrary, "ClearArena");
	memoryPoolCode.PushArraySized = (memory_pool_push_array_sized*)GetProcAddress(memoryPoolLibrary, "PushArraySized");
	memoryPoolCode.InitListedMemory = (memory_pool_init_listed_memory*)GetProcAddress(memoryPoolLibrary, "InitializeListedMemory");
	memoryPoolCode.AddListedItem = (memory_pool_add_listed_item*)GetProcAddress(memoryPoolLibrary, "AddListedItem");
	memoryPoolCode.RemoveListedItem = (memory_pool_remove_listed_item*)GetProcAddress(memoryPoolLibrary, "RemoveListedItem");
	memoryPoolCode.AddToEndOfList = (memory_pool_add_to_end*)GetProcAddress(memoryPoolLibrary, "AddToEndOfList");
	
    }
    if (memoryPoolCode.PushStruct && memoryPoolCode.PushArray && memoryPoolCode.PoolAlloc)
    {
	OutputDebugString("Memory Pool Code Successfully Loaded");
    }

#if DIRECTXLOAD
    HMODULE directXOBJLibrary = LoadLibrary("D:/ExternalCustomAPIs/OBJLoader/dll/directx_obj_loader.dll");
    if (directXOBJLibrary)
    {
	directXOBJCode.DirectXLoadOBJ = (direct_x_load_obj*)GetProcAddress(directXOBJLibrary, "DirectXLoadOBJ");
    }
#else
    HMODULE parseOBJLibrary = LoadLibrary("D:/ExternalCustomAPIs/OBJLoader/dll/obj_loader.dll");
    if (parseOBJLibrary)
    {
	parseObjCode.ParseOBJData = (parse_obj_data*)GetProcAddress(parseOBJLibrary, "ParseOBJData");
    }
#endif    
    
    
    program_memory memory = {};


    memory.transientStorageSize = Megabytes(64);

    memory.permanentStorageSize = Gigabytes(1);
    
    memoryPoolCode.PoolAlloc(0, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE, &memory);

    programState = (program_state*)memory.permanentStorage;    

    //arena, size, base

#if 0    
    memoryPoolCode.InitArena2(&programState->setupArena, memory.permanentStorageSize - sizeof(program_state),
				   (u8*)memory.permanentStorage + sizeof(program_state));

    memoryPoolCode.InitArena2(&programState->perFrameArena, memory.transientStorageSize - sizeof(program_state),
				   (u8*)memory.transientStorage + sizeof(program_state));

#else
    size_t setupArenaAllocSize = Megabytes(700);
    size_t perFrameArenaAllocSize = Megabytes(50);

#if 0    
    memory.permanentArenaBase = (u8*)memory.permanentStorage + setupArenaAllocSize - sizeof(program_state);
    //Now set it up for transient storage bc you are actually using it lmaoooooo
    memory.transientArenaBase = (u8*)memory.transientStorage + perFrameArenaAllocSize - sizeof(program_state);
#else
    memory.permanentArenaBase = (u8*)memory.permanentStorage + sizeof(program_state);
    memory.transientArenaBase = (u8*)memory.transientStorage + sizeof(program_state);
    
#endif    
    
    memoryPoolCode.InitArena(&programState->setupArena, setupArenaAllocSize, &memory, e_arena_type::permanent);
    memoryPoolCode.InitArena(&programState->perFrameArena, perFrameArenaAllocSize, &memory, e_arena_type::transient);

    size_t debugVectorArenaSize = Megabytes(5);
    memoryPoolCode.InitArena(&programState->debugVectorArena, debugVectorArenaSize, &memory, e_arena_type::permanent);
    //This is probably overkill
    size_t chunkRenderedIndexSize = Megabytes(10);
    memoryPoolCode.InitArena(&programState->renderedVoxelIndexArena, chunkRenderedIndexSize, &memory, e_arena_type::permanent);


    size_t indexBufferArenaSize = Megabytes(5);
    memoryPoolCode.InitArena(&programState->indexBufferArena, indexBufferArenaSize, &memory, e_arena_type::permanent);
    
    
    i32* foo = (i32*)memoryPoolCode.PushStruct(&programState->debugVectorArena, sizeof(i32));
    
    
#endif    
    


    

    UINT desiredSchedulerMs = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMs) == TIMERR_NOERROR);
    
    
    D3D_FEATURE_LEVEL levels[] = {
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
	D3D_FEATURE_LEVEL_9_2,
	D3D_FEATURE_LEVEL_9_1,    
    };

    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; //If we want to be able to have d3d interact w/ d2d
    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_create_device_flag7
#if 1
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif    
    



    D3D_FEATURE_LEVEL featureLevel;


    IDXGIAdapter* adapter = NULL;
    IDXGIFactory6* factory = 0;

    HR(CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)&factory));

    //Set this back to 1 after debugging graphics
//    factory->EnumAdapters(1, &adapter);
    HRESULT hr = {};
    
    hr = factory->EnumAdapterByGpuPreference(0,
					DXGI_GPU_PREFERENCE::DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
					__uuidof(IDXGIAdapter),
					(void**)&adapter);
					
#if 1    
    IDXGIOutput* adapterOutput = {};
    adapter->EnumOutputs(1, &adapterOutput);
#endif
    

    D3D11CreateDevice(
	adapter,
	D3D_DRIVER_TYPE_UNKNOWN,
	0,
	deviceFlags,
	levels,
	ArrayCount(levels),
	D3D11_SDK_VERSION,
	&d3dDevice,
	&featureLevel,
	&context);
	
    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    windowClass.lpfnWndProc = Win32MainWindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.lpszClassName = "Dx11Test";


    if (RegisterClass(&windowClass))
    {
	i32 x = CW_USEDEFAULT;
	i32 y = CW_USEDEFAULT;

	i32 nDefaultWidth = screenW;
	i32 nDefaultHeight = screenH;

	RECT rect = {};

	
	SetRect(&rect, 0, 0, nDefaultWidth, nDefaultHeight);

	AdjustWindowRect(&rect,
			 WS_OVERLAPPEDWINDOW,
			 false);

	i32 windowWidth = rect.right - rect.left;
	i32 windowHeight = rect.bottom - rect.top;

	HWND windowHandle = CreateWindow(
	    windowClass.lpszClassName,
	    "DX11 Test",
	    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	    x, y,
	    windowWidth, windowHeight,
	    0,
	    0,
	    hInstance,
	    0);


	DWORD lastError = GetLastError();
	if (windowHandle)
	{
	    //Now that we have a window to draw in and an interface to send data and give commands to the GPU,
	    //we create the swap chain
	    i32 monitorRefreshRate = 60;
	    
	    HDC refreshDC = GetDC(windowHandle);
	    i32 win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
	    ReleaseDC(windowHandle, refreshDC);
	    if (win32RefreshRate > 1)
	    {
		monitorRefreshRate = win32RefreshRate;
	    }
	    r32 gameUpdateHz = (monitorRefreshRate / 2.0f);
	    r32 targetSecondsPerFrame = 1.0f / (r32)gameUpdateHz;
	    

	    DXGI_SWAP_CHAIN_DESC1 desc = {};
	    desc.BufferCount = 2;
	    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	    desc.SampleDesc.Count = 1;
	    desc.SampleDesc.Quality = 0;
	    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	    desc.Width = windowWidth;
	    desc.Height = windowHeight;
	    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;



	    hr = factory->CreateSwapChainForHwnd(
		d3dDevice,
		windowHandle,
		&desc,
		0,
		0,
		&swapChain);

	    //use this bc you are repeating code
	    ConfigureBackBuffer();

	    
	    //Getting the back buffer from the swap chain (since we defined DXGI_USAGE_RENDER_TARGET_OUTPUT)

	    shaders shaders = {};
	    cube_buffers cubeBuffer = {};
	    direct_x_loaded_buffers loadedBuffers = {};
	    
	    CreateDeviceDependentResources(&shaders, &loadedBuffers, &programState->setupArena, &memory);


	    running = true;

	    
	    dx_camera camera = {};	    

	    CreateViewAndPerspective(&camera);

	    

	    //GameCode loading
	    char* sourceDLLName = "game_layer.dll";
	    win32_game_code game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath, gameCodeLockFullPath);

//(memory_arena* objLocationArena, program_memory* mainProgramMemory, obj* allGameObjects, i32* numOfGameObjects)

	    obj* allGameObjects = 0;
	    i32 numOfGameObjects = 0;
	    game_initialize_data initializedData;
	    win32_voxel_chunk win32VoxelChunk = {};	    
	    initializedData = game.GameInitialize(&memoryPoolCode, &programState->setupArena, &programState->renderedVoxelIndexArena, &memory, &numOfGameObjects, &parseObjCode, &programState->debugVectorArena);

	    win32VoxelChunk.chunk = &initializedData.chunk;
	    
	    u32 loadCounter = 120;


	    game_state* gameState = initializedData.gameState;

	    gameState->windowW = windowWidth;
	    gameState->windowH = windowHeight;
	    /* MAIN GAME LOOP */
	    /* MAIN GAME LOOP */
	    /* MAIN GAME LOOP */

	    //Setting up controls and such
	    game_input input[2] = {};
	    game_input* newInput = &input[0];
	    game_input* oldInput = &input[1];


	    
	    Win32InitVoxelGrid(&win32VoxelChunk, &programState->setupArena, &programState->indexBufferArena,  &win32State);
	    win32_debug_vectors debugVectors = {};
	    Win32InitAllDebugVectors(&debugVectors, &programState->setupArena, &programState->debugVectorArena, &win32State);
	    
	    ShowCursor(false);

	    RAWINPUTDEVICE rid[1];
	    rid[0].usUsagePage = 0x01;
	    rid[0].usUsage = 0x02;
	    rid[0].dwFlags = RIDEV_NOLEGACY;
	    rid[0].hwndTarget = windowHandle;

	    if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE)
	    {
		lastError = GetLastError();
	    }

	    freeCam = true;
	    
	    r32 lastTime = 0.0f;

	    LARGE_INTEGER lastCounter = Win32GetWallClock();
	    u64 lastCycleCount = __rdtsc();
	    LARGE_INTEGER flipWallClock = Win32GetWallClock();

	    

	    mouse_movements arcMouse = {};

	    bool32 arcCamInitialized = false;
	    
	    while(running)
	    {

		FILETIME newDLLWriteTime = Win32GetLastWriteTime(sourceGameCodeDLLFullPath);
		if (CompareFileTime(&newDLLWriteTime, &game.dllLastWriteTime) != 0)
		{
		    Win32UnloadGameCode(&game);
		    game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath, gameCodeLockFullPath);
		    loadCounter = 0;
		}

		POINT mouseP;
		GetCursorPos(&mouseP);
		ScreenToClient(windowHandle, &mouseP);
		newInput->mouseXBounded = mouseP.x;
		newInput->mouseYBounded = mouseP.y;
		newInput->mouseZBounded = 0;

#if 1
		char mouseBuffer[256];
		sprintf_s(mouseBuffer, sizeof(mouseBuffer), "Mouse Values in Win32: X: %f, Y: %f\n",
			  (r32)mouseP.x, (r32)mouseP.y);
		OutputDebugString(mouseBuffer);
#endif		
		
#if 0
		LARGE_INTEGER wallClockTime = Win32GetWallClock();

		r32 now = (r32)(wallClockTime.QuadPart);
		
		r32 deltaTime = now - lastTime;
		lastTime = now;
#else
		r32 deltaTime = targetSecondsPerFrame;

#endif
		DWORD maxControllerCount = XUSER_MAX_COUNT;

		game_controller_input* oldKeyboardController = GetController(oldInput, 0);
		game_controller_input* newKeyboardController = GetController(newInput, 0);

		*newKeyboardController = {};
		newKeyboardController->isConnected = true;
		for (i32 buttonIndex  = 0; buttonIndex < ArrayCount(newKeyboardController->buttons); ++buttonIndex)
		{
		    newKeyboardController->buttons[buttonIndex].endedDown =
			oldKeyboardController->buttons[buttonIndex].endedDown;
		};


		mouse_movements mouse = {};


		//At some point please move this in the process pending messages, rn I don't wanna go back to
		//debugging it so I'm leaving it for future me.... Thanks future me!

		Win32SetMouseStates(newInput, oldInput);

		
		Win32ProcessPendingMessages(newKeyboardController, oldKeyboardController, &camera, &mouse, newInput, oldInput);

		r32 xChange = deltaTime * (0.3f * mouse.x);
		r32 yChange = deltaTime * (0.3f * mouse.y);		


		if (freeCam)
		{
		    ProcessMouseControlFP(&camera, -xChange, -yChange);
		    ProcessPlayerMovement(newKeyboardController, &camera, deltaTime);		    
		    UpdateCameraFP(&camera, gameState);
		}
		else
		{
		    if (!arcCamInitialized)
		    {
			InitArcBall(&camera, &win32VoxelChunk);
			arcCamInitialized = true;
		    }
		    UpdateCameraArc(&camera, &arcMouse, newInput, gameState);

		    //Debug vectors start here
		}


		//GameUpdate
		UpdateGameStateInfo(gameState, &camera);
		game.GameUpdate(&memory, newInput, gameState, &memoryPoolCode, &programState->debugVectorArena, &initializedData.chunk);

		//Convert necessary game related data to win32 specific data

		RenderVoxelCubes(&shaders, &camera, &win32VoxelChunk, &win32State);
		RenderDebug(&shaders, &debugVectors, gameState, &camera);
		swapChain->Present(1, 0);


		game_input* temp = newInput;
		newInput = oldInput;
		oldInput = temp;


		memoryPoolCode.ClearArena(&programState->perFrameArena);
	    } //Loop Bracket
	}
    }
    return(0);
}
