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



struct shader_info
{
    i32 temp;
};

struct program_state
{
    memory_arena setupArena;
    memory_arena perFrameArena;

    u8* arenaBase;
    
    shader_info* shaderInfo;
};

struct direct_x_loaded_buffers
{
    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;
    ID3D11Buffer* vertInstanceBuffer;
    i32 indexCount;
    i32 instanceCount;
};

struct voxel_constants
{
    DirectX::XMFLOAT4 worldPos;
};

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



#include "game_layer.h"

global_variable ID3D11DeviceContext* context;
global_variable ID3D11Device* d3dDevice;
global_variable WINDOWPLACEMENT windowPosition = {sizeof(windowPosition)};
global_variable IDXGISwapChain1* swapChain;
global_variable ID3D11RenderTargetView* renderTarget;
global_variable ID3D11DepthStencilView* depthStencilView;
global_variable ID3D11Texture2D* depthStencil;
global_variable D3D11_TEXTURE2D_DESC bbDesc;


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
        
struct r32_3
{
    r32 x;
    r32 y;
    r32 z;    
};

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
struct shaders
{
    ID3D11VertexShader* vertexShader;
    ID3D11InputLayout* inputLayout;
    ID3D11PixelShader* pixelShader;
    ID3D11Buffer* constantBuffer;
    ID3D11Buffer* instanceBuffer;
};

struct dx_instance_data
{
    DirectX::XMFLOAT3 pos;
};



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

struct obj_conversion
{
    vertex_position_color* objVerts;
    u16* indices;
    u32 objVertsSize;
    u32 indexCount;
};

internal obj_conversion
ConvertGameOBJToDXOBJ(obj* currObj, memory_arena* arena)
{
    obj_conversion result;
    HRESULT hr = {};

//At some point you will loop all of these depending on the loaded objects in the game, but rn you only need to load
    //one so I'm loading one

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
CreateViewAndPerspective(dx_camera* camera)
{
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.7f, 1.5f, 0.f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, -0.1f, 0.0f, 0.f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);

    r32 aspectRatioX = GetAspectRatio();
    r32 aspectRatioY = aspectRatioX < (16.0f / 9.0f) ? aspectRatioX / (16.0f / 9.0f) : 1.0f;


    camera->up = up;
    camera->worldUp = up;
    camera->yaw = -90.0f;
    camera->pitch = 0.0f;
    camera->front = {0.0f, 0.0f, -1.0f, 0.0f};
    camera->movementSpeed = 5.0f;
    camera->turnSpeed = 0.2f;
    camera->position = {10.0f, 10.0f, 10.0f};
    
    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.view,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixLookAtRH(
		eye,
		at,
		up)
	    )
	);
    

    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.projection,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixPerspectiveFovRH(
		2.0f * (r32)(atan(tan(DirectX::XMConvertToRadians(70) * 0.5f)) / aspectRatioY),
		aspectRatioX,
		0.01f,
		100.0f)
	    )
	);


}

internal void
CreateWindowSizeDependentResources(dx_camera* camera)
{
    CreateViewAndPerspective(camera);
}

internal void
UpdateCamera(dx_camera* camera)
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
}

internal void
ProcessMouseControl(dx_camera* camera, r32 xChange, r32 yChange)
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

#if 0    
    char textBuffer[256];
    
#if 0
    sprintf_s(textBuffer, sizeof(textBuffer),
	      "X: %f, Y: %f, Z: %f\n",
	      DirectX::XMVectorGetX(camera->front),
	      DirectX::XMVectorGetY(camera->front),
	      DirectX::XMVectorGetZ(camera->front));
    OutputDebugString(textBuffer);
    
    OutputDebugString(textBuffer);
#else
    
    sprintf_s(textBuffer, sizeof(textBuffer),
	      "X: %f, Y: %f, Z: %f\n",
	      DirectX::XMVectorGetX(camera->position),
	      DirectX::XMVectorGetY(camera->position),
	      DirectX::XMVectorGetZ(camera->position));
    OutputDebugString(textBuffer);

#endif    
#endif    
}

internal void
ProcessPlayerMovement(game_controller_input* controller, dx_camera* camera, r32 deltaTime)
{
    //Now comes the hard part...

    r32 velocity = camera->movementSpeed * deltaTime;
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

    if (oldState->endedDown)
    {
	newState->started = false;
    }
}

struct mouse_movements
{
    r32 x, y;
};

