// Community regression test added 2026-10-30; GNU GPL v3, see LICENSE.
#include "pose_search.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main()
{
    const std::vector<PoseSearchSpan> clockSpans{{0,100},{100,200}};
    int clockSeq=0;float clockMs=80;
    assert(AdvancePoseClock(clockSpans,clockSeq,clockMs,70,[](int){return 1;}));
    assert(clockSeq==1 && clockMs==150);
    assert(AdvancePoseClock(clockSpans,clockSeq,clockMs,50,[](int){return 1;}));
    assert(clockMs==200); // End is inclusive, as in the original playback.
    assert(AdvancePoseClock(clockSpans,clockSeq,clockMs,1,[](int){return 1;}) && clockMs==101);
    assert(!AdvancePoseClock(clockSpans,clockSeq,clockMs,10000,[](int){return 1;}));
    int invalid=-1;float zero=0;
    assert(!AdvancePoseClock(clockSpans,invalid,zero,1,[](int){return 1;}));
    assert(!AdvancePoseClock(std::vector<PoseSearchSpan>{},invalid,zero,1,[](int){return 1;}));
    const std::vector<PoseSearchSpan> spans{{0,30000},{100,18000}};
    auto error = [](int q,float t) { const float d=t-12347; return d*d+(q==1?0.0f:100.0f); };
    // Original full coarse scan and one-ms refinement, in the original evaluation order.
    int oldSeq=-1; float oldTime=0,oldError=1e9f;
    auto consider=[&](int q,float t){float e=error(q,t);if(e<oldError){oldError=e;oldSeq=q;oldTime=t;}};
    for(int q=0;q<2;q++)for(float t=(float)spans[q].start;t<=spans[q].end;t+=10)consider(q,t);
    const int refine=oldSeq;const float mid=oldTime;
    for(float t=mid-10;t<=mid+10;t+=1)if(t>=spans[refine].start&&t<=spans[refine].end)consider(refine,t);
    PoseSearch s;s.Begin(spans);
    assert(!s.Step(error,[]{return false;}));assert(s.evaluations==0);
    int frames=0;
    while(!s.Done())
    {
        unsigned available=17, used=0;
        s.Step([&](int q,float t){++used;return error(q,t);},[&](){if(!available)return false;--available;return true;});
        assert(used<=17);assert(++frames<1000);
    }
    assert(frames>200 && s.bestSequence==oldSeq && s.bestTime==oldTime && s.bestError==oldError);
    PoseSearch local;local.BeginLocal(spans,1,12200,12500,12340,49,0.01f);
    unsigned globalSamples=0;
    while(!local.Done()) { unsigned left=7; local.Step([&](int q,float t){if(!local.Local())globalSamples++;return error(q,t);},[&](){if(!left)return false;--left;return true;}); }
    // This local grid cannot hit odd 12347, so it must resume the global fallback without retaining a bad seed.
    assert(globalSamples>0 && local.bestTime==oldTime && local.bestError==oldError);
    local.BeginLocal(spans,1,12201,12501,12340,49,0.01f);globalSamples=0;
    while(!local.Done()) { unsigned left=7; local.Step([&](int q,float t){if(!local.Local())globalSamples++;return error(q,t);},[&](){if(!left)return false;--left;return true;}); }
    assert(globalSamples==0 && local.bestTime==12347 && local.bestError==0);
    const auto evaluations=s.evaluations;s.Step(error,[]{return true;});assert(s.evaluations==evaluations);
    s.Cancel();assert(!s.Active());s.Begin({{0,0}});s.Step(error,[]{return true;});assert(s.Done());
    s.Begin({});s.Step(error,[]{return true;});assert(s.Done()&&s.bestSequence==-1);
    s.Begin({{10,0},{0,std::numeric_limits<double>::infinity()}});s.Step(error,[]{return true;});assert(s.Done());
    // All objects share the frame allowance, and the next frame starts after the last serviced one.
    std::vector<PoseSearch> jobs(20);for(auto& j:jobs)j.Begin(spans);
    size_t next=0;unsigned globalFrames=0;
    for(;;)
    {
        unsigned available=37,used=0;bool all=true;const size_t first=next;
        for(size_t v=0;v<jobs.size();v++)
        {
            const size_t i=(first+v)%jobs.size();if(jobs[i].Done())continue;all=false;
            if(!available)continue;next=(i+1)%jobs.size();
            jobs[i].Step([&](int q,float t){++used;return error(q,t);},[&](){if(!available)return false;--available;return true;});
        }
        assert(used<=37);if(all)break;assert(++globalFrames<10000);
    }
    for(auto& j:jobs)assert(j.bestSequence==oldSeq&&j.bestTime==oldTime&&j.bestError==oldError);
    puts("PASS: resumed search equals original exhaustive result; zero/tiny/shared budgets; 20-job fairness and completion; cancellation, empty/invalid spans, exact boundaries and completed-job idempotence.");
}
