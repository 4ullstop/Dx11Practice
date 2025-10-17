
#include <windows.h>
#include <stdio.h>
#include <math.h>

#include <DirectXMath.h>


#include "D:/ExternalCustomAPIs/MemoryPools/code/memory_pool_dll_include.h"

//If you separate the program out in the future the file_reader.h file can be included in the separated dll file
//and the file_reader.cpp should stay here in our win32 implementation

#include "D:/ExternalCustomAPIs/FileReader/file_reader.h"
#include "D:/ExternalCustomAPIs/FileReader/file_reader.cpp"

#include "D:/ExternalCustomAPIs/OBJLoader/code/directx_obj_loader_dll_include.h"


#include "D:/ExternalCustomAPIs/Types/typedefs.h"
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include "win32_dx11.h"
#include "game_layer.h"


#include <xinput.h>


struct shader_info
{
    i32 temp;
};

struct program_state
{
    memory_arena setupArena;
    
    shader_info* shaderInfo;
};


global_variable memory_pool_dll_code memoryPoolCode;
global_variable direct_x_load_obj_code directXOBJCode;
global_variable bool32 running;
global_variable program_state* programState;
global_variable ID3D11Texture2D* backBuffer;
global_variable u32 frameCount;
global_variable i64 perfCountFrequency;
//global_variable constant_buffer_struct constantBufferData;


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
    D3D11_TEXTURE2D_DESC bbDesc;
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

#if 0    
    DirectX::XMStoreFloat4x4(
	&constantBufferData.view,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixLookAtRH(
		eye,
		at,
		up)
	    )
	);

    DirectX::XMStoreFloat4x4(
	&constantBufferData.projection,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixPerspectiveFovRH(
		2.0f * (r32)(atan(tan(DirectX::XMConvertToRadians(70) * 0.5f)) / aspectRatioY),
		aspectRatioX,
		0.01f,
		100.0f)
	    )
	);    
#else
    camera->up = up;
    camera->yaw = 0.0f;
    camera->pitch = 0.0f;
    camera->movementSpeed = 5.0f;
    camera->turnSpeed = 0.2f;
    
    DirectX::XMStoreFloat4x4(
	&camera->constantBufferData.view,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixLookAtRH(
		eye,
		at,
		up)
	    )
	);
    
    r32 aspectRatioX = GetAspectRatio();
    r32 aspectRatioY = aspectRatioX < (16.0f / 9.0f) ? aspectRatioX / (16.0f / 9.0f) : 1.0f;

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
#endif    

}

internal void
CreateWindowSizeDependentResources(dx_camera* camera)
{
    CreateViewAndPerspective(camera);
}

internal void
UpdateCamera(dx_camera* camera)
{
    //Set front.x
    DirectX::XMVectorSetX(camera->front, (r32)(cos(ToRadians(camera->yaw) * ToRadians(camera->pitch))));
    //Set front.y
    DirectX::XMVectorSetY(camera->front, (r32)(sin(ToRadians(camera->pitch))));
    //Set front.z
    DirectX::XMVectorSetZ(camera->front, (r32)(cos(ToRadians(camera->pitch) * sin(ToRadians(camera->yaw)))));

    //Normalize the magnitude
    camera->front = DirectX::XMVector4Normalize(camera->front);

    camera->right = DirectX::XMVector4Normalize(DirectX::XMVector4Cross(camera->front, camera->worldUp));
    camera->up = DirectX::XMVector4Normalize(DirectX::XMVector4Cross(camera->right, camera->front));
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
    
}