internal void
Win32ProcessPendingMessages(game_controller_input* keyboardController, game_controller_input* oldKeyboardController, dx_camera* camera, mouse_movements* mouse)
{
    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
	switch(msg.message)
	{
	case WM_QUIT:
	{
	    running = false;
	} break;
	case WM_SIZE:
	{
	    CreateWindowSizeDependentResources(camera);
	} break;
	case WM_INPUT:
	{
	    UINT dwSize = 0;
	    
	    
	    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	    LPBYTE lpb = (LPBYTE)memoryPoolCode.PushStruct(&programState->perFrameArena, (sizeof(BYTE)) * dwSize);
	    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

	    RAWINPUT* raw = (RAWINPUT*)lpb;
	    if (raw->header.dwType == RIM_TYPEMOUSE)
	    {
		mouse->x = (r32)raw->data.mouse.lLastX;
		mouse->y = (r32)raw->data.mouse.lLastY;
	    }
	    
	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_KEYDOWN:
	{
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
		else if (VKCode == VK_ESCAPE)
		{
		    running = false; 
		}
		else if (VKCode == 'F')
		{
		    ToggleFullscreen(msg.hwnd);
		}

		
		
#if 0
		if (isDown)
		{
		    bool32 altKeyWasDown = ((msg.lParam & (1 << 29)) != 0);
		    if ((VKCode == VK_RETURN) && altKeyWasDown)
		    {
			OutputDebugString("Hitting here?\n");
			if (msg.hwnd)
			{
			    ToggleFullscreen(msg.hwnd);
			}
		    }
		}
#endif		
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
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

struct shader_code
{
    ID3DBlob* vertexShaderCode;
    ID3DBlob* pixelShaderCode;    
};

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
    HRESULT hr = {};
    
    FILE* vShader, *pShader; //vertex (v) pixel (p)
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
				  &shaderResources->vertexShader);



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
	&shaderResources->inputLayout
	);

    

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
	&shaderResources->constantBuffer);
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

    ID3D11Buffer* voxelCB;
};

