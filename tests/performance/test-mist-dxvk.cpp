// Community regression test added 2026-10-30; GNU GPL v3, see LICENSE.

#define CINTERFACE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstring>
constexpr GUID kMistClassification={0x71d871aa,0x673f,0x4f7a,{0xa4,0xf5,0x87,0x35,0xd3,0x90,0x51,0x23}};
uint64_t g_mistEpoch=1;struct MistClassification{uint64_t epoch;BOOL same;};
std::vector<uint8_t> g_mistTop(65536,0);struct{bool active=false;}g_probe;
int logCount=0; void Log(const char*,...){logCount++;}
void require(bool ok,const char* msg){if(!ok){printf("FAIL: %s\n",msg);ExitProcess(1);}}
    bool IsMistTexture(IDirect3DTexture9* t)
    {
        MistClassification result = {};
        DWORD resultSize = sizeof(result);
        if (SUCCEEDED(t->lpVtbl->GetPrivateData(t, kMistClassification, &result, &resultSize)) &&
            resultSize == sizeof(result) && result.epoch == g_mistEpoch)
            return result.same != FALSE;
        result.epoch = g_mistEpoch;
        result.same = FALSE;
        // If storage fails, keep the original draw without a GPU readback loop.
        if (FAILED(t->lpVtbl->SetPrivateData(t, kMistClassification, &result, sizeof(result), 0)))
            return false;
        bool same = false;
        D3DSURFACE_DESC desc = {};
        if (SUCCEEDED(t->lpVtbl->GetLevelDesc(t, 0, &desc)) &&
            (desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT5))
        {
            const UINT rowBytes = desc.Width / 4 * 16, rows = desc.Height / 4;
            D3DLOCKED_RECT lr = {};
            if (static_cast<size_t>(rowBytes) * rows == g_mistTop.size() &&
                SUCCEEDED(t->lpVtbl->LockRect(t, 0, &lr, nullptr, D3DLOCK_READONLY)) && lr.pBits)
            {
                same = true;
                for (UINT r = 0; r < rows && same; ++r)
                    same = memcmp(static_cast<const uint8_t*>(lr.pBits) + static_cast<size_t>(r) * lr.Pitch,
                                  g_mistTop.data() + static_cast<size_t>(r) * rowBytes, rowBytes) == 0;
                t->lpVtbl->UnlockRect(t, 0);
            }
        }
        result.same = same ? TRUE : FALSE;
        if (FAILED(t->lpVtbl->SetPrivateData(t, kMistClassification, &result, sizeof(result), 0)))
            return false;
        if (same)
            Log("sky: texture %p is the rain mist (SnowMist01.blp): left out in every frame", t);
        return same;
    }

int wmain(int argc, wchar_t** argv){
 HMODULE lib=LoadLibraryW(argc > 1 ? argv[1] : L"d3d9.dll");require(lib!=nullptr,"load exact local DXVK");
 auto create=reinterpret_cast<IDirect3D9*(WINAPI*)(UINT)>(GetProcAddress(lib,"Direct3DCreate9"));require(create!=nullptr,"D3D entry");auto api=create(D3D_SDK_VERSION);require(api!=nullptr,"D3D9");
 HWND wnd=CreateWindowExW(0,L"STATIC",L"Offline texture-cache verification",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);require(wnd!=nullptr,"hidden window");
 D3DPRESENT_PARAMETERS pp={};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=wnd;pp.BackBufferWidth=64;pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 IDirect3DDevice9* dev=nullptr;require(SUCCEEDED(api->lpVtbl->CreateDevice(api,0,D3DDEVTYPE_HAL,wnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)),"create hidden 64x64 device");
 std::vector<IDirect3DTexture9*> textures;
 for(int k=0;k<300;k++){
 IDirect3DTexture9*t=nullptr;require(SUCCEEDED(dev->lpVtbl->CreateTexture(dev,256,256,1,0,D3DFMT_DXT3,D3DPOOL_MANAGED,&t,nullptr)),"texture allocation");textures.push_back(t);
 D3DLOCKED_RECT lr={};require(SUCCEEDED(t->lpVtbl->LockRect(t,0,&lr,nullptr,0)),"fill lock");for(int y=0;y<64;y++){auto row=(char*)lr.pBits+y*lr.Pitch;memset(row,k==299?0:255,1024);}t->lpVtbl->UnlockRect(t,0);
 }
 for(int frame=0;frame<10;frame++)for(int k=0;k<300;k++)require(IsMistTexture(textures[k])==(k==299),"classification beyond 256 resources");
 for(auto t:textures){MistClassification result={};DWORD size=sizeof(result);require(SUCCEEDED(t->lpVtbl->GetPrivateData(t,kMistClassification,&result,&size))&&result.epoch==g_mistEpoch,"resource result retained");}
 require(logCount==1,"one classification log across repeated frames"); require(SUCCEEDED(dev->lpVtbl->Reset(dev,&pp)),"device reset");++g_mistEpoch;
 for(int k=0;k<300;k++)require(IsMistTexture(textures[k])==(k==299),"classification after reset");
 for(auto t:textures)t->lpVtbl->Release(t);dev->lpVtbl->Release(dev);api->lpVtbl->Release(api);DestroyWindow(wnd);FreeLibrary(lib);
 puts("PASS: actual local DXVK, 300 DXT3 managed textures, positive and negative classification, device reset; isolated hidden device, no game input.");
}
