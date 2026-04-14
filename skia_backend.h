#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkFlattenable.h"
#include "include/core/SkImage.h"
#include "include/core/SkM44.h"
#include "include/core/SkPicture.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"

#include <functional>
#include <string>
#include <vector>

typedef unsigned int GLuint;

struct SkiaBackend
{
    sk_sp<GrDirectContext> context;
    GLuint fbo = 0;
    GLuint color_tex = 0;
    int tex_width = 0;
    int tex_height = 0;
    sk_sp<SkSurface> surface;
    std::vector<uint8_t> pixels;

    bool Init();
    void Shutdown();
    void AllocateTarget(int width, int height);
    void RebindSurface();
    SkCanvas *GetCanvas()
    {
        return surface ? surface->getCanvas() : nullptr;
    }
    void Flush();
    void ReadbackPixels();
};

struct ImageViewer
{
    float zoom = 1.0f;
    float pan_x = 0.0f;
    float pan_y = 0.0f;
    float last_view_w = 0;
    float last_view_h = 0;
    int last_img_w = 0;
    int last_img_h = 0;
    bool focused = false;
    int hover_px = -1;
    int hover_py = -1;
    uint32_t hover_color = 0;

    void Reset()
    {
        zoom = 1.0f;
        pan_x = 0.0f;
        pan_y = 0.0f;
    }
    void ZoomTo(float z)
    {
        zoom = fmaxf(0.1f, fminf(z, 32.0f));
    }
    void ZoomBy(float factor)
    {
        ZoomTo(zoom * factor);
    }
    void Fit(float view_w, float view_h, int img_w, int img_h);
    void Clamp(float view_w, float view_h, int img_w, int img_h);
    // flip_y=true: texture origin is bottom-left (GL FBO). flip_y=false:
    // texture uploaded via glTexImage2D with row 0 = image top.
    void DrawImage(GLuint tex, int tex_w, int tex_h,
                   const uint8_t *pixels = nullptr, bool flip_y = true);
    bool IsFocused() const
    {
        return focused;
    }
};

struct ResourceInfo
{
    enum Type
    {
        Shader,
        Image
    };
    Type type;
    uint32_t id;
    std::string type_name;
    sk_sp<SkImage> image; // for Image type
    const void *effect;   // for View button dedup
};

// Accumulates indented text lines for dump export. Not virtual -- commands
// and helpers call methods directly.
struct TextWriter
{
    std::string out;
    int depth = 1; // always indent at least once under the command header

    void Indent();
    void Push(const char *label);
    void Pop();

#ifdef _MSC_VER
    void Line(const char *fmt, ...);
#else
    void Line(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#endif
};

// Kind of a recorded command. Used where we need to behave differently
// based on the command type (group pair-matching, annotation filtering,
// etc.) — faster and typo-proof vs. comparing `name` strings.
enum class CommandKind
{
    Generic,
    Save,
    SaveLayer,
    Restore,
    OffscreenLayer,
    EndOffscreenLayer,
    Annotation,
};

struct CommandInfo
{
    std::string name;
    std::function<void()> ui;
    std::function<void(TextWriter &)> text;
    int indent = 0; // save/restore + offscreen-layer nesting depth
    CommandKind kind = CommandKind::Generic;
    // For Save / SaveLayer / OffscreenLayer: index of matching close
    // (Restore / EndOffscreenLayer). -1 otherwise.
    int range_end = -1;
    // -1 = belongs to main picture. Otherwise, index into
    // DebugCanvas::GetLayers() identifying the owning offscreen layer.
    int layer = -1;
    // Position of this command within its owning picture's playback
    // stream (= number of skip()-eligible SkCanvas calls preceded).
    // Used as stopIndex when replaying. Different from the flat command
    // index because layer cmds are inlined into the flat list but live
    // in their own picture stream, and synthetic EndOffscreenLayer
    // markers don't correspond to any SkCanvas call.
    int play_index = 0;
};

// Android HWUI emits paired annotations around offscreen layer draws in
// MSKP captures:
//   drawAnnotation(rect, "OffscreenLayerDraw|<nodeId>", null)
//   drawPicture(layerPic)     <-- child picture = layer's draw commands
// We parse those out during collection, inline the layer's commands into
// the flat command list bracketed by "OffscreenLayer" / "EndOffscreenLayer"
// markers, and keep a LayerSpan so the viewport can switch its render
// target to the layer's bounds when stepping inside the span.
struct LayerSpan
{
    int id;                   // RenderNode id from the annotation
    SkIRect dirty;            // annotation's dirty rect (parent-space)
    sk_sp<SkPicture> picture; // the child picture: use cullRect for size
    int begin_cmd;            // flat-list index of "OffscreenLayer" marker
    int end_cmd;              // flat-list index of "EndOffscreenLayer"
    int inner_start;          // first inlined child command (= begin_cmd + 1)
    // Cached fully-rendered image of the layer. Filled lazily by
    // DebugCanvas::DrawToPicture so the SurfaceID|<id> + drawImageRect
    // composite in the main picture can substitute this image for the
    // dummy that Android wrote at capture time.
    sk_sp<SkImage> cached_image;
};

class DebugCanvas
{
  public:
    void CollectFromPicture(const SkPicture *picture);
    void DrawToPicture(SkCanvas *canvas, const SkPicture *picture,
                       int stopIndex, bool show_clip = false,
                       bool highlight_geometry = false);
    // Replay an offscreen layer's captured SkPicture to `canvas` (sized to
    // the layer's cullRect), stopping at relStopIndex commands in. Used
    // when the UI has selected a command whose CommandInfo::layer != -1.
    void DrawLayerToPicture(SkCanvas *canvas, const LayerSpan &layer,
                            int relStopIndex, bool show_clip = false,
                            bool highlight_geometry = false);

    int GetCommandCount() const
    {
        return (int)commands_.size();
    }
    std::vector<CommandInfo> &GetCommands()
    {
        return commands_;
    }
    std::vector<ResourceInfo> &GetResources()
    {
        return resources_;
    }
    std::vector<LayerSpan> &GetLayers()
    {
        return layers_;
    }

  private:
    std::vector<CommandInfo> commands_;
    std::vector<ResourceInfo> resources_;
    std::vector<LayerSpan> layers_;
};

struct PictureFrame
{
    sk_sp<SkPicture> picture;
    float width = 0;
    float height = 0;
};

std::vector<PictureFrame> LoadPictures(const char *path);
#ifdef _MSC_VER
std::string sfmt(const char *fmt, ...);
#else
std::string sfmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#endif
bool HasSksl(const SkFlattenable *effect);
std::string ExtractSksl(const SkFlattenable *effect);
void OpenEffectWindow(const SkFlattenable *effect);
void AddImageTile(uint32_t id, const SkImage *img);
void FocusImageTile(uint32_t id, const SkImage *img);
void ResetImageTiles();

// Two-way selection binding with Resources panel.
extern uint32_t g_selected_image;
void DrawShadersPanel(const std::vector<ResourceInfo> &resources);
void DrawImageWindows(const std::vector<ResourceInfo> &resources);

// Draws the pixel magnifier grid. Clicking a cell updates hover_px/hover_py
// in-place so callers see the refined sample position on the next frame.
void DrawPixelMagnifier(int &hover_px, int &hover_py, int tex_w, int tex_h,
                        const uint8_t *pixels, int mag_size, float max_width);

// Render a command's dump callback into a text string.
std::string RenderDetailText(const std::function<void(TextWriter &)> &detail);
