# Unofficial community cache and frame-pacing fixes

Upstream: [aloofbit/comfyatmosphere](https://github.com/aloofbit/comfyatmosphere).
[GNU GPL v3](LICENSE) · [Attribution and change notice](NOTICE.community.md) · [中文](COMMUNITY.zh-CN.md).

Based on upstream `73bebaaf2439aa17cc7766bb99bf8513f9aa6ade`.
This is a community fork, not an official upstream release.

## Changes

- Cache positive **and negative** rain splash, rain mist and building-liquid classifications
  on D3D resources rather than fixed-capacity maps keyed by recycled pointers.
  Device reset/replacement invalidates surviving results using epochs.
- Rotate diagnostic logs at approximately 5 MiB, retaining one previous file.
- Report shadow CPU stages for individual slow frames with `/atmos framelog 30`.
- Resume shadow-animation matching across frames under a shared approximately **1 ms
  search budget**, with rotating service. Retain the local window, global coarse scan,
  refinement, and animation clock rules. Observations are frozen during a search;
  historical results are projected and validated against the current pose before use.

The budget covers search evaluations, **not all rendering operations**. A single
non-preemptible evaluation can exceed it. Pose reacquisition may take multiple frames;
stale results can be rejected. No models, fish, bats or effects are removed, and the
patch does not lower quality settings.

## Evidence and limitations

One Windows/NVIDIA/DXVK setup recorded **38.47 ms in shadow animation processing**
inside a 48.36 ms frame. Sampled searches included decorative fish and bat displays
6407/6408. After the change, the player reported visibly steadier frame pacing.
**There is no controlled post-fix FPS benchmark or broad hardware compatibility claim.**
CPU regressions and standalone DXVK managed-resource/Reset checks passed; those tests
do not establish that every client, camera angle or long session is free of problems.

## Installation

1. Exit the game and back up your existing `comfyatmos.dll`.
2. Copy the release DLL into your client folder.
3. Existing users should **retain their personal ini and addon settings**. For a fresh
   installation use the supplied default `comfyatmos.ini` and `addon/ComfyAtmosphere`,
   and follow the original README to add `comfyatmos.dll` to `dlls.txt`.
4. Start with your normal mod loader. Verify the download using `SHA256SUMS.txt`.

Game files, DXVK and VanillaFixes are not bundled. If a service launcher manages this
DLL, update through that service rather than forcing a different file into its client.
To roll back, exit and restore the DLL you backed up.

If facing the sun produces a black screen with a bright spot, check debug view first:
`/console comfyDebugView 0`. Debug views intentionally replace the scene. This release
does **not** add automatic reset of a saved debug view.

## Complete-source build

Install Visual Studio 2022 with Desktop development with C++/Windows SDK and CMake.
Use **Win32**, not x64. From a copy of this repository:

```powershell
./scripts/build-community.ps1
```

The script reads `COMMUNITY_VERSION`, builds the DLL and CPU regressions, then runs
CTest. Output: `build-community/artifacts/Release/comfyatmos.dll`.

Equivalent commands:

```text
cmake -S . -B build-community -G "Visual Studio 17 2022" -A Win32 -DCOMFYATMOS_RELEASE_VERSION=v0.17.1-community.20261030.1 -DCOMFY_BUILD_PERFORMANCE_TESTS=ON
cmake --build build-community --config Release --parallel 2
ctest --test-dir build-community -C Release --output-on-failure
```

Optional standalone GPU regressions:

```text
cmake -S . -B build-community -G "Visual Studio 17 2022" -A Win32 -DCOMFY_BUILD_GPU_TESTS=ON
cmake --build build-community --config Release --parallel 2
build-community/Release/mist_dxvk_test.exe C:/path/to/d3d9.dll
build-community/Release/liquid_dxvk_test.exe C:/path/to/d3d9.dll
```

These create an isolated hidden graphics device and do not control any game window.
Use your own D3D9/DXVK installation. All test source is under `tests/performance`.

## Source and provenance

The tagged GitHub source archive and attached `*-source.zip` contain the corresponding
source, tests and build scripts. `SOURCE.json` in the binary download records the
source commit/tree, release identifier and upstream base. Development tools and the
Windows SDK are installed separately. Different compiler versions are not promised
to produce byte-identical files. Original project documentation follows in README.md.
