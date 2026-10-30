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

int logCount=0;void Log(const char*,...){logCount++;}
void require(bool ok,const char*msg){if(!ok){printf("FAIL: %s\n",msg);ExitProcess(1);}}
namespace
{
    // Liquid textures already judged (WaterTextureIsWater). Cleared with the device.
    constexpr GUID kLiquidClassification = {0xf2941a63, 0x40f7, 0x42cc, {0xa1,0x37,0xf6,0x24,0x56,0x61,0x9c,0xc0}};
    uint64_t g_liquidEpoch = 1;
    struct LiquidClassification { uint64_t epoch; BOOL water; };

    float Unpack565(uint16_t c, int ch)
    {
        return ch == 0 ? ((c >> 11) & 31) / 31.0f : ch == 1 ? ((c >> 5) & 63) / 63.0f : (c & 31) / 31.0f;
    }

    // The average colour of a texture's smallest level: one pixel, or a compressed block's two end colours.
    bool TextureColour(IDirect3DTexture9* t, float rgb[3])
    {
        const DWORD levels = t->lpVtbl->GetLevelCount(t);
        if (levels == 0)
            return false;
        D3DSURFACE_DESC desc = {};
        if (FAILED(t->lpVtbl->GetLevelDesc(t, levels - 1, &desc)))
            return false;
        D3DLOCKED_RECT lr = {};
        if (FAILED(t->lpVtbl->LockRect(t, levels - 1, &lr, nullptr, D3DLOCK_READONLY)) || !lr.pBits)
            return false;
        const uint8_t* p = static_cast<const uint8_t*>(lr.pBits);
        bool ok = true;
        switch (desc.Format)
        {
        case D3DFMT_DXT1: case D3DFMT_DXT2: case D3DFMT_DXT3: case D3DFMT_DXT4: case D3DFMT_DXT5:
        {
            const uint8_t* b = desc.Format == D3DFMT_DXT1 ? p : p + 8;   // the colour half of the block
            uint16_t c0, c1;
            memcpy(&c0, b, 2);
            memcpy(&c1, b + 2, 2);
            for (int ch = 0; ch < 3; ++ch)
                rgb[ch] = 0.5f * (Unpack565(c0, ch) + Unpack565(c1, ch));
            break;
        }
        case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8:
            rgb[0] = p[2] / 255.0f; rgb[1] = p[1] / 255.0f; rgb[2] = p[0] / 255.0f;
            break;
        case D3DFMT_R5G6B5:
        {
            uint16_t c;
            memcpy(&c, p, 2);
            for (int ch = 0; ch < 3; ++ch)
                rgb[ch] = Unpack565(c, ch);
            break;
        }
        default:
            ok = false;
        }
        t->lpVtbl->UnlockRect(t, levels - 1);
        return ok;
    }
}

// Water in a building comes in the same draws as its lava and slime (2026-10-03, Stormwind's canals): the
// texture tells them apart. Blue at least as strong as red, and green not far over blue: water; red over blue
// is lava, green far over blue slime. Each texture is judged once, and logged.
bool WaterTextureIsWater(IDirect3DDevice9* dev)
{
    IDirect3DBaseTexture9* base = nullptr;
    if (FAILED(dev->lpVtbl->GetTexture(dev, 0, &base)) || !base)
        return false;
    LiquidClassification result = {};
    DWORD resultSize = sizeof(result);
    if (SUCCEEDED(base->lpVtbl->GetPrivateData(base, kLiquidClassification, &result, &resultSize)) &&
        resultSize == sizeof(result) && result.epoch == g_liquidEpoch)
    {
        base->lpVtbl->Release(base);
        return result.water != FALSE;
    }
    result.epoch = g_liquidEpoch;
    result.water = FALSE;
    if (FAILED(base->lpVtbl->SetPrivateData(base, kLiquidClassification, &result, sizeof(result), 0)))
    {
        base->lpVtbl->Release(base);
        return false;
    }
    bool water = false;
    float rgb[3] = {};
    if (base->lpVtbl->GetType(base) == D3DRTYPE_TEXTURE &&
        TextureColour(reinterpret_cast<IDirect3DTexture9*>(base), rgb))
    {
        water = rgb[2] >= rgb[0] && rgb[1] <= rgb[2] * 1.5f;
        Log("water: a building's liquid, texture %p, colour %.2f %.2f %.2f: %s", base, rgb[0], rgb[1], rgb[2],
            water ? "water, ours" : "not water, the game's");
    }
    else
        Log("water: a building's liquid, texture %p: its colour could not be read, left to the game", base);
    result.water = water ? TRUE : FALSE;
    if (FAILED(base->lpVtbl->SetPrivateData(base, kLiquidClassification, &result, sizeof(result), 0)))
        water = false;
    base->lpVtbl->Release(base);
    return water;
}