internal void
ProcessPlayerMovement(game_controller_input* controller, dx_camera* camera, r32 deltaTime)
{
    //Now comes the hard part...

    r32 velocity = camera->movementSpeed * deltaTime;
    if (controller->moveUp.endedDown)
    {
	camera->position = DirectX::XMVectorAdd(camera->position, DirectX::XMVectorScale(camera->front, velocity));
    }
    if (controller->moveDown.endedDown)
    {
	camera->position = DirectX::XMVectorSubtract(camera->position, DirectX::XMVectorScale(camera->front, velocity));
    }
    if (controller->moveRight.endedDown)
    {
	camera->position = DirectX::XMVectorSubtract(camera->position, DirectX::XMVectorScale(camera->right, velocity));
    }
    if (controller->moveLeft.endedDown)
    {
	camera->position = DirectX::XMVectorAdd(camera->position, DirectX::XMVectorScale(camera->right, velocity));
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

internal void
Win32ProcessPendingMessages(game_controller_input* keyboardController, game_controller_input* oldKeyboardController, dx_camera* camera)
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
		    Win32ProcessKeyboardMessage(&keyboardController->moveUp,
						&oldKeyboardController->moveUp, isDown);
		}
		if (VKCode == 'A')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveLeft,
						&oldKeyboardController->moveLeft, isDown);
		}
		if (VKCode == 'S')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveDown,
						&oldKeyboardController->moveDown, isDown);
		}
		if (VKCode == 'D')
		{
		    Win32ProcessKeyboardMessage(&keyboardController->moveRight,
						&oldKeyboardController->moveRight, isDown);
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
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

struct shaders
{
    ID3D11VertexShader* vertexShader;
    ID3D11InputLayout* inputLayout;
    ID3D11PixelShader* pixelShader;
    ID3D11Buffer* constantBuffer;
};

internal void
CreateShaders(ID3D11Device* device, shaders* shaderResources)
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
    hr = device->CreateVertexShader(fileResult.contents,
				  fileResult.contentsSize,
				  nullptr,
				  &shaderResources->vertexShader);



    //This is the descriptor for the vertex data layout since d3d doesn't define one
    //This project could include more than 2 elements in the struct but we would have to modify the description below
    //to fit the struct
    D3D11_INPUT_ELEMENT_DESC iaDesc[] =
    {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
	  0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},

	{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
	  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };


    hr = device->CreateInputLayout(
	iaDesc,
	ArrayCount(iaDesc),
	bytes,
	fileResult.contentsSize,
	&shaderResources->inputLayout
	);

    

    debug_read_file_result pixelShaderResult = DEBUGPlatformReadEntireFile(&blankThread, "../build/CubePixelShader.cso");
    
    bytes = (BYTE*)pixelShaderResult.contents;
    hr = device->CreatePixelShader(
	pixelShaderResult.contents,
	pixelShaderResult.contentsSize,
	nullptr,
	&shaderResources->pixelShader);

    CD3D11_BUFFER_DESC cbDesc(
	sizeof(constant_buffer_struct),
	D3D11_BIND_CONSTANT_BUFFER);

    hr = device->CreateBuffer(
	&cbDesc,
	nullptr,
	&shaderResources->constantBuffer);
}


internal void
CreateCube(ID3D11Device* device, cube_buffers* cubeBuffer)
{
    cube_buffers result = {};

    HRESULT hr = S_OK;

    //Cube geo
    vertex_position_color CubeVertices[] =
    {
	{DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(0, 0, 0),},
	{DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT3(0, 0, 1),},
	{DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f), DirectX::XMFLOAT3(0, 1, 0),},
	{DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f), DirectX::XMFLOAT3(0, 1, 1),},
	{DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f), DirectX::XMFLOAT3(1, 0, 0),},
	{DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT3(1, 0, 1),},
	{DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT3(1, 1, 0),},
	{DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), DirectX::XMFLOAT3(1, 1, 1),},
    };

    //Create the vertex buffer
    CD3D11_BUFFER_DESC vDesc(
	sizeof(CubeVertices),
	D3D11_BIND_VERTEX_BUFFER);

    D3D11_SUBRESOURCE_DATA vData;
    ZeroMemory(&vData, sizeof(D3D11_SUBRESOURCE_DATA));
    vData.pSysMem = CubeVertices;
    vData.SysMemPitch = 0;
    vData.SysMemSlicePitch = 0;

    hr = device->CreateBuffer(
	&vDesc,
	&vData,
	&cubeBuffer->vertexBuffer);

    //For fun make a file loader that loads and obj file, doesn't have to be great atm, make it better later

    //For fun he says... make a file loader for fun he says...
    
    unsigned short cubeIndices[] =
    {
	0,2,1,
	1,2,3,

	4,5,6,
	5,7,6,

	0,1,5,
	0,5,4,
	
	2,6,7,
	2,7,3,

	0,4,6,
	0,6,2,

	1,3,7,
	1,7,5,
    };


    //Read over why we are doing this again...
    cubeBuffer->indexCount = ArrayCount(cubeIndices);
    CD3D11_BUFFER_DESC iDesc(
	sizeof(cubeIndices),
	D3D11_BIND_INDEX_BUFFER);

    D3D11_SUBRESOURCE_DATA iData;
    ZeroMemory(&iData, sizeof(D3D11_SUBRESOURCE_DATA));
    iData.pSysMem = cubeIndices;
    iData.SysMemPitch = 0;
    iData.SysMemSlicePitch = 0;

    hr = device->CreateBuffer(
	&iDesc,
	&iData,
	&cubeBuffer->indexBuffer);
}

