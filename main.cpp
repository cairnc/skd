#include <SDL3/SDL.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include "imgui_impl_opengl3_loader.h"
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"

#include "layer_trace.h"
#include "protos/perfetto/trace/android/surfaceflinger_common.pb.h"
#include "protos/perfetto/trace/android/surfaceflinger_layers.pb.h"

using LayerProto = perfetto::protos::LayerProto;
using LayersSnapshotProto = perfetto::protos::LayersSnapshotProto;

// Skia draws into the default framebuffer (FBO 0). ImGui draws on top in the
// same frame. Surface is recreated on resize.
static sk_sp<SkSurface> MakeWindowSurface(GrDirectContext *ctx, int w, int h)
{
    GrGLFramebufferInfo fb_info;
    fb_info.fFBOID = 0;
    fb_info.fFormat = GL_RGBA8;
    GrBackendRenderTarget target =
        GrBackendRenderTargets::MakeGL(w, h, 0, 8, fb_info);
    return SkSurfaces::WrapBackendRenderTarget(
        ctx, target, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(), nullptr);
}

// Screen-bounds drawing: AOSP layer traces are typically in device pixels at
// the physical display size. We fit the composite region into the Skia canvas
// with uniform scaling + centering. Returns the transform applied so a ruler
// or pointer can use the same mapping later.
struct FitTransform
{
    float scale = 1.f;
    float tx = 0.f, ty = 0.f;
};

static FitTransform FitRectToCanvas(float rw, float rh, float cw, float ch,
                                    float pad)
{
    FitTransform t;
    float avail_w = std::max(1.f, cw - 2 * pad);
    float avail_h = std::max(1.f, ch - 2 * pad);
    if (rw <= 0 || rh <= 0)
        return t;
    t.scale = std::min(avail_w / rw, avail_h / rh);
    t.tx = pad + (avail_w - rw * t.scale) * 0.5f;
    t.ty = pad + (avail_h - rh * t.scale) * 0.5f;
    return t;
}

struct AppState
{
    std::unique_ptr<layerviewer::LayerTrace> trace;
    int frame = 0;
    int selected_layer_id = -1;
    bool show_invisible = false;
};

// Build id → LayerProto* map for the current snapshot.
static std::unordered_map<int, const LayerProto *>
IndexLayers(const LayersSnapshotProto &snap)
{
    std::unordered_map<int, const LayerProto *> out;
    if (!snap.has_layers())
        return out;
    for (const LayerProto &l : snap.layers().layers())
        out[l.id()] = &l;
    return out;
}

static bool LayerIsVisible(const LayerProto &l)
{
    // Heuristic matching winscope's notion of visibility.
    if (l.flags() & 0x01) // hidden
        return false;
    if (l.has_color() && l.color().has_a() && l.color().a() <= 0.f)
        return false;
    if (!l.has_screen_bounds())
        return false;
    const auto &b = l.screen_bounds();
    return b.right() > b.left() && b.bottom() > b.top();
}

