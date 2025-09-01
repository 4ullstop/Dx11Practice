#include <windows.h>

//I don't like this include statement, I would Like to figure out how not to include it, or create a local one
//for the code to reference instead of going into this location

#include "D:/ExternalCustomAPIs/MemoryPools/code/memory_pools.h"



//Also not a huge fan of the fact that typedefs.h is redefined
#include "typedefs.h"
#include <d3d11_2.h>


#define MEMORY_POOL_PUSH_STRUCT(name) void* name(memory_arena* arena, void* type)
typedef MEMORY_POOL_PUSH_STRUCT(memory_pool_push_struct);
MEMORY_POOL_PUSH_STRUCT(MemoryPoolPushStructStub)
{
    return(0);
}
global_variable memory_pool_push_struct* MemoryPoolPushStruct_ = MemoryPoolPushStructStub;
#define MemoryPoolPushStruct MemoryPoolPushStruct_

#define MEMORY_POOL_PUSH_ARRAY(name) void* name(memory_arena* arena, i32 count, void* type)
typedef MEMORY_POOL_PUSH_ARRAY(memory_pool_push_array);
MEMORY_POOL_PUSH_ARRAY(MemoryPoolPushArrayStub)
{
    return(0);
}
global_variable memory_pool_push_array* MemoryPoolPushArray = MemoryPoolPushArrayStub;
#define MemoryPoolPushArray MemoryPoolPushArray_




global_variable bool32 running;

internal void
Win32ProcessPendingMessages(void)
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

//It's just gonna be worth creating your own APIs for the functions you seem to be reusing like crazy
#if 0
internal void
CreateShaders(ID3D11Device* device)
{
    FILE* vShader, *pShader; //vertex (v) pixel (p)
    BYTE* bytes;

    size_t destSize = 4096;
    size_t bytesRead = 0;
    
    
}
#endif
//NOTE: this function should be called asynchronously, Take the time to have it execute
//on a separate thread
internal void
CreateDeviceDependentResources(void)
{
//    CreateShaders();

//    CreateCube();
}

int CALLBACK WinMain(HINSTANCE hInstance,
		     HINSTANCE hPrevInstance,
		     LPSTR lpCmdLine,
		     int nCmdShow)
{
    /*
      Load our memory pool library
     */

    memory_pool_dll_code memoryPoolCode = {};

    HMODULE memoryPoolLibrary = LoadLibrary("D:/ExternalCustomAPIs/MemoryPools/dll/memory_pools.dll");

    if (memoryPoolLibrary)
    {
	memoryPoolCode.PushStruct = (memory_pool_push_struct*)GetProcAddress(memoryPoolLibrary, "PushStruct");
	memoryPoolCode.PushArray = (memory_pool_push_array*)GetProcAddress(memoryPoolLibrary, "PushArray");
    }
    if (memoryPoolCode.PushStruct && memoryPoolCode.PushArray)
    {
	OutputDebugString("Memory Pool Code Successfully Loaded");
    }
    

    
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
    
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* context;

    D3D_FEATURE_LEVEL featureLevel;
    
    D3D11CreateDevice(
	nullptr,
	D3D_DRIVER_TYPE_HARDWARE,
	0,
	deviceFlags,
	levels,
	ArrayCount(levels),
	D3D11_SDK_VERSION,
	&d3dDevice,
	&featureLevel,
	&context);
	
    WNDCLASS windowClass = {};
    windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    windowClass.lpfnWndProc = Win32MainWindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "Dx11Test";

    if (RegisterClass(&windowClass))
    {
	HWND windowHandle = CreateWindowEx(
	    0,
	    windowClass.lpszClassName,
	    "DX11 Test",
	    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    0,
	    0,
	    hInstance,
	    0);

	RECT rect;


	if (windowHandle)
	{
	    //Now that we have a window to draw in and an interface to send data and give commands to the GPU,
	    //we create the swap chain

	    //Configuring the swap chain
	    GetWindowRect(windowHandle, &rect);
	    i32 windowWidth = rect.right - rect.left;
	    i32 windowHeight = rect.bottom - rect.top;
	    
	    DXGI_SWAP_CHAIN_DESC1 desc = {};
	    desc.BufferCount = 2;
	    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	    desc.SampleDesc.Count = 1;
	    desc.SampleDesc.Quality = 0;
	    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	    desc.Width = windowWidth;
	    desc.Height = windowHeight;

	    IDXGIDevice3* dxgiDevice = 0;	    

	    IDXGIAdapter* adapter = NULL;
	    IDXGIFactory2* factory = 0;
	    
//	    HRESULT hr = dxgiDevice->GetAdapter(&adapter);
	    HR(d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
	    HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&adapter));

	    HR(adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory));

	    IDXGISwapChain1* swapChain = 0;
	    ID3D11Texture2D* backBuffer = 0;
	    ID3D11RenderTargetView* renderTarget = 0;

	    D3D11_TEXTURE2D_DESC bbDesc;

	    ID3D11Texture2D* depthStencil;
	    ID3D11DepthStencilView* depthStencilView;
	    
	    D3D11_VIEWPORT viewport;


	    HR(factory->CreateSwapChainForHwnd(
		d3dDevice,
		windowHandle,
		&desc,
		0,
		0,
		&swapChain));

	    //Getting the back buffer from the swap chain (since we defined DXGI_USAGE_RENDER_TARGET_OUTPUT)
	    HR(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

	    //Creating a render target view that is bound to the back buffer resource (remember the definition
	    //of a 'render target' as well as 'view'
	    HR(d3dDevice->CreateRenderTargetView(
		backBuffer,
		nullptr,
		&renderTarget));

	    backBuffer->GetDesc(&bbDesc);


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

	    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	    viewport.Height = (r32)bbDesc.Height;
	    viewport.Width = (r32)bbDesc.Width;
	    viewport.MinDepth = 0;
	    viewport.MaxDepth = 1;

	    context->RSSetViewports(
		1,
		&viewport);

	    
	    running = true;
	    
	    while(running)
	    {
		Win32ProcessPendingMessages();
	    }
	}
    }

    //ID3D11Device is just the set of functions which you call infrequently at the beginning of the program
    //to acquire and configure the set of resources needed to start drawing pixels

    //ID3D11DeviceContext contains the methods we will call every frame, loading in buffers and views and other
    //resources

    return(0);
}