int wmain(int argc, wchar_t** argv){
 HMODULE lib=LoadLibraryW(argc > 1 ? argv[1] : L"d3d9.dll");require(lib!=nullptr,"load exact local DXVK");
 auto create=reinterpret_cast<IDirect3D9*(WINAPI*)(UINT)>(GetProcAddress(lib,"Direct3DCreate9"));require(create!=nullptr,"D3D entry");auto api=create(D3D_SDK_VERSION);require(api!=nullptr,"D3D9");
 HWND wnd=CreateWindowExW(0,L"STATIC",L"Offline texture-cache verification",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);require(wnd!=nullptr,"hidden window");
 D3DPRESENT_PARAMETERS pp={};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=wnd;pp.BackBufferWidth=64;pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 IDirect3DDevice9* dev=nullptr;require(SUCCEEDED(api->lpVtbl->CreateDevice(api,0,D3DDEVTYPE_HAL,wnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)),"create hidden 64x64 device");

 std::vector<IDirect3DTexture9*> textures;
 for(int k=0;k<600;k++){
  IDirect3DTexture9*t=nullptr;require(SUCCEEDED(dev->lpVtbl->CreateTexture(dev,4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr)),"allocate liquid");textures.push_back(t);
  D3DLOCKED_RECT lr={};require(SUCCEEDED(t->lpVtbl->LockRect(t,0,&lr,nullptr,0)),"fill liquid");
  for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=k%2?0xFF0000FF:0xFFFF0000;
  t->lpVtbl->UnlockRect(t,0);
 }
 for(int f=0;f<10;f++)for(int k=0;k<600;k++){dev->lpVtbl->SetTexture(dev,0,reinterpret_cast<IDirect3DBaseTexture9*>(textures[k]));require(WaterTextureIsWater(dev)==(k%2!=0),"water and lava beyond512");}
 require(logCount==600,"classifications logged once per resource");
 dev->lpVtbl->SetTexture(dev,0,nullptr);require(!WaterTextureIsWater(dev),"no bound texture");
 require(SUCCEEDED(dev->lpVtbl->Reset(dev,&pp)),"reset liquid device");++g_liquidEpoch;
 for(int k=0;k<600;k++){dev->lpVtbl->SetTexture(dev,0,reinterpret_cast<IDirect3DBaseTexture9*>(textures[k]));require(WaterTextureIsWater(dev)==(k%2!=0),"liquid after reset");}
 require(logCount==1200,"one new classification per reset epoch");
 // New resource-private state at the same wrapper address cannot inherit old classification.
 auto*t=textures[0];t->lpVtbl->FreePrivateData(t,kLiquidClassification);D3DLOCKED_RECT lr={};require(SUCCEEDED(t->lpVtbl->LockRect(t,0,&lr,nullptr,0)),"change recycled liquid");
 for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)lr.pBits+y*lr.Pitch))[x]=0xFF0000FF;
 t->lpVtbl->UnlockRect(t,0);dev->lpVtbl->SetTexture(dev,0,reinterpret_cast<IDirect3DBaseTexture9*>(t));require(WaterTextureIsWater(dev),"recycled resource state");
 dev->lpVtbl->SetTexture(dev,0,nullptr);for(auto t:textures)t->lpVtbl->Release(t);
 dev->lpVtbl->Release(dev);api->lpVtbl->Release(api);DestroyWindow(wnd);FreeLibrary(lib);
 puts("PASS: actual installed DXVK and candidate liquid functions; 600 water/lava resources x10 frames, cached once each, Reset reclassification, empty slot and replaced resource-private state.");
}