internal void
Update(void)
{

    //Simply rotates the cube once per frame
#if 0    
    DirectX::XMStoreFloat4x4(
	&constantBufferData.world,
	DirectX::XMMatrixTranspose(
	    DirectX::XMMatrixRotationY(
		DirectX::XMConvertToRadians(
		    (r32)frameCount++
		    )
		)
	    )
	);

    if (frameCount == MAXUINT) frameCount = 0;
#endif    
}

//NOTE: this function should be called asynchronously, Take the time to have it execute
//on a separate thread
internal void
CreateDeviceDependentResources(ID3D11Device* device, shaders* shaders, direct_x_loaded_buffers* loadedBuffers, memory_arena* mainArena, program_memory* programMemory)
{

    CreateShaders(device, shaders);

#if 0     
    CreateCube(device, cubeBuffer);
#else

#if 0
    directXOBJCode.DirectXLoadOBJ("D:/ExternalCustomAPIs/OBJLoader/misc/cubetester_normals.obj", mainArena, programMemory, device, loadedBuffers);
#else
    directXOBJCode.DirectXLoadOBJ("D:/ExternalCustomAPIs/OBJLoader/misc/monkey.obj", mainArena, programMemory, device, loadedBuffers);
#endif    
#endif    
}

internal void
Render(ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencil, ID3D11Buffer* constantBuffer, shaders* shader, direct_x_loaded_buffers* loadedBuffers, dx_camera* camera)
{
#if 0    
    context->UpdateSubresource(
	shader->constantBuffer,
	0,
	nullptr,
	&constantBufferData,
	0,
	0);
#else
    context->UpdateSubresource(
	shader->constantBuffer,
	0,
	nullptr,
	&camera->constantBufferData,
	0,
	0);
#endif    
    //Clear the render target and z buffer
    r32 teal [] = {0.098f, 0.439f, 0.439f, 1.000f};
    context->ClearRenderTargetView(
	renderTarget,
	teal);

    context->ClearDepthStencilView(
	depthStencil,
	D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
	1.0f,
	0);

    //Set the render target
    context->OMSetRenderTargets(
	1,
	&renderTarget,
	depthStencil);

    //Set the IA stage by setting the input topology and layout
    UINT stride = sizeof(vertex_position_color);
    UINT offset = 0;

    context->IASetVertexBuffers(
	0,
	1,
	&loadedBuffers->vertexBuffer,
	&stride,
	&offset);

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

    context->DrawIndexed(
	loadedBuffers->indexCount,
	0,
	0);

}

int CALLBACK WinMain(HINSTANCE hInstance,
		     HINSTANCE hPrevInstance,
		     LPSTR lpCmdLine,
		     int nCmdShow)
{
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
    }
    if (memoryPoolCode.PushStruct && memoryPoolCode.PushArray && memoryPoolCode.PoolAlloc)
    {
	OutputDebugString("Memory Pool Code Successfully Loaded");
    }

    HMODULE directXOBJLibrary = LoadLibrary("D:/ExternalCustomAPIs/OBJLoader/dll/directx_obj_loader.dll");
    if (directXOBJLibrary)
    {
	directXOBJCode.DirectXLoadOBJ = (direct_x_load_obj*)GetProcAddress(directXOBJLibrary, "DirectXLoadOBJ");
    }
    
    program_memory memory = {};


    memory.transientStorageSize = Megabytes(64);
    memory.permanentStorageSize = Gigabytes(1);
    
    memoryPoolCode.PoolAlloc(0, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE, &memory);

    programState = (program_state*)memory.permanentStorage;    
    
    memoryPoolCode.InitializeArena(&programState->setupArena, memory.permanentStorageSize - sizeof(program_state),
				   (u8*)memory.permanentStorage + sizeof(program_state));

    programState->shaderInfo = (shader_info*)memoryPoolCode.PushStruct(&programState->setupArena, sizeof(programState->shaderInfo));

    

    
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
    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_create_device_flag