static void
DrawLayerTreeNode(const LayerProto &layer,
                  const std::unordered_map<int, const LayerProto *> &by_id,
                  const std::unordered_map<int, std::vector<int>> &children,
                  AppState &app)
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
    auto it = children.find(layer.id());
    bool has_children = it != children.end() && !it->second.empty();
    if (!has_children)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (app.selected_layer_id == layer.id())
        flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(layer.id());
    bool open = ImGui::TreeNodeEx("##n", flags, "#%d %s", layer.id(),
                                  layer.has_name() ? layer.name().c_str()
                                                   : "(unnamed)");
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        app.selected_layer_id = layer.id();
    if (open)
    {
        if (has_children)
        {
            for (int cid : it->second)
            {
                auto cit = by_id.find(cid);
                if (cit != by_id.end())
                    DrawLayerTreeNode(*cit->second, by_id, children, app);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

static void DrawLayerTree(const LayersSnapshotProto &snap, AppState &app)
{
    if (!snap.has_layers())
    {
        ImGui::TextUnformatted("(no layers in snapshot)");
        return;
    }
    auto by_id = IndexLayers(snap);
    std::unordered_map<int, std::vector<int>> children;
    std::vector<int> roots;
    for (const LayerProto &l : snap.layers().layers())
    {
        int parent = l.has_parent() ? l.parent() : -1;
        if (parent >= 0 && by_id.count(parent))
            children[parent].push_back(l.id());
        else
            roots.push_back(l.id());
    }
    for (int id : roots)
        DrawLayerTreeNode(*by_id[id], by_id, children, app);
}

static void DrawLayerInspector(const LayersSnapshotProto &snap,
                               const AppState &app)
{
    if (app.selected_layer_id < 0)
    {
        ImGui::TextUnformatted("Select a layer to inspect.");
        return;
    }
    auto by_id = IndexLayers(snap);
    auto it = by_id.find(app.selected_layer_id);
    if (it == by_id.end())
    {
        ImGui::Text("Layer #%d not in this snapshot.", app.selected_layer_id);
        return;
    }
    const LayerProto &l = *it->second;
    ImGui::Text("id:     %d", l.id());
    ImGui::Text("name:   %s", l.name().c_str());
    ImGui::Text("type:   %s", l.type().c_str());
    ImGui::Text("z:      %d", l.z());
    ImGui::Text("flags:  0x%x", l.flags());
    if (l.has_parent())
        ImGui::Text("parent: %d", l.parent());
    if (l.has_layer_stack())
        ImGui::Text("stack:  %u", l.layer_stack());
    if (l.has_screen_bounds())
    {
        const auto &b = l.screen_bounds();
        ImGui::Text("screen_bounds: [%.1f %.1f %.1f %.1f]", b.left(), b.top(),
                    b.right(), b.bottom());
    }
    if (l.has_size())
        ImGui::Text("size:   %d x %d", l.size().w(), l.size().h());
    if (l.has_color())
    {
        const auto &c = l.color();
        ImGui::Text("color:  %.2f %.2f %.2f a=%.2f", c.r(), c.g(), c.b(),
                    c.a());
    }
    if (l.has_corner_radius())
        ImGui::Text("corner_radius: %.2f", l.corner_radius());
    if (!l.dataspace().empty())
        ImGui::Text("dataspace: %s", l.dataspace().c_str());
    if (!l.pixel_format().empty())
        ImGui::Text("pixel_format: %s", l.pixel_format().c_str());
}

static void DrawBoundsCanvas(SkCanvas *canvas, int fb_w, int fb_h,
                             const LayersSnapshotProto &snap,
                             const AppState &app)
{
    SkPaint bg;
    bg.setColor(SkColorSetARGB(255, 24, 24, 28));
    canvas->drawRect(SkRect::MakeIWH(fb_w, fb_h), bg);

    // Figure out composite area from the first display, or fall back to the
    // union of all layer screen_bounds.
    float cw = 0, ch = 0;
    if (snap.displays_size() > 0 && snap.displays(0).has_size())
    {
        cw = (float)snap.displays(0).size().w();
        ch = (float)snap.displays(0).size().h();
    }
    if (cw <= 0 || ch <= 0)
    {
        for (const LayerProto &l : snap.layers().layers())
        {
            if (!l.has_screen_bounds())
                continue;
            cw = std::max(cw, l.screen_bounds().right());
            ch = std::max(ch, l.screen_bounds().bottom());
        }
    }
    if (cw <= 0 || ch <= 0)
        return;

    FitTransform t = FitRectToCanvas(cw, ch, (float)fb_w, (float)fb_h, 32.f);

    // Device rect.
    SkPaint border;
    border.setStyle(SkPaint::kStroke_Style);
    border.setAntiAlias(true);
    border.setStrokeWidth(1.5f);
    border.setColor(SkColorSetARGB(180, 120, 120, 140));
    canvas->drawRect(SkRect::MakeXYWH(t.tx, t.ty, cw * t.scale, ch * t.scale),
                     border);

    // Draw layers sorted by z (ascending) so higher z paint on top.
    std::vector<const LayerProto *> layers;
    layers.reserve(snap.layers().layers_size());
    for (const LayerProto &l : snap.layers().layers())
        if (app.show_invisible || LayerIsVisible(l))
            layers.push_back(&l);
    std::sort(layers.begin(), layers.end(),
              [](const LayerProto *a, const LayerProto *b)
              { return a->z() < b->z(); });

    SkPaint fill, stroke;
    fill.setStyle(SkPaint::kFill_Style);
    fill.setAntiAlias(true);
    stroke.setStyle(SkPaint::kStroke_Style);
    stroke.setAntiAlias(true);
    stroke.setStrokeWidth(1.f);

    for (const LayerProto *l : layers)
    {
        if (!l->has_screen_bounds())
            continue;
        const auto &b = l->screen_bounds();
        SkRect r = SkRect::MakeLTRB(
            t.tx + b.left() * t.scale, t.ty + b.top() * t.scale,
            t.tx + b.right() * t.scale, t.ty + b.bottom() * t.scale);
        SkColor4f c = {0.4f, 0.6f, 0.95f, 0.12f};
        if (l->has_color())
        {
            const auto &lc = l->color();
            c = {lc.r(), lc.g(), lc.b(),
                 std::min(0.25f, std::max(0.04f, lc.a() * 0.2f))};
        }
        fill.setColor4f(c, nullptr);
        canvas->drawRect(r, fill);

        SkColor4f sc = {c.fR, c.fG, c.fB, 0.7f};
        if (app.selected_layer_id == l->id())
        {
            sc = {1.f, 0.85f, 0.2f, 1.f};
            stroke.setStrokeWidth(2.5f);
        }
        else
        {
            stroke.setStrokeWidth(1.f);
        }
        stroke.setColor4f(sc, nullptr);
        canvas->drawRect(r, stroke);
    }
}

int main(int argc, char **argv)
{
    AppState app;
    if (argc >= 2)
    {
        app.trace = layerviewer::LoadLayerTrace(argv[1]);
        if (!app.trace->error.empty())
            std::fprintf(stderr, "trace load failed: %s\n",
                         app.trace->error.c_str());
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "layerviewer",
                                 SDL_GetError(), nullptr);
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    SDL_Window *window =
        SDL_CreateWindow("layerviewer", 1440, 900,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                             SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "layerviewer",
                                 SDL_GetError(), nullptr);
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "layerviewer",
                                 SDL_GetError(), window);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1);

    sk_sp<const GrGLInterface> gl_interface = GrGLMakeNativeInterface();
    if (!gl_interface)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "layerviewer",
                                 "GrGLMakeNativeInterface failed", window);
        return 1;
    }
    sk_sp<GrDirectContext> gr_ctx = GrDirectContexts::MakeGL(gl_interface);
    if (!gr_ctx)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "layerviewer",
                                 "GrDirectContexts::MakeGL failed", window);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 150");

    sk_sp<SkSurface> surface;
    int surf_w = 0, surf_h = 0;

    bool quit = false;
    while (!quit)
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT)
                quit = true;
            else if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                     ev.window.windowID == SDL_GetWindowID(window))
                quit = true;
            else if (ev.type == SDL_EVENT_DROP_FILE)
            {
                app.trace = layerviewer::LoadLayerTrace(ev.drop.data);
                app.frame = 0;
                app.selected_layer_id = -1;
            }
        }

        int fb_w, fb_h;
        SDL_GetWindowSizeInPixels(window, &fb_w, &fb_h);
        if (fb_w != surf_w || fb_h != surf_h || !surface)
        {
            surface = MakeWindowSurface(gr_ctx.get(), fb_w, fb_h);
            surf_w = fb_w;
            surf_h = fb_h;
        }

        SkCanvas *canvas = surface->getCanvas();
        if (app.trace && app.trace->proto && app.trace->snapshot_count() > 0)
        {
            app.frame =
                std::clamp(app.frame, 0, app.trace->snapshot_count() - 1);
            DrawBoundsCanvas(canvas, fb_w, fb_h, app.trace->snapshot(app.frame),
                             app);
        }
        else
        {
            canvas->clear(SkColorSetARGB(255, 24, 24, 28));
        }

        gr_ctx->flushAndSubmit();
        gr_ctx->resetContext();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

        if (ImGui::Begin("Trace"))
        {
            if (!app.trace)
            {
                ImGui::TextUnformatted("Drop a .winscope / .perfetto layers "
                                       "trace file on the window, or pass "
                                       "one on the command line.");
            }
            else if (!app.trace->error.empty())
            {
                ImGui::Text("Load error: %s", app.trace->error.c_str());
            }
            else
            {
                ImGui::Text("File: %s", app.trace->path.c_str());
                ImGui::Text("Snapshots: %d", app.trace->snapshot_count());
                if (app.trace->snapshot_count() > 1)
                    ImGui::SliderInt("frame", &app.frame, 0,
                                     app.trace->snapshot_count() - 1);
                ImGui::Checkbox("show invisible layers", &app.show_invisible);
                if (app.trace->snapshot_count() > 0)
                {
                    const auto &snap = app.trace->snapshot(app.frame);
                    if (snap.has_elapsed_realtime_nanos())
                        ImGui::Text("elapsed: %.3f s",
                                    snap.elapsed_realtime_nanos() / 1e9);
                    if (!snap.where().empty())
                        ImGui::Text("where:   %s", snap.where().c_str());
                    if (snap.has_layers())
                        ImGui::Text("layers:  %d", snap.layers().layers_size());
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("Layers") && app.trace && app.trace->proto &&
            app.trace->snapshot_count() > 0)
            DrawLayerTree(app.trace->snapshot(app.frame), app);
        ImGui::End();

        if (ImGui::Begin("Inspector") && app.trace && app.trace->proto &&
            app.trace->snapshot_count() > 0)
            DrawLayerInspector(app.trace->snapshot(app.frame), app);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    surface.reset();
    gr_ctx.reset();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
