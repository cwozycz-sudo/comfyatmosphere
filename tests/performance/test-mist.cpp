// Community regression test added 2026-10-30; GNU GPL v3, see LICENSE.

#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <unordered_map>
using UINT=unsigned;using DWORD=unsigned;using BOOL=int;
constexpr int FALSE=0,TRUE=1,D3DFMT_DXT3=3,D3DFMT_DXT5=5,D3DLOCK_READONLY=16;
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)
struct GUID{};constexpr GUID kMistClassification{};
uint64_t g_mistEpoch=1;struct MistClassification{uint64_t epoch;BOOL same;};
std::vector<uint8_t> g_mistTop(65536,7);int logs=0;
void Log(const char*,...){logs++;}
struct D3DSURFACE_DESC{int Format=3;UINT Width=256,Height=256;};struct D3DLOCKED_RECT{int Pitch;void* pBits;};
struct IDirect3DTexture9;
struct Vtbl {
 int (*GetPrivateData)(IDirect3DTexture9*,const GUID&,void*,DWORD*);
 int (*SetPrivateData)(IDirect3DTexture9*,const GUID&,const void*,DWORD,DWORD);
 int (*GetLevelDesc)(IDirect3DTexture9*,int,D3DSURFACE_DESC*);
 int (*LockRect)(IDirect3DTexture9*,int,D3DLOCKED_RECT*,void*,int);
 int (*UnlockRect)(IDirect3DTexture9*,int);
};
struct IDirect3DTexture9{Vtbl* lpVtbl;bool has=false,fail=false;MistClassification c{};std::vector<uint8_t> pixels=std::vector<uint8_t>(65536,7);int locks=0;int fmt=3;bool lockFail=false;};
int get(IDirect3DTexture9*t,const GUID&,void*p,DWORD*n){if(!t->has)return -1;memcpy(p,&t->c,sizeof(t->c));*n=sizeof(t->c);return 0;}
int set(IDirect3DTexture9*t,const GUID&,const void*p,DWORD,DWORD){if(t->fail)return -1;t->has=true;memcpy(&t->c,p,sizeof(t->c));return 0;}
int desc(IDirect3DTexture9*t,int,D3DSURFACE_DESC*d){*d={};d->Format=t->fmt;return 0;}
int lock(IDirect3DTexture9*t,int,D3DLOCKED_RECT*r,void*,int){t->locks++;if(t->lockFail)return -1;*r={1024,t->pixels.data()};return 0;}
int unlock(IDirect3DTexture9*,int){return 0;}
Vtbl v{get,set,desc,lock,unlock};
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


int main(){
 std::vector<IDirect3DTexture9> ts;
 for(int i=0;i<300;i++){ts.push_back({&v});if(i<299)ts.back().pixels[0]=0;}
 for(int frame=0;frame<100;frame++)for(int i=0;i<300;i++)assert(IsMistTexture(&ts[i])==(i==299));
 int locks=0;for(auto&t:ts)locks+=t.locks;
 assert(locks==300);assert(logs==1);
 // The released pointer-table algorithm: after 256 entries the positive repeats every frame.
 int oldLocks=0,oldLogs=0;std::unordered_map<int,bool> cache;
 for(int f=0;f<100;f++)for(int i=0;i<300;i++)if(cache.find(i)==cache.end()){
  oldLocks++;if(i==299)oldLogs++;if(cache.size()<256)cache[i]=(i==299);
 }
 assert(oldLocks==4656 && oldLogs==100);
 g_mistEpoch++;assert(IsMistTexture(&ts[299]));assert(ts[299].locks==2);
 ts[299].has=false;ts[299].pixels[0]=0;assert(!IsMistTexture(&ts[299]));assert(ts[299].locks==3);
 IDirect3DTexture9 unsupported{&v};unsupported.fail=true;for(int i=0;i<100;i++)assert(!IsMistTexture(&unsupported));assert(unsupported.locks==0);
 IDirect3DTexture9 dxt5{&v};dxt5.fmt=5;assert(IsMistTexture(&dxt5));assert(IsMistTexture(&dxt5));assert(dxt5.locks==1);
 IDirect3DTexture9 fail{&v};fail.lockFail=true;for(int i=0;i<100;i++)assert(!IsMistTexture(&fail));assert(fail.locks==1);
 puts("PASS actual candidate function: 300 textures x 100 frames; GPU-lock calls 4656 -> 300; repeated positive logs 100 -> 1; epoch reset, resource/pointer reuse, storage failure, lock failure, DXT3/5. Mock D3D, not game frame-time measurement.");
}
