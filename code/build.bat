@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

set commonCompilerFlags=-nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4101 -wd4189 -wd4505 -DTEST_INTERNAL=1 -DTEST_SLOW=1 -DTEST_WIN32=1 -FC -Zi

set commonLinkerFlags=-incremental:no user32.lib gdi32.lib winmm.lib d3d11.lib dxgi.lib /LIBPATH:"D:\ExternalCustomAPIs\MemoryPools\dll" memory_pools.lib

REM 32-bit build
REM cl %commonCompilerFlags% ..\code\win32_handmade.cpp %commonLinkerFlags%

REM fxc compiler for shaders...
REM start "C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" "L:\code\vshader.hlsl"

fxc.exe /T vs_5_0 /Fo CubeVertexShader.cso "L:\code\vshader.hlsl"
fxc.exe /T ps_5_0 /Fo CubePixelShader.cso "L:\code\pshader.hlsl"

cl %commonCompilerFlags% ..\code\win32_dx11.cpp  /link %commonLinkerFlags%
popd
