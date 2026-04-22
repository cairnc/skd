# layerviewer

A winscope-like viewer for AOSP layer traces, built to use real
SurfaceFlinger code (eventually) rather than reimplementing it. Rendering
via Skia/Ganesh; UI via ImGui; windowing via SDL3.

## Current state

- **Scaffold**: SDL3 window + GL 3.2 core context + Skia `GrDirectContext`
  on the default framebuffer + ImGui (docking) on top.
- **AOSP foundation** (under `aosp/`, pulled from `android-15.0.0_r23`):
  - `libutils` (RefBase, sp/wp, String8/16, Vector, Errors, Threads)
  - `libbase` (stringprintf, file, unique_fd, strings, …)
  - `libui` value types (Rect, Region, Transform, FloatRect, Point, Size)
  - `libmath` (header-only: mat4, vec2/3/4, quat, half)
  - `arect`, `ftl` (header-only)
  - `shim/` for the Android side: `<log/log.h>`, `<cutils/trace.h>`,
    `<cutils/properties.h>`, `<cutils/native_handle.h>`, `<hardware/*>`
    collapse to no-ops / forward to graphics enums.
- **Perfetto SF layer-trace protos** compiled via `protoc` at build time
  (`surfaceflinger_common.proto`, `surfaceflinger_layers.proto`,
  `surfaceflinger_transactions.proto`, `graphics/rect.proto`).
- **Trace loader + viewer**:
  - Drop a `.winscope` file on the window (or pass it on the command
    line) to parse `LayersTraceFileProto`.
  - Frame slider across snapshots.
  - Layer tree panel (built from `parent` edges).
  - Inspector for the selected layer.
  - Skia draws each layer's `screen_bounds` fitted to the canvas with the
    selected layer highlighted.
- **`make-fake-trace`**: generates a synthetic `.winscope` file with a
  few moving layers, for smoke testing.

### Not yet

- GL-backed `GraphicBuffer` / `Fence` (to be written from scratch against
  a GL texture + `GLsync`).
- RenderEngine (`SkiaGLRenderEngine` needs EGL/GLES — macOS desktop GL
  will need a `SkiaDesktopGLRenderEngine` variant against our existing
  `GrDirectContext`).
- CompositionEngine.
- SurfaceFlinger `FrontEnd` (`LayerHierarchyBuilder`,
  `LayerSnapshotBuilder`) — currently we read `screen_bounds` from the
  trace directly; the real frontend would compute snapshots from
  transactions.
- Perfetto trace ingestion via the full Perfetto trace schema (for
  traces captured with `perfetto --config` rather than dumpsys-style
  LYRTRACE files).

## Building

Dependencies are git submodules; Skia builds from source, protobuf comes
from Homebrew.

```sh
git submodule update --init --recursive
brew install cmake ninja protobuf  # macOS
```

### macOS

```sh
cd third_party/skia
python3 tools/git-sync-deps
bin/gn gen out/Release --args='is_debug=false skia_use_gl=true skia_use_metal=true'
ninja -C out/Release skia
cd ../..

cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
```

### Linux

```sh
sudo apt-get install -y cmake ninja-build python3 protobuf-compiler \
    libprotobuf-dev libgl-dev libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libxext-dev libxss-dev libxtst-dev \
    libwayland-dev libxkbcommon-dev libdecor-0-dev \
    libdbus-1-dev libibus-1.0-dev libpipewire-0.3-dev \
    libfontconfig1-dev libfreetype-dev

cd third_party/skia
python3 tools/git-sync-deps
bin/gn gen out/Release --args='is_debug=false skia_use_gl=true'
ninja -C out/Release skia
cd ../..

cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
```

### Run

```sh
./build/make-fake-trace build/fake.winscope
./build/layerviewer build/fake.winscope
# or: ./build/layerviewer  and drag a .winscope onto the window
```

## Layout

```
aosp/
  shim/        replacement headers (log, cutils, hardware, system/graphics…)
  libutils/    RefBase, sp<>, String8, Threads, Timers, …
  libbase/     android-base/{strings, file, unique_fd, …}
  libmath/     header-only vec/mat/quat/half
  arect/       <android/rect.h>
  ftl/         SF's internal templates (small_vector, flags, enum, …)
  libui/       value types: Rect, Region, Transform, FloatRect, …
  perfetto_protos/    SF layer-trace .proto schema + protoc-generated C++
third_party/
  skia/ SDL/ imgui/   git submodules
  aosp/        shallow clones of AOSP repos (gitignored; edits go to aosp/)
```

AOSP submodule checkouts live under `third_party/aosp/` (gitignored).
Files we actually compile are copied into `aosp/` and tracked, so edits
are explicit and the tree stays small.

## License

[MIT](LICENSE)