#if TEST_INTERNAL
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif    
    
    ID3D11Device* d3dDevice = {};
    ID3D11DeviceContext* context;

    D3D_FEATURE_LEVEL featureLevel;



    IDXGIAdapter* adapter = NULL;
    IDXGIFactory2* factory = 0;

    HR(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), (void**)&factory));

 
    factory->EnumAdapters(1, &adapter);

    IDXGIOutput* adapterOutput = {};
    adapter->EnumOutputs(1, &adapterOutput);
    
    
    HRESULT hr = 
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


	    DXGI_SWAP_CHAIN_DESC1 desc = {};
	    desc.BufferCount = 2;
	    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	    desc.SampleDesc.Count = 1;
	    desc.SampleDesc.Quality = 0;
	    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	    desc.Width = windowWidth;
	    desc.Height = windowHeight;


	    IDXGISwapChain1* swapChain = 0;

	    ID3D11RenderTargetView* renderTarget = 0;

	    D3D11_TEXTURE2D_DESC bbDesc;

	    ID3D11Texture2D* depthStencil;
	    ID3D11DepthStencilView* depthStencilView;
	    



	    hr = factory->CreateSwapChainForHwnd(
		d3dDevice,
		windowHandle,
		&desc,
		0,
		0,
		&swapChain);

	    
	    //Getting the back buffer from the swap chain (since we defined DXGI_USAGE_RENDER_TARGET_OUTPUT)
	    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	    backBuffer->GetDesc(&bbDesc);

	    D3D11_VIEWPORT viewport = {};
	    viewport.Height = (r32)bbDesc.Height;
	    viewport.Width = (r32)bbDesc.Width;
	    viewport.MinDepth = 0;
	    viewport.MaxDepth = 1;

	    context->RSSetViewports(
		1,
		&viewport);

	    
	    //Creating a render target view that is bound to the back buffer resource (remember the definition
	    //of a 'render target' as well as 'view'
	    hr = d3dDevice->CreateRenderTargetView(
		backBuffer,
		nullptr,
		&renderTarget);

	    shaders shaders = {};
	    cube_buffers cubeBuffer = {};
	    direct_x_loaded_buffers loadedBuffers = {};
	    
	    CreateDeviceDependentResources(d3dDevice, &shaders, &loadedBuffers, &programState->setupArena, &memory);



	    //Creating the depth stencil
	    CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		(UINT)bbDesc.Width,
		(UINT)bbDesc.Height,
		1, //The depth stencil view has only one texture
		1, //Use a single mipmap levelx
		D3D11_BIND_DEPTH_STENCIL
		);


	    d3dDevice->CreateTexture2D(
		&depthStencilDesc,
		nullptr,
		&depthStencil);

	    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);

	    d3dDevice->CreateDepthStencilView(
		depthStencil,
		&depthStencilViewDesc,
		&depthStencilView);

	    	    
	    running = true;

	    
	    dx_camera camera = {};	    
	    CreateViewAndPerspective(&camera);



	    /* MAIN GAME LOOP */
	    /* MAIN GAME LOOP */
	    /* MAIN GAME LOOP */

	    //Setting up controls and such
	    game_input input[2] = {};
	    game_input* newInput = &input[0];
	    game_input* oldInput = &input[1];

	    
	    r32 lastTime = 0.0f;
	    
	    
	    while(running)
	    {
		r32 now = Win32GetWallClock();
		r32 deltaTime = now - lastTime;
		lastTime = now;
		
		DWORD maxControllerCount = XUSER_MAX_COUNT;

		game_controller_input* oldKeyboardController = GetController(oldInput, 0);
		game_controller_input* newKeyboardController = GetController(newInput, 0);

		*newKeyboardController = {};
		newKeyboardController->isConnected = true;
		for (i32 buttonIndex  = 0; buttonIndex < ArrayCount(newKeyboardController->buttons); ++buttonIndex)
		{
		    newKeyboardController->buttons[buttonIndex].endedDown =
			oldKeyboardController->buttons[buttonIndex].endedDown;
		}

		POINT mouseP;
		GetCursorPos(&mouseP);
		ScreenToClient(windowHandle, &mouseP);
		newInput->mouseX = mouseP.x;
		newInput->mouseY = mouseP.y;

		r32 xChange = (r32)oldInput->mouseX - (r32)newInput->mouseX;
		r32 yChange = (r32)oldInput->mouseY - (r32)newInput->mouseY;
		
		Win32ProcessPendingMessages(newKeyboardController, oldKeyboardController, &camera);
		ProcessPlayerMovement(newKeyboardController, &camera, deltaTime);
		ProcessMouseControl(&camera, xChange, yChange);
		
		
		Update();
		
		Render(context, renderTarget, depthStencilView, shaders.constantBuffer, &shaders, &loadedBuffers, &camera);
		swapChain->Present(1, 0);
	    }
	}
    }

    //ID3D11Device is just the set of functions which you call infrequently at the beginning of the program
    //to acquire and configure the set of resources needed to start drawing pixels

    //ID3D11DeviceContext contains the methods we will call every frame, loading in buffers and views and other
    //resources

    return(0);
}