internal void
Win32InitVoxelGrid(win32_voxel_chunk* win32VoxelChunk, memory_arena* arena)
{
    HRESULT hr = {};
    
    i32 numOfRenderedVoxels = win32VoxelChunk->chunk->numOfRenderedVoxels;
    win32VoxelChunk->indexBuffers = (ID3D11Buffer**)memoryPoolCode.PushArraySized(arena,
										  (size_t)(sizeof(ID3D11Buffer*) * numOfRenderedVoxels));


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
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(voxel_constants);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    
    hr = d3dDevice->CreateBuffer(&cbDesc, NULL, &win32VoxelChunk->voxelCB);
    
    //You'll also want to probably store this on a specific arena that can be wiped whenever the chunk becomes dirty

    for (i32 i = 0; i < numOfRenderedVoxels; i++)
    {
	voxel* currVoxel = &win32VoxelChunk->chunk->voxels[win32VoxelChunk->chunk->renderedVoxelIndex[i]];

	v3 voxelPos = currVoxel->pos;

	if (currVoxel->renderedIndiceCount > 36)
	{
	    i32 foo = 0;
	}
	
	CD3D11_BUFFER_DESC indexDesc(
	    sizeof(u16) * currVoxel->renderedIndiceCount,
	    D3D11_BIND_INDEX_BUFFER);

	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
	indexData.pSysMem = currVoxel->renderedIndices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	hr = d3dDevice->CreateBuffer(
	    &indexDesc,
	    &indexData,
	    &win32VoxelChunk->indexBuffers[i]);
	
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
RenderVoxelCubes(shaders* shader, dx_camera* camera, win32_voxel_chunk* win32VoxelChunk)
{
    //Start rendering the voxels so we can see what is going on with the cubes being rendered
//Check the speeds here and where it could be casuing issues


    r32 teal [] = {0.098f, 0.439f, 0.439f, 1.000f};

    context->UpdateSubresource(shader->constantBuffer, 0, nullptr, &camera->constantBufferData, 0, 0);

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
    context->IASetInputLayout(shader->inputLayout);
    context->VSSetConstantBuffers(0, 1, &shader->constantBuffer);

    UINT stride = sizeof(vertex_position_color);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &win32VoxelChunk->vertexBuffers, &stride, &offset);    




    context->VSSetShader(
	shader->vertexShader,
	nullptr,
	0);

    context->PSSetShader(
	shader->pixelShader,
	nullptr,
	0);


   
    HRESULT hr = {};

    for (int i = 0; i < win32VoxelChunk->chunk->numOfRenderedVoxels; i++)
    {

	context->VSSetConstantBuffers(1, 1, &win32VoxelChunk->voxelCB);

	context->IASetIndexBuffer(win32VoxelChunk->indexBuffers[i], DXGI_FORMAT_R16_UINT, 0);

	
	//We are getting the constant buffer on the GPU and using for the CPU
	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = context->Map(win32VoxelChunk->voxelCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	voxel_constants* data = (voxel_constants*)mapped.pData;

	voxel* currVoxel = &win32VoxelChunk->chunk->voxels[win32VoxelChunk->chunk->renderedVoxelIndex[i]];

	DirectX::XMVECTOR vecWorld = DirectX::XMVectorSet(currVoxel->pos.x, currVoxel->pos.y, currVoxel->pos.z, 1.0f);
	DirectX::XMStoreFloat4(&data->worldPos, vecWorld);


	context->Unmap(win32VoxelChunk->voxelCB, 0);
	
	context->DrawIndexed(
	    currVoxel->renderedIndiceCount,
	    0,
	    0);

    }
}

internal void
Render(ID3D11Buffer* constantBuffer, shaders* shader, direct_x_loaded_buffers* loadedBuffers, dx_camera* camera)
{

    context->UpdateSubresource(
	shader->constantBuffer,
	0,
	nullptr,
	&camera->constantBufferData,
	0,
	0);    

    //Clear the render target and z buffer
    r32 teal [] = {0.098f, 0.439f, 0.439f, 1.000f};
    context->ClearRenderTargetView(
	renderTarget,
	teal);

    context->ClearDepthStencilView(
	depthStencilView,
	D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
	1.0f,
	0);

    //Set the render target
    context->OMSetRenderTargets(
	1,
	&renderTarget,
	depthStencilView);

    //Set the IA stage by setting the input topology and layout

    UINT strides[2] = {sizeof(vertex_position_color), sizeof(inst_buffer_struct)};
    UINT offsets[2] = {0, 0};

    ID3D11Buffer* vertInstBuffers[2] = {loadedBuffers->vertexBuffer, shader->instanceBuffer};
    
    context->IASetVertexBuffers(
	0,
	2,
	vertInstBuffers,
	strides,
	offsets);    


    context->IASetIndexBuffer(
	loadedBuffers->indexBuffer,
	DXGI_FORMAT_R16_UINT,
	0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(shader->inputLayout);

    //Set up vertex shader stage 
    context->VSSetShader(
	shader->vertexShader,
	nullptr,
	0);


    context->VSSetConstantBuffers(
	0,
	1,
	&shader->constantBuffer);    


    //Setup the pixel shader stage
    context->PSSetShader(
	shader->pixelShader,
	nullptr,
	0);

    //Calling draw tells d3d to start sending commands to the graphics device


    //Draw each cube from the voxel grids
    //Each cube needs a buffer

    
    
    context->DrawIndexedInstanced(
	loadedBuffers->indexCount,
	loadedBuffers->instanceCount,
	0,
	0,
	0);

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

    HMODULE memoryPoolLibrary = LoadLibrary("D:/ExternalCustomAPIs/MemoryPools/dll/memory_pools.dll");

    if (memoryPoolLibrary)
    {
	memoryPoolCode.PushStruct = (memory_pool_push_struct*)GetProcAddress(memoryPoolLibrary, "PushStruct");
	memoryPoolCode.PushArray = (memory_pool_push_array*)GetProcAddress(memoryPoolLibrary, "PushArray");
	memoryPoolCode.PoolAlloc = (memory_pool_alloc*)GetProcAddress(memoryPoolLibrary, "PoolAlloc");
	memoryPoolCode.InitializeArena = (memory_pool_initialize_arena*)GetProcAddress(memoryPoolLibrary, "InitializeArena");
	memoryPoolCode.ClearArena = (memory_pool_clear_arena*)GetProcAddress(memoryPoolLibrary, "ClearArena");
	memoryPoolCode.PushArraySized = (memory_pool_push_array_sized*)GetProcAddress(memoryPoolLibrary, "PushArraySized");
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

    memoryPoolCode.InitializeArena(&programState->setupArena, memory.permanentStorageSize - sizeof(program_state),
				   (u8*)memory.permanentStorage + sizeof(program_state));

    memoryPoolCode.InitializeArena(&programState->perFrameArena, memory.transientStorageSize - sizeof(program_state),
				   (u8*)memory.transientStorage + sizeof(program_state));

    programState->shaderInfo = (shader_info*)memoryPoolCode.PushStruct(&programState->setupArena, sizeof(programState->shaderInfo));

    

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

	i32 nDefaultWidth = 640;
	i32 nDefaultHeight = 480;

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

	    //GameInitialize();
	    //LoadOBJs();
	    


	    	    
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
	    initializedData = game.GameInitialize(&memoryPoolCode, &programState->setupArena, &memory, &numOfGameObjects, &parseObjCode);

	    win32VoxelChunk.chunk = &initializedData.chunk;
	    
	    u32 loadCounter = 120;

	    /* MAIN GAME LOOP */
	    /* MAIN GAME LOOP */
	    /* MAIN GAME LOOP */

	    //Setting up controls and such
	    game_input input[2] = {};
	    game_input* newInput = &input[0];
	    game_input* oldInput = &input[1];


	    
	    Win32InitVoxelGrid(&win32VoxelChunk, &programState->setupArena);

	    ShowCursor(false);
	    
	    RAWINPUTDEVICE rid[1];
	    rid[0].usUsagePage = 0x01;
	    rid[0].usUsage = 0x02;
	    rid[0].dwFlags = RIDEV_NOLEGACY;
	    rid[0].hwndTarget = 0;

	    if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE)
	    {
		lastError = GetLastError();
	    }
	    
	    
	    r32 lastTime = 0.0f;


	    
//Why tf is this here???
#if 0
	    DirectX::XMStoreFloat4x4(
		&camera.constantBufferData.world,
		DirectX::XMMatrixTranspose(
		    DirectX::XMMatrixRotationY(
			DirectX::XMConvertToRadians(
			    (r32)frameCount++
			    )
			)
		    )
		);
#endif



	    LARGE_INTEGER lastCounter = Win32GetWallClock();
	    u64 lastCycleCount = __rdtsc();
	    LARGE_INTEGER flipWallClock = Win32GetWallClock();
	    
	    while(running)
	    {

		FILETIME newDLLWriteTime = Win32GetLastWriteTime(sourceGameCodeDLLFullPath);
		if (CompareFileTime(&newDLLWriteTime, &game.dllLastWriteTime) != 0)
		{
		    Win32UnloadGameCode(&game);
		    game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath, gameCodeLockFullPath);
		    loadCounter = 0;
		}
		
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
		
		Win32ProcessPendingMessages(newKeyboardController, oldKeyboardController, &camera, &mouse);


		r32 xChange = deltaTime * (0.3f * mouse.x);
		r32 yChange = deltaTime * (0.3f * mouse.y);
		



		ProcessMouseControl(&camera, -xChange, -yChange);
		ProcessPlayerMovement(newKeyboardController, &camera, deltaTime);		
		UpdateCamera(&camera);		



#if 0		
		char textBuffer[256];
		sprintf_s(textBuffer, sizeof(textBuffer),
			  "xChange: %f, yChange: %f\n", xChange, yChange);
		OutputDebugString(textBuffer);
#endif		



		
//		Render(shaders.constantBuffer, &shaders, &loadedBuffers, &camera);
		RenderVoxelCubes(&shaders, &camera, &win32VoxelChunk);
		swapChain->Present(1, 0);

		//Spinner to slow down to a locked fps for testing purposes, also bc it's running too fast when
		//there isn't much for the computer to do atm

#if 0	
		LARGE_INTEGER workCounter = Win32GetWallClock();
		r32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);
		r32 secondsElapsedForFrame = workSecondsElapsed;
		if (secondsElapsedForFrame < targetSecondsPerFrame)
		{
		    DWORD sleepMs = 0;
		    while (secondsElapsedForFrame < targetSecondsPerFrame)
		    {
			if (sleepIsGranular)
			{
			    sleepMs = (DWORD) (1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
			    if (sleepMs > 0)
			    {
				Sleep(sleepMs - 1);
			    }
			}
			secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());

		    }
		    r32 testSecondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
		    if (testSecondsElapsedForFrame < targetSecondsPerFrame)
		    {
			//LOG THAT WE MISSSED SLEEP 
		    }
		}
		else
		{
		    //If secondsElapsedForFrame is higher than the target, then we missed a frame
		}
#endif

		game_input* temp = newInput;
		newInput = oldInput;
		oldInput = temp;


		memoryPoolCode.ClearArena(&programState->perFrameArena);
	    } //Loop Bracket
	}
    }

    //ID3D11Device is just the set of functions which you call infrequently at the beginning of the program
    //to acquire and configure the set of resources needed to start drawing pixels

    //ID3D11DeviceContext contains the methods we will call every frame, loading in buffers and views and other
    //resources

    return(0);
}
