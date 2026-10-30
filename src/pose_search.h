// Modified 2026-10-30 by cwozycz-sudo: Add resumable pose search and clock helpers.
// Community changes distributed under the upstream GNU GPL v3; see LICENSE.
#pragma once
#include <vector>
#include <cstddef>
#include <cmath>
#include <utility>

// Resumable version of the original coarse scan followed by a one-ms refinement.
// Evaluations use immutable observations held by the caller across frames.
struct PoseSearchSpan { double start, end; };

template<class Sequences, class Next>
bool AdvancePoseClock(const Sequences& sequences, int& sequence, float& milliseconds,
                      float elapsed, Next next)
{
    if (sequence < 0 || static_cast<std::size_t>(sequence) >= sequences.size()) return false;
    milliseconds += elapsed;
    for (int n = 0; n < 8 && milliseconds > static_cast<float>(sequences[sequence].end); ++n)
    {
        const float over = milliseconds - static_cast<float>(sequences[sequence].end);
        sequence = next(sequence);
        if (sequence < 0 || static_cast<std::size_t>(sequence) >= sequences.size()) return false;
        milliseconds = static_cast<float>(sequences[sequence].start) + over;
    }
    return std::isfinite(milliseconds) && milliseconds <= static_cast<float>(sequences[sequence].end);
}
struct PoseSearch
{
    std::vector<PoseSearchSpan> spans;
    std::size_t sequence = 0;
    double cursor = 0, refineEnd = 0;
    int phase = 0; // 0 idle, 1 coarse, 2 refine, 3 complete, 4 local window
    int bestSequence = -1, refineSequence = -1;
    float bestTime = 0, bestError = 1e9f;
    unsigned evaluations = 0;
    int localSequence = -1;
    double localEnd = 0;
    float localThreshold = 0;
    bool Active() const { return phase != 0; }
    bool Done() const { return phase == 3; }
    bool Local() const { return phase == 4; }
    void Cancel() { phase = 0; spans.clear(); }
    void Begin(std::vector<PoseSearchSpan> input)
    {
        spans = std::move(input); sequence = 0; phase = 1;
        bestSequence = refineSequence = -1; bestTime = 0; bestError = 1e9f;
        evaluations = 0; cursor = spans.empty() ? 0 : spans[0].start;
    }
    void BeginLocal(std::vector<PoseSearchSpan> input, int q, double lo, double hi,
                    float seedTime, float seedError, float threshold)
    {
        Begin(std::move(input));
        if (q < 0 || static_cast<std::size_t>(q) >= spans.size() || lo > hi) return;
        localSequence = q; localEnd = hi; localThreshold = threshold;
        cursor = lo; phase = 4;
        bestSequence = q; bestTime = seedTime; bestError = seedError;
    }
    template<class Evaluate, class Allow>
    bool Step(Evaluate evaluate, Allow allow)
    {
        while (phase == 1 || phase == 2 || phase == 4)
        {
            if (phase == 4 && cursor > localEnd)
            {
                if (bestError <= localThreshold) { phase = 3; break; }
                // The original global scan resets a failed local prediction.
                phase = 1; sequence = 0; cursor = spans.empty() ? 0 : spans[0].start;
                bestSequence = -1; bestTime = 0; bestError = 1e9f;
                continue;
            }
            if (phase == 1)
            {
                while (sequence < spans.size() &&
                       (!std::isfinite(cursor) || !std::isfinite(spans[sequence].end) ||
                        cursor > spans[sequence].end))
                {
                    ++sequence;
                    if (sequence < spans.size()) cursor = spans[sequence].start;
                }
                if (sequence == spans.size())
                {
                    if (bestSequence < 0) { phase = 3; break; }
                    refineSequence = bestSequence;
                    cursor = bestTime - 10.0;
                    refineEnd = bestTime + 10.0;
                    phase = 2;
                    continue;
                }
            }
            else if (phase == 2)
            {
                if (cursor > refineEnd) { phase = 3; break; }
                if (cursor < spans[refineSequence].start || cursor > spans[refineSequence].end)
                { cursor += 1.0; continue; }
            }
            if (!allow()) return false;
            const int q = phase == 4 ? localSequence : phase == 1 ? static_cast<int>(sequence) : refineSequence;
            const float t = static_cast<float>(cursor);
            const float error = evaluate(q, t);
            ++evaluations;
            if (error < bestError)
            { bestError = error; bestSequence = q; bestTime = t; }
            cursor += phase == 4 ? 2.0 : phase == 1 ? 10.0 : 1.0;
        }
        return Done();
    }
};
