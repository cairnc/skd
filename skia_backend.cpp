#include "skia_backend.h"

#include "imgui.h"
#include "include/core/SkBlender.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkShader.h"
#include "include/effects/SkRuntimeEffect.h"
#include "src/shaders/SkShaderBase.h"
#include "src/shaders/SkBlendShader.h"
#include "src/effects/colorfilters/SkColorFilterBase.h"
#include "src/effects/colorfilters/SkComposeColorFilter.h"
#include "src/core/SkImageFilter_Base.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSerialProcs.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkVertices.h"
#include "src/core/SkVerticesPriv.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/docs/SkMultiPictureDocument.h"
#include "include/core/SkPictureRecorder.h"

#define GL_SILENCE_DEPRECATION
#include "gl_extras.h"

#include "include/core/SkData.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <set>

constexpr float kMagnifierCellSize = 16.0f;
constexpr float kViewportPanelWidth = 180.0f;
constexpr float kPanMargin = 32.0f;
constexpr int kViewportMagnifierSize = 11;
constexpr int kImageMagnifierSize = 9;
constexpr int kCheckerTileSize = 16;

std::string sfmt(const char *fmt, ...)
{
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int n = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    std::string s(n, '\0');
    vsnprintf(s.data(), n + 1, fmt, args2);
    va_end(args2);
    return s;
}

// When set, DrawShadersPanel scrolls to and focuses this shader.
static uint32_t g_scroll_to_shader = 0;

std::string ExtractSksl(const SkFlattenable *effect)
{
    if (!effect)
        return {};
    SkFlattenable::Type type = effect->getFlattenableType();
    if (type == SkFlattenable::kSkShader_Type)
    {
        const SkShaderBase *sb = as_SB(static_cast<const SkShader *>(effect));
        SkRuntimeEffect *rt = sb->asRuntimeEffect();
        if (rt)
            return std::string(rt->source());
    }
    return {};
}

static uint32_t GetEffectId(const SkFlattenable *effect)
{
    switch (effect->getFlattenableType())
    {
    case SkFlattenable::kSkShader_Type:
        return as_SB(static_cast<const SkShader *>(effect))->uniqueID();
    case SkFlattenable::kSkImageFilter_Type:
        return static_cast<const SkImageFilter_Base *>(
                   static_cast<const SkImageFilter *>(effect))
            ->uniqueID();
    default:
        return 0;
    }
}

void OpenEffectWindow(const SkFlattenable *effect)
{
    g_scroll_to_shader = GetEffectId(effect);
    ImGui::SetWindowFocus("Shaders");
}

bool HasSksl(const SkFlattenable *effect)
{
    return !ExtractSksl(effect).empty();
}

bool SkiaBackend::Init()
{
    sk_sp<const GrGLInterface> interface = GrGLMakeNativeInterface();
    if (!interface)
        return false;
    context = GrDirectContexts::MakeGL(interface);
    return context != nullptr;
}

void SkiaBackend::Shutdown()
{
    surface.reset();
    if (fbo)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (color_tex)
    {
        glDeleteTextures(1, &color_tex);
        color_tex = 0;
    }
    context.reset();
}

void SkiaBackend::AllocateTarget(int width, int height)
{
    if (width == tex_width && height == tex_height)
        return;

    surface.reset();
    if (fbo)
        glDeleteFramebuffers(1, &fbo);
    if (color_tex)
        glDeleteTextures(1, &color_tex);

    glGenTextures(1, &color_tex);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           color_tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    tex_width = width;
    tex_height = height;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GrGLFramebufferInfo fb_info;
    fb_info.fFBOID = fbo;
    fb_info.fFormat = GL_RGBA8;

    GrBackendRenderTarget target =
        GrBackendRenderTargets::MakeGL(width, height, 0, 8, fb_info);
    surface = SkSurfaces::WrapBackendRenderTarget(
        context.get(), target, kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SkiaBackend::RebindSurface()
{
    surface.reset();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GrGLFramebufferInfo fb_info;
    fb_info.fFBOID = fbo;
    fb_info.fFormat = GL_RGBA8;

    GrBackendRenderTarget target =
        GrBackendRenderTargets::MakeGL(tex_width, tex_height, 0, 8, fb_info);
    surface = SkSurfaces::WrapBackendRenderTarget(
        context.get(), target, kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
}

void SkiaBackend::Flush()
{
    context->flushAndSubmit();
}

void SkiaBackend::ReadbackPixels()
{
    if (tex_width <= 0 || tex_height <= 0)
        return;
    pixels.resize(tex_width * tex_height * 4);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // GL reads bottom-up, flip vertically
    int stride = tex_width * 4;
    std::vector<uint8_t> row(stride);
    for (int y = 0; y < tex_height / 2; y++)
    {
        int y2 = tex_height - 1 - y;
        memcpy(row.data(), &pixels[y * stride], stride);
        memcpy(&pixels[y * stride], &pixels[y2 * stride], stride);
        memcpy(&pixels[y2 * stride], row.data(), stride);
    }
}

std::vector<PictureFrame> LoadPictures(const char *path)
{
    std::vector<PictureFrame> frames;
    std::unique_ptr<SkFILEStream> stream = SkFILEStream::Make(path);
    if (!stream || !stream->isValid())
        return frames;

    SkDeserialProcs dprocs;
    dprocs.fImageProc = [](const void *data, size_t len,
                           void *) -> sk_sp<SkImage>
    {
        return SkImages::DeferredFromEncodedData(
            SkData::MakeWithCopy(data, len));
    };

    // Detect .mskp (Multi-Picture Document) by magic header.
    static const char kMSKPMagic[] = "Skia Multi-Picture Doc\n\n";
    constexpr size_t kMagicLen = sizeof(kMSKPMagic) - 1;
    char header[kMagicLen];
    if (stream->read(header, kMagicLen) == kMagicLen &&
        memcmp(header, kMSKPMagic, kMagicLen) == 0)
    {
        stream->rewind();
        int n = SkMultiPictureDocument::ReadPageCount(stream.get());
        if (n <= 0)
            return frames;
        std::vector<SkDocumentPage> pages(n);
        if (!SkMultiPictureDocument::Read(stream.get(), pages.data(), n,
                                          &dprocs))
            return frames;
        frames.reserve(n);
        for (const auto &p : pages)
            frames.push_back({p.fPicture, p.fSize.width(), p.fSize.height()});
        return frames;
    }

    stream->rewind();
    sk_sp<SkPicture> pic = SkPicture::MakeFromStream(stream.get(), &dprocs);
    if (pic)
    {
        SkRect cull = pic->cullRect();
        frames.push_back({pic, cull.width(), cull.height()});
    }
    return frames;
}

static const char *StyleName(SkPaint::Style s)
{
    switch (s)
    {
    case SkPaint::kFill_Style:
        return "Fill";
    case SkPaint::kStroke_Style:
        return "Stroke";
    case SkPaint::kStrokeAndFill_Style:
        return "StrokeAndFill";
    }
    return "?";
}

static const char *CapName(SkPaint::Cap c)
{
    switch (c)
    {
    case SkPaint::kButt_Cap:
        return "Butt";
    case SkPaint::kRound_Cap:
        return "Round";
    case SkPaint::kSquare_Cap:
        return "Square";
    }
    return "?";
}

static const char *JoinName(SkPaint::Join j)
{
    switch (j)
    {
    case SkPaint::kMiter_Join:
        return "Miter";
    case SkPaint::kRound_Join:
        return "Round";
    case SkPaint::kBevel_Join:
        return "Bevel";
    }
    return "?";
}

static const char *ClipOpName(SkClipOp op)
{
    switch (op)
    {
    case SkClipOp::kDifference:
        return "Difference";
    case SkClipOp::kIntersect:
        return "Intersect";
    }
    return "?";
}

void TextWriter::Indent()
{
    for (int i = 0; i < depth; i++)
        out += "  ";
}

void TextWriter::Push(const char *label)
{
    Indent();
    out += label;
    out += ":\n";
    depth++;
}

void TextWriter::Pop()
{
    depth--;
}

void TextWriter::Line(const char *fmt, ...)
{
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int n = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    std::string s(n, '\0');
    vsnprintf(s.data(), n + 1, fmt, args2);
    va_end(args2);
    Indent();
    out += s;
    out += '\n';
}

// ---- ImGui detail helpers (render directly to current ImGui window) ----

static void UiRect(const char *label, const SkRect &r)
{
    ImGui::Text("%s: [%.1f, %.1f, %.1f, %.1f] (%.1fx%.1f)", label, r.fLeft,
                r.fTop, r.fRight, r.fBottom, r.width(), r.height());
}

static void UiViewButton(const SkFlattenable *effect)
{
    if (!HasSksl(effect))
        return;
    ImGui::SameLine();
    ImGui::PushID(effect);
    if (ImGui::Button("View"))
        OpenEffectWindow(effect);
    ImGui::PopID();
}

// Renders an "Image #id  WxH" line with a "View" button next to it that
// opens (or focuses) the image in the Images panel. Used both for direct
// image arguments (drawImageRect etc.) and for images discovered inside
// a paint via serialize traversal.
static void UiImageButton(const SkImage *img)
{
    if (!img)
        return;
    uint32_t id = img->uniqueID();
    ImGui::Text("Image #%u  %dx%d", id, img->width(), img->height());
    ImGui::SameLine();
    ImGui::PushID((const void *)img);
    if (ImGui::Button("View"))
        FocusImageTile(id, img);
    ImGui::PopID();
}

// Walk a flattenable via serialize and render a clickable button for
// every SkImage Skia reports along the way. Returns the count so callers
// can decide whether to add a heading.
static int UiImagesFrom(const SkFlattenable *f)
{
    if (!f)
        return 0;
    std::vector<sk_sp<SkImage>> imgs;
    SkSerialProcs procs;
    procs.fImageCtx = &imgs;
    procs.fImageProc = [](SkImage *img, void *ctx) -> sk_sp<const SkData>
    {
        static_cast<std::vector<sk_sp<SkImage>> *>(ctx)->push_back(
            sk_ref_sp(img));
        return nullptr;
    };
    (void)f->serialize(&procs);
    for (const auto &i : imgs)
        UiImageButton(i.get());
    return (int)imgs.size();
}

static void UiColor(const SkColor4f &c)
{
    ImVec4 col(c.fR, c.fG, c.fB, c.fA);
    ImGui::PushID(&c);
    ImGui::ColorButton("##color", col, ImGuiColorEditFlags_AlphaPreview,
                       ImVec2(14, 14));
    ImGui::SameLine();
    if (ImGui::Button("Copy"))
        ImGui::SetClipboardText(sfmt("#%08X", c.toSkColor()).c_str());
    ImGui::PopID();
}

static void UiShader(const char *label, const SkShader *sh);
static void UiColorFilter(const char *label, const SkColorFilter *cf);
static void UiImageFilter(const char *label, const SkImageFilter *f);

template <typename T>
static void UiEffectLeaf(const char *label, const T *effect)
{
    if (!effect)
        return;
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Type: %s", effect->getTypeName());
        UiViewButton(effect);
        UiImagesFrom(effect);
        ImGui::TreePop();
    }
}

static void UiShader(const char *label, const SkShader *sh)
{
    if (!sh)
        return;
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Type: %s", sh->getTypeName());
        UiViewButton(sh);
        UiImagesFrom(sh);

        const SkShaderBase *sb = as_SB(sh);
        if (sb->type() == SkShaderBase::ShaderType::kBlend)
        {
            const SkBlendShader *blend = static_cast<const SkBlendShader *>(sb);
            ImGui::Text("BlendMode: %d", (int)blend->mode());
            UiShader("Dst", blend->dst().get());
            UiShader("Src", blend->src().get());
        }
        ImGui::TreePop();
    }
}

static void UiColorFilter(const char *label, const SkColorFilter *cf)
{
    if (!cf)
        return;
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Type: %s", cf->getTypeName());
        UiViewButton(cf);
        UiImagesFrom(cf);

        const SkColorFilterBase *base =
            static_cast<const SkColorFilterBase *>(cf);
        if (base->type() == SkColorFilterBase::Type::kCompose)
        {
            const SkComposeColorFilter *comp =
                static_cast<const SkComposeColorFilter *>(base);
            UiColorFilter("Outer", comp->outer().get());
            UiColorFilter("Inner", comp->inner().get());
        }
        ImGui::TreePop();
    }
}

static void UiImageFilter(const char *label, const SkImageFilter *f)
{
    if (!f)
        return;
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Type: %s", f->getTypeName());
        UiViewButton(f);
        UiImagesFrom(f);
        for (int i = 0; i < f->countInputs(); i++)
        {
            std::string child = sfmt("Input %d", i);
            UiImageFilter(child.c_str(), f->getInput(i));
        }
        ImGui::TreePop();
    }
}

static void UiPaint(const SkPaint &p)
{
    if (!ImGui::TreeNodeEx("Paint", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    UiColor(p.getColor4f());
    ImGui::Text("Style: %s", StyleName(p.getStyle()));
    if (p.getStyle() != SkPaint::kFill_Style)
    {
        ImGui::Text("StrokeWidth: %.2f", p.getStrokeWidth());
        ImGui::Text("Cap: %s", CapName(p.getStrokeCap()));
        ImGui::Text("Join: %s", JoinName(p.getStrokeJoin()));
        ImGui::Text("Miter: %.2f", p.getStrokeMiter());
    }
    ImGui::Text("AntiAlias: %s", p.isAntiAlias() ? "true" : "false");
    std::optional<SkBlendMode> bm = p.asBlendMode();
    if (bm && *bm != SkBlendMode::kSrcOver)
        ImGui::Text("BlendMode: %d", (int)*bm);

    UiShader("Shader", p.getShader());
    UiColorFilter("ColorFilter", p.getColorFilter());
    UiImageFilter("ImageFilter", p.getImageFilter());
    UiEffectLeaf("MaskFilter", p.getMaskFilter());
    UiEffectLeaf("PathEffect", p.getPathEffect());
    UiEffectLeaf("Blender", p.getBlender());

    ImGui::TreePop();
}

static void UiPath(const SkPath &path)
{
    if (!ImGui::TreeNodeEx("Path", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    ImGui::Text("Verbs: %d", path.countVerbs());
    ImGui::Text("Points: %d", path.countPoints());
    ImGui::Text("FillType: %d", (int)path.getFillType());
    UiRect("Bounds", path.getBounds());
    ImGui::TreePop();
}

static void UiM44(const char *label, const SkM44 &m)
{
    if (!ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
        return;
    float v[16];
    m.getColMajor(v);
    ImGui::Text("[%7.2f %7.2f %7.2f %7.2f]", v[0], v[4], v[8], v[12]);
    ImGui::Text("[%7.2f %7.2f %7.2f %7.2f]", v[1], v[5], v[9], v[13]);
    ImGui::Text("[%7.2f %7.2f %7.2f %7.2f]", v[2], v[6], v[10], v[14]);
    ImGui::Text("[%7.2f %7.2f %7.2f %7.2f]", v[3], v[7], v[11], v[15]);
    ImGui::TreePop();
}

// ---- Text helpers (append to TextWriter for dump export) ----

static void TxRect(TextWriter &w, const char *label, const SkRect &r)
{
    w.Line("%s: [%.1f, %.1f, %.1f, %.1f] (%.1fx%.1f)", label, r.fLeft, r.fTop,
           r.fRight, r.fBottom, r.width(), r.height());
}

static void TxColor(TextWriter &w, const SkColor4f &c)
{
    w.Line("Color: #%08X (%.2f, %.2f, %.2f, %.2f)", c.toSkColor(), c.fR, c.fG,
           c.fB, c.fA);
}

static void TxShader(TextWriter &w, const char *label, const SkShader *sh);
static void TxColorFilter(TextWriter &w, const char *label,
                          const SkColorFilter *cf);
static void TxImageFilter(TextWriter &w, const char *label,
                          const SkImageFilter *f);

template <typename T>
static void TxEffectLeaf(TextWriter &w, const char *label, const T *effect)
{
    if (!effect)
        return;
    w.Push(label);
    w.Line("Type: %s", effect->getTypeName());
    w.Pop();
}

static void TxShader(TextWriter &w, const char *label, const SkShader *sh)
{
    if (!sh)
        return;
    w.Push(label);
    w.Line("Type: %s", sh->getTypeName());
    const SkShaderBase *sb = as_SB(sh);
    if (sb->type() == SkShaderBase::ShaderType::kBlend)
    {
        const SkBlendShader *blend = static_cast<const SkBlendShader *>(sb);
        w.Line("BlendMode: %d", (int)blend->mode());
        TxShader(w, "Dst", blend->dst().get());
        TxShader(w, "Src", blend->src().get());
    }
    w.Pop();
}

static void TxColorFilter(TextWriter &w, const char *label,
                          const SkColorFilter *cf)
{
    if (!cf)
        return;
    w.Push(label);
    w.Line("Type: %s", cf->getTypeName());
    const SkColorFilterBase *base = static_cast<const SkColorFilterBase *>(cf);
    if (base->type() == SkColorFilterBase::Type::kCompose)
    {
        const SkComposeColorFilter *comp =
            static_cast<const SkComposeColorFilter *>(base);
        TxColorFilter(w, "Outer", comp->outer().get());
        TxColorFilter(w, "Inner", comp->inner().get());
    }
    w.Pop();
}

static void TxImageFilter(TextWriter &w, const char *label,
                          const SkImageFilter *f)
{
    if (!f)
        return;
    w.Push(label);
    w.Line("Type: %s", f->getTypeName());
    for (int i = 0; i < f->countInputs(); i++)
    {
        std::string child = sfmt("Input %d", i);
        TxImageFilter(w, child.c_str(), f->getInput(i));
    }
    w.Pop();
}

static void TxPaint(TextWriter &w, const SkPaint &p)
{
    w.Push("Paint");
    TxColor(w, p.getColor4f());
    w.Line("Style: %s", StyleName(p.getStyle()));
    if (p.getStyle() != SkPaint::kFill_Style)
    {
        w.Line("StrokeWidth: %.2f", p.getStrokeWidth());
        w.Line("Cap: %s", CapName(p.getStrokeCap()));
        w.Line("Join: %s", JoinName(p.getStrokeJoin()));
        w.Line("Miter: %.2f", p.getStrokeMiter());
    }
    w.Line("AntiAlias: %s", p.isAntiAlias() ? "true" : "false");
    std::optional<SkBlendMode> bm = p.asBlendMode();
    if (bm && *bm != SkBlendMode::kSrcOver)
        w.Line("BlendMode: %d", (int)*bm);

    TxShader(w, "Shader", p.getShader());
    TxColorFilter(w, "ColorFilter", p.getColorFilter());
    TxImageFilter(w, "ImageFilter", p.getImageFilter());
    TxEffectLeaf(w, "MaskFilter", p.getMaskFilter());
    TxEffectLeaf(w, "PathEffect", p.getPathEffect());
    TxEffectLeaf(w, "Blender", p.getBlender());

    w.Pop();
}

static void TxPath(TextWriter &w, const SkPath &path)
{
    w.Push("Path");
    w.Line("Verbs: %d", path.countVerbs());
    w.Line("Points: %d", path.countPoints());
    w.Line("FillType: %d", (int)path.getFillType());
    TxRect(w, "Bounds", path.getBounds());
    w.Pop();
}

static void TxM44(TextWriter &w, const char *label, const SkM44 &m)
{
    w.Push(label);
    float v[16];
    m.getColMajor(v);
    w.Line("[%7.2f %7.2f %7.2f %7.2f]", v[0], v[4], v[8], v[12]);
    w.Line("[%7.2f %7.2f %7.2f %7.2f]", v[1], v[5], v[9], v[13]);
    w.Line("[%7.2f %7.2f %7.2f %7.2f]", v[2], v[6], v[10], v[14]);
    w.Line("[%7.2f %7.2f %7.2f %7.2f]", v[3], v[7], v[11], v[15]);
    w.Pop();
}

std::string RenderDetailText(const std::function<void(TextWriter &)> &detail)
{
    if (!detail)
        return {};
    TextWriter w;
    detail(w);
    return std::move(w.out);
}

class InterceptCanvas : public SkCanvas
{
  public:
    InterceptCanvas(int w, int h, SkCanvas *target, int stopIndex,
                    bool highlight_geometry = false,
                    const std::vector<LayerSpan> *external_layers = nullptr)
        : SkCanvas(w, h), target(target), stop_index(stopIndex),
          highlight(highlight_geometry), external_layers(external_layers)
    {
    }

    std::vector<CommandInfo> commands;
    std::vector<ResourceInfo> resources;
    std::vector<LayerSpan> layers;
    // DrawLayerImage UI closures capture one of these shared boxes;
    // CollectFromPicture fills them in after rendering the layers.
    struct PendingLayerImageBox
    {
        int layer_id;
        std::shared_ptr<sk_sp<SkImage>> box;
    };
    std::vector<PendingLayerImageBox> pending_layer_image_boxes;

  private:
    SkCanvas *target;
    int stop_index;
    int index = 0;
    int depth = 0; // current save/restore depth (stamped on each command)
    bool highlight = false;
    std::set<uint32_t> seen_shaders;
    std::set<uint32_t> seen_images;
    // Android MSKP offscreen-layer state. pending_layer_id is set by the
    // OffscreenLayerDraw annotation and consumed by the next drawPicture.
    int pending_layer_id = -1;
    SkIRect pending_layer_dirty = SkIRect::MakeEmpty();
    // pending_image_layer_id is set by the SurfaceID annotation and
    // consumed by the next drawImageRect (the layer composite).
    int pending_image_layer_id = -1;
    // Stamped on every recorded command so the viewport can route replay
    // into the right layer picture. -1 = main picture.
    int current_layer = -1;
    // Per-stream playback positions. Each record() in the main stream
    // advances main_play_idx; in a layer stream advances layer_play_idx.
    // The synthetic EndOffscreenLayer marker doesn't advance either.
    int main_play_idx = 0;
    int layer_play_idx = 0;
    // Non-null when this canvas is replaying a parent picture and needs
    // to substitute the layer composite drawImageRect with the layer's
    // pre-rendered SkImage. Owned by DebugCanvas; we just borrow.
    const std::vector<LayerSpan> *external_layers = nullptr;

    bool collect()
    {
        return target == nullptr;
    }
    bool skip()
    {
        return stop_index >= 0 && index++ > stop_index;
    }
    // True if the command currently being forwarded is THE selected one.
    // Called from draw overrides after skip() but while forwarding.
    bool isHighlightTarget() const
    {
        return highlight && stop_index >= 0 && index - 1 == stop_index;
    }
    // Solid magenta that ignores the original paint's color/shader/filter —
    // what the user wants to see as the "geometry" of this draw.
    static SkPaint magentaPaint()
    {
        SkPaint p;
        p.setColor(0xFFFF00FF);
        p.setAntiAlias(true);
        return p;
    }
    SkPaint scratch_paint;
    const SkPaint &paintFor(const SkPaint &p)
    {
        if (isHighlightTarget())
        {
            scratch_paint = magentaPaint();
            return scratch_paint;
        }
        return p;
    }
    const SkPaint *paintFor(const SkPaint *p)
    {
        if (isHighlightTarget())
        {
            scratch_paint = magentaPaint();
            return &scratch_paint;
        }
        return p;
    }
    void record(const char *name, std::function<void()> ui = {},
                std::function<void(TextWriter &)> text = {},
                CommandKind kind = CommandKind::Generic)
    {
        int pi = (current_layer == -1) ? main_play_idx++ : layer_play_idx++;
        commands.push_back({name, std::move(ui), std::move(text), depth, kind,
                            -1, current_layer, pi});
    }

    void collectShader(const SkShader *s)
    {
        if (!s)
            return;
        uint32_t id = as_SB(s)->uniqueID();
        if (seen_shaders.count(id))
            return;
        seen_shaders.insert(id);

        // Only list shaders with inspectable SkSL source. Wrapper types
        // (LocalMatrix, gradients, etc.) have no View action so listing
        // them is just noise.
        if (HasSksl(s))
            resources.push_back(
                {ResourceInfo::Shader, id, s->getTypeName(), nullptr, s});

        const SkShaderBase *sb = as_SB(s);
        if (sb->type() == SkShaderBase::ShaderType::kBlend)
        {
            const SkBlendShader *blend = static_cast<const SkBlendShader *>(sb);
            collectShader(blend->dst().get());
            collectShader(blend->src().get());
        }
    }

    void collectImage(const SkImage *img)
    {
        if (!img)
            return;
        uint32_t id = img->uniqueID();
        if (seen_images.count(id))
            return;
        // Probe that the image can actually produce pixels. Skia hands us
        // a placeholder (with a fresh uniqueID) when its deserializer
        // can't decode the bytes — common for Android HWUI captures that
        // embed GPU surface contents as raw pixels rather than an encoded
        // format. These fail readPixels and clutter the Resources list.
        // Keep valid 1x1 images (rare but legal).
        uint32_t tmp = 0;
        SkImageInfo probe_info = SkImageInfo::Make(1, 1, kRGBA_8888_SkColorType,
                                                   kPremul_SkAlphaType);
        SkPixmap probe(probe_info, &tmp, 4);
        if (!img->readPixels(probe, 0, 0))
            return;
        seen_images.insert(id);
        resources.push_back({ResourceInfo::Image, id,
                             sfmt("%dx%d", img->width(), img->height()),
                             sk_ref_sp(img), nullptr});
    }

    // Walk a flattenable via serialize() purely as a traversal mechanism.
    // We return nullptr from the hooks so Skia uses its default path, but
    // we capture every image/typeface/picture Skia reports along the way.
    // Covers images referenced via image shaders, image filters, etc.
    static SkSerialProcs makeCollectProcs(InterceptCanvas *self)
    {
        SkSerialProcs procs;
        procs.fImageCtx = self;
        procs.fImageProc = [](SkImage *img, void *ctx) -> sk_sp<const SkData>
        {
            static_cast<InterceptCanvas *>(ctx)->collectImage(img);
            return nullptr;
        };
        return procs;
    }

    void walkFlattenable(const SkFlattenable *f)
    {
        if (!f)
            return;
        SkSerialProcs procs = makeCollectProcs(this);
        // Discard the returned bytes; we only want the callback to fire.
        (void)f->serialize(&procs);
    }

    void collectPaint(const SkPaint &p)
    {
        collectShader(p.getShader());
        walkFlattenable(p.getShader());
        walkFlattenable(p.getColorFilter());
        walkFlattenable(p.getImageFilter());
        walkFlattenable(p.getMaskFilter());
        walkFlattenable(p.getPathEffect());
        walkFlattenable(p.getBlender());
    }

    void collectPaint(const SkPaint *p)
    {
        if (p)
            collectPaint(*p);
    }

    void onDrawPaint(const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawPaint", [=] { UiPaint(p); },
                [=](TextWriter &w) { TxPaint(w, p); });
            return;
        }
        if (skip())
            return;
        target->drawPaint(paintFor(p));
    }
    void onDrawRect(const SkRect &r, const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawRect",
                [=]
                {
                    UiRect("Rect", r);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Rect", r);
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawRect(r, paintFor(p));
    }
    void onDrawRRect(const SkRRect &rr, const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawRRect",
                [=]
                {
                    UiRect("Rect", rr.rect());
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Rect", rr.rect());
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawRRect(rr, paintFor(p));
    }
    void onDrawDRRect(const SkRRect &o, const SkRRect &i,
                      const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawDRRect",
                [=]
                {
                    UiRect("Outer", o.rect());
                    UiRect("Inner", i.rect());
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Outer", o.rect());
                    TxRect(w, "Inner", i.rect());
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawDRRect(o, i, paintFor(p));
    }
    void onDrawOval(const SkRect &r, const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawOval",
                [=]
                {
                    UiRect("Oval", r);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Oval", r);
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawOval(r, paintFor(p));
    }
    void onDrawArc(const SkRect &r, SkScalar sa, SkScalar sw, bool uc,
                   const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawArc",
                [=]
                {
                    UiRect("Oval", r);
                    ImGui::Text("Start: %.1f  Sweep: %.1f", sa, sw);
                    ImGui::Text("UseCenter: %s", uc ? "true" : "false");
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Oval", r);
                    w.Line("Start: %.1f  Sweep: %.1f", sa, sw);
                    w.Line("UseCenter: %s", uc ? "true" : "false");
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawArc(r, sa, sw, uc, paintFor(p));
    }
    void onDrawPath(const SkPath &path, const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawPath",
                [=]
                {
                    UiPath(path);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    TxPath(w, path);
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawPath(path, paintFor(p));
    }
    void onDrawRegion(const SkRegion &r, const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawRegion", [=] { UiPaint(p); },
                [=](TextWriter &w) { TxPaint(w, p); });
            return;
        }
        if (skip())
            return;
        target->drawRegion(r, paintFor(p));
    }
    void onDrawBehind(const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawBehind", [=] { UiPaint(p); },
                [=](TextWriter &w) { TxPaint(w, p); });
            return;
        }
        if (skip())
            return;
        target->drawPaint(paintFor(p));
    }
    void onDrawPoints(PointMode m, size_t c, const SkPoint pts[],
                      const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawPoints",
                [=]
                {
                    ImGui::Text("Count: %zu", c);
                    ImGui::Text("Mode: %d", (int)m);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    w.Line("Count: %zu", c);
                    w.Line("Mode: %d", (int)m);
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawPoints(m, {pts, c}, paintFor(p));
    }
    void onDrawImage2(const SkImage *img, SkScalar x, SkScalar y,
                      const SkSamplingOptions &s, const SkPaint *p) override
    {
        int iw = img->width(), ih = img->height();
        SkPaint pp = p ? *p : SkPaint();
        if (collect())
        {
            collectImage(img);
            collectPaint(p);
            sk_sp<SkImage> img_keep = sk_ref_sp(img);
            record(
                "DrawImage",
                [=]
                {
                    ImGui::Text("Pos: (%.1f, %.1f)", x, y);
                    UiImageButton(img_keep.get());
                    UiPaint(pp);
                },
                [=](TextWriter &w)
                {
                    w.Line("Pos: (%.1f, %.1f)", x, y);
                    w.Line("Size: %dx%d", iw, ih);
                    TxPaint(w, pp);
                });
            return;
        }
        if (skip())
            return;
        if (isHighlightTarget())
            target->drawRect(SkRect::MakeXYWH(x, y, (float)iw, (float)ih),
                             magentaPaint());
        else
            target->drawImage(img, x, y, s, p);
    }
    void onDrawImageRect2(const SkImage *img, const SkRect &src,
                          const SkRect &dst, const SkSamplingOptions &s,
                          const SkPaint *p, SrcRectConstraint c) override
    {
        int iw = img->width(), ih = img->height();
        SkPaint pp = p ? *p : SkPaint();
        // SurfaceID|<id> just preceded us: this drawImageRect composites
        // the offscreen layer back into the parent. Record/replay it as
        // a special "DrawLayerImage" so the user knows what's happening,
        // and at replay swap the dummy with the layer's cached image.
        int composite_layer = pending_image_layer_id;
        pending_image_layer_id = -1;
        if (collect())
        {
            // The composite's "image" is just a throwaway placeholder
            // Android wrote to bind the SurfaceID to a drawImageRect call
            // (typically 1x1 magenta). Skipping collectImage keeps it out
            // of the Images panel — the real layer image gets added by
            // CollectFromPicture below.
            if (composite_layer < 0)
                collectImage(img);
            collectPaint(p);
            if (composite_layer >= 0)
            {
                int lid = composite_layer;
                // cached_image is populated after CollectFromPicture
                // finishes. Give the closure a shared pointer whose
                // contents we fill in later when the image exists.
                auto img_box = std::make_shared<sk_sp<SkImage>>();
                pending_layer_image_boxes.push_back({lid, img_box});
                record(
                    "DrawLayerImage",
                    [=]
                    {
                        ImGui::Text("Layer #%d", lid);
                        UiRect("Src", src);
                        UiRect("Dst", dst);
                        if (*img_box)
                            UiImageButton(img_box->get());
                        UiPaint(pp);
                    },
                    [=](TextWriter &w)
                    {
                        w.Line("Layer #%d", lid);
                        TxRect(w, "Src", src);
                        TxRect(w, "Dst", dst);
                        TxPaint(w, pp);
                    });
                return;
            }
            sk_sp<SkImage> img_keep = sk_ref_sp(img);
            record(
                "DrawImageRect",
                [=]
                {
                    UiRect("Src", src);
                    UiRect("Dst", dst);
                    UiImageButton(img_keep.get());
                    UiPaint(pp);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Src", src);
                    TxRect(w, "Dst", dst);
                    w.Line("Image: %dx%d", iw, ih);
                    TxPaint(w, pp);
                });
            return;
        }
        if (skip())
            return;
        if (isHighlightTarget())
        {
            target->drawRect(dst, magentaPaint());
            return;
        }
        // Substitute the layer's actual rendered image for the dummy that
        // Android wrote at capture time, when one is available.
        const SkImage *to_draw = img;
        if (composite_layer >= 0 && external_layers)
        {
            for (const auto &L : *external_layers)
            {
                if (L.id == composite_layer && L.cached_image)
                {
                    to_draw = L.cached_image.get();
                    break;
                }
            }
        }
        target->drawImageRect(to_draw, src, dst, s, p, c);
    }
    void onDrawImageLattice2(const SkImage *img, const Lattice &l,
                             const SkRect &dst, SkFilterMode f,
                             const SkPaint *p) override
    {
        SkPaint pp = p ? *p : SkPaint();
        if (collect())
        {
            collectImage(img);
            collectPaint(p);
            sk_sp<SkImage> img_keep = sk_ref_sp(img);
            record(
                "DrawImageLattice",
                [=]
                {
                    UiRect("Dst", dst);
                    UiImageButton(img_keep.get());
                    UiPaint(pp);
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Dst", dst);
                    TxPaint(w, pp);
                });
            return;
        }
        if (skip())
            return;
        if (isHighlightTarget())
            target->drawRect(dst, magentaPaint());
        else
            target->drawImageLattice(img, l, dst, f, p);
    }
    void onDrawTextBlob(const SkTextBlob *b, SkScalar x, SkScalar y,
                        const SkPaint &p) override
    {
        SkRect bounds = b->bounds();
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawTextBlob",
                [=]
                {
                    ImGui::Text("Pos: (%.1f, %.1f)", x, y);
                    UiRect("Bounds", bounds);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    w.Line("Pos: (%.1f, %.1f)", x, y);
                    TxRect(w, "Bounds", bounds);
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawTextBlob(b, x, y, paintFor(p));
    }
    void onDrawGlyphRunList(const sktext::GlyphRunList &,
                            const SkPaint &) override
    {
    }
    void onDrawSlug(const sktext::gpu::Slug *, const SkPaint &) override
    {
    }
    void onDrawPatch(const SkPoint c[12], const SkColor col[4],
                     const SkPoint tc[4], SkBlendMode m,
                     const SkPaint &p) override
    {
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawPatch", [=] { UiPaint(p); },
                [=](TextWriter &w) { TxPaint(w, p); });
            return;
        }
        if (skip())
            return;
        target->drawPatch(c, col, tc, m, paintFor(p));
    }
    void onDrawVerticesObject(const SkVertices *v, SkBlendMode m,
                              const SkPaint &p) override
    {
        int vc = v->priv().vertexCount();
        if (collect())
        {
            collectPaint(p);
            record(
                "DrawVertices",
                [=]
                {
                    ImGui::Text("VertexCount: %d", vc);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    w.Line("VertexCount: %d", vc);
                    TxPaint(w, p);
                });
            return;
        }
        if (skip())
            return;
        target->drawVertices(v, m, paintFor(p));
    }
    void onDrawShadowRec(const SkPath &path, const SkDrawShadowRec &) override
    {
        if (collect())
        {
            record(
                "DrawShadow", [=] { UiPath(path); },
                [=](TextWriter &w) { TxPath(w, path); });
            return;
        }
    }
    void onDrawPicture(const SkPicture *pic, const SkMatrix *m,
                       const SkPaint *p) override
    {
        // OffscreenLayerDraw annotation just set pending_layer_id: this
        // drawPicture IS the layer's content.
        bool is_layer = pending_layer_id >= 0;
        if (collect() && is_layer)
        {
            int id = pending_layer_id;
            SkIRect dirty = pending_layer_dirty;
            sk_sp<SkPicture> layer_pic = sk_ref_sp(pic);
            pending_layer_id = -1;

            int begin_idx = (int)commands.size();
            SkIRect layer_bounds = layer_pic->cullRect().roundOut();
            int layer_idx = (int)layers.size();
            layers.push_back(
                {id, dirty, layer_pic, begin_idx, -1, begin_idx + 1, {}});

            // Begin marker corresponds to the drawPicture call in the
            // parent stream — advances main_play_idx via record().
            record(
                "OffscreenLayer",
                [=]
                {
                    ImGui::Text("Layer #%d", id);
                    ImGui::Text("Size: %dx%d", layer_bounds.width(),
                                layer_bounds.height());
                    UiRect("Dirty", SkRect::Make(dirty));
                },
                [=](TextWriter &w)
                {
                    w.Line("Layer #%d", id);
                    w.Line("Size: %dx%d", layer_bounds.width(),
                           layer_bounds.height());
                    TxRect(w, "Dirty", SkRect::Make(dirty));
                },
                CommandKind::OffscreenLayer);

            // Recurse with a fresh layer_play_idx so child commands get
            // positions in the layer's OWN picture stream (used as the
            // stopIndex in DrawLayerToPicture).
            depth++;
            int prev_layer = current_layer;
            int saved_layer_play = layer_play_idx;
            current_layer = layer_idx;
            layer_play_idx = 0;
            pic->playback(this);
            layer_play_idx = saved_layer_play;
            current_layer = prev_layer;
            depth--;

            // End marker is synthetic: doesn't correspond to a parent
            // SkCanvas call. Push directly, do NOT advance main_play_idx.
            int end_idx = (int)commands.size();
            commands.push_back({"EndOffscreenLayer",
                                {},
                                {},
                                depth,
                                CommandKind::EndOffscreenLayer,
                                -1,
                                current_layer,
                                main_play_idx});
            commands[begin_idx].range_end = end_idx;
            layers[layer_idx].end_cmd = end_idx;
            return;
        }
        if (!collect() && is_layer)
        {
            // Replay mode for an offscreen-layer drawPicture: advance
            // index for THIS drawPicture call, but DO NOT recurse — its
            // contents would be drawn at parent (0,0) and double up with
            // the SurfaceID composite. The composite drawImageRect later
            // uses the layer's pre-rendered cached image instead.
            pending_layer_id = -1;
            skip();
            return;
        }
        // Default: recurse so inner commands are walked one-by-one. Without
        // this MSKP frames (each wrapped in a drawPicture) would collapse
        // to a single "DrawPicture" command or draw all-or-nothing.
        if (m || p)
            this->save();
        if (m)
            this->concat(*m);
        pic->playback(this);
        if (m || p)
            this->restore();
    }
    void onDrawAtlas2(const SkImage *img, const SkRSXform xf[],
                      const SkRect src[], const SkColor col[], int count,
                      SkBlendMode m, const SkSamplingOptions &s,
                      const SkRect *cull, const SkPaint *p) override
    {
        SkPaint pp = p ? *p : SkPaint();
        if (collect())
        {
            collectImage(img);
            collectPaint(p);
            sk_sp<SkImage> img_keep = sk_ref_sp(img);
            record(
                "DrawAtlas",
                [=]
                {
                    ImGui::Text("Count: %d", count);
                    UiImageButton(img_keep.get());
                    UiPaint(pp);
                },
                [=](TextWriter &w)
                {
                    w.Line("Count: %d", count);
                    TxPaint(w, pp);
                });
            return;
        }
        if (skip())
            return;
        target->drawAtlas(img, {xf, (size_t)count}, {src, (size_t)count},
                          {col, col ? (size_t)count : 0u}, m, s, cull, p);
    }
    void onDrawDrawable(SkDrawable *d, const SkMatrix *m) override
    {
        if (collect())
        {
            record("DrawDrawable");
            return;
        }
        if (skip())
            return;
        target->drawDrawable(d, m);
    }
    void onDrawEdgeAAQuad(const SkRect &, const SkPoint[4], QuadAAFlags,
                          const SkColor4f &, SkBlendMode) override
    {
    }
    void onDrawEdgeAAImageSet2(const ImageSetEntry[], int, const SkPoint[],
                               const SkMatrix[], const SkSamplingOptions &,
                               const SkPaint *, SrcRectConstraint) override
    {
    }
    void onDrawAnnotation(const SkRect &r, const char key[],
                          SkData *value) override
    {
        std::string k = key;
        // Android HWUI offscreen-layer markers: stash state for the next
        // draw op to consume, but also record/forward like a normal
        // annotation so the user can see them in the command list (and
        // hide via the Hide Annotations toggle).
        //   OffscreenLayerDraw|<nodeId>  -> next drawPicture IS a layer
        //   SurfaceID|<nodeId>           -> next drawImageRect composites
        size_t pipe = k.find('|');
        if (pipe != std::string::npos)
        {
            std::string kind = k.substr(0, pipe);
            if (kind == "OffscreenLayerDraw")
            {
                pending_layer_id = std::atoi(k.c_str() + pipe + 1);
                pending_layer_dirty = r.roundOut();
            }
            else if (kind == "SurfaceID")
                pending_image_layer_id = std::atoi(k.c_str() + pipe + 1);
        }
        std::string v =
            (value && value->size() > 0)
                ? std::string((const char *)value->data(), value->size())
                : std::string();
        if (collect())
        {
            record(
                "Annotation",
                [=]
                {
                    ImGui::Text("Key: %s", k.c_str());
                    if (!v.empty())
                        ImGui::Text("Value: %s", v.c_str());
                    UiRect("Rect", r);
                },
                [=](TextWriter &w)
                {
                    w.Line("Key: %s", k.c_str());
                    if (!v.empty())
                        w.Line("Value: %s", v.c_str());
                    TxRect(w, "Rect", r);
                },
                CommandKind::Annotation);
            return;
        }
        if (skip())
            return;
        target->drawAnnotation(r, key, value);
    }
    void onClipRect(const SkRect &r, SkClipOp op, ClipEdgeStyle s) override
    {
        bool aa = s == kSoft_ClipEdgeStyle;
        if (collect())
        {
            record(
                "ClipRect",
                [=]
                {
                    UiRect("Rect", r);
                    ImGui::Text("Op: %s", ClipOpName(op));
                    ImGui::Text("AA: %s", aa ? "true" : "false");
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Rect", r);
                    w.Line("Op: %s", ClipOpName(op));
                    w.Line("AA: %s", aa ? "true" : "false");
                });
            return;
        }
        if (skip())
            return;
        target->clipRect(r, op, aa);
    }
    void onClipRRect(const SkRRect &rr, SkClipOp op, ClipEdgeStyle s) override
    {
        bool aa = s == kSoft_ClipEdgeStyle;
        if (collect())
        {
            record(
                "ClipRRect",
                [=]
                {
                    UiRect("Rect", rr.rect());
                    ImGui::Text("Op: %s", ClipOpName(op));
                    ImGui::Text("AA: %s", aa ? "true" : "false");
                },
                [=](TextWriter &w)
                {
                    TxRect(w, "Rect", rr.rect());
                    w.Line("Op: %s", ClipOpName(op));
                    w.Line("AA: %s", aa ? "true" : "false");
                });
            return;
        }
        if (skip())
            return;
        target->clipRRect(rr, op, aa);
    }
    void onClipPath(const SkPath &path, SkClipOp op, ClipEdgeStyle s) override
    {
        bool aa = s == kSoft_ClipEdgeStyle;
        if (collect())
        {
            record(
                "ClipPath",
                [=]
                {
                    UiPath(path);
                    ImGui::Text("Op: %s", ClipOpName(op));
                    ImGui::Text("AA: %s", aa ? "true" : "false");
                },
                [=](TextWriter &w)
                {
                    TxPath(w, path);
                    w.Line("Op: %s", ClipOpName(op));
                    w.Line("AA: %s", aa ? "true" : "false");
                });
            return;
        }
        if (skip())
            return;
        target->clipPath(path, op, aa);
    }
    void onClipShader(sk_sp<SkShader> sh, SkClipOp op) override
    {
        if (collect())
        {
            record(
                "ClipShader", [=] { ImGui::Text("Op: %s", ClipOpName(op)); },
                [=](TextWriter &w) { w.Line("Op: %s", ClipOpName(op)); });
            return;
        }
        if (skip())
            return;
        target->clipShader(sh, op);
    }
    void onClipRegion(const SkRegion &r, SkClipOp op) override
    {
        if (collect())
        {
            record(
                "ClipRegion", [=] { ImGui::Text("Op: %s", ClipOpName(op)); },
                [=](TextWriter &w) { w.Line("Op: %s", ClipOpName(op)); });
            return;
        }
        if (skip())
            return;
        target->clipRegion(r, op);
    }
    void onResetClip() override
    {
    }
    void onDrawMesh(const SkMesh &, sk_sp<SkBlender>, const SkPaint &) override
    {
    }
    void willSave() override
    {
        if (collect())
        {
            record("Save", {}, {}, CommandKind::Save);
            depth++;
            return;
        }
        if (skip())
            return;
        target->save();
    }
    void willRestore() override
    {
        if (collect())
        {
            if (depth > 0)
                depth--;
            record("Restore", {}, {}, CommandKind::Restore);
            return;
        }
        if (skip())
            return;
        target->restore();
    }
    SaveLayerStrategy getSaveLayerStrategy(const SaveLayerRec &rec) override
    {
        SkPaint p = rec.fPaint ? *rec.fPaint : SkPaint();
        bool has_bounds = rec.fBounds != nullptr;
        SkRect bounds = has_bounds ? *rec.fBounds : SkRect::MakeEmpty();
        if (collect())
        {
            record(
                "SaveLayer",
                [=]
                {
                    if (has_bounds)
                        UiRect("Bounds", bounds);
                    UiPaint(p);
                },
                [=](TextWriter &w)
                {
                    if (has_bounds)
                        TxRect(w, "Bounds", bounds);
                    TxPaint(w, p);
                },
                CommandKind::SaveLayer);
            depth++;
            return kNoLayer_SaveLayerStrategy;
        }
        if (!skip())
            target->saveLayer(rec);
        return kNoLayer_SaveLayerStrategy;
    }
    void didConcat44(const SkM44 &m) override
    {
        if (collect())
        {
            record(
                "Concat", [=] { UiM44("Matrix", m); },
                [=](TextWriter &w) { TxM44(w, "Matrix", m); });
            return;
        }
        if (skip())
            return;
        target->concat(m);
    }
    void didSetM44(const SkM44 &m) override
    {
        if (collect())
        {
            record(
                "SetMatrix", [=] { UiM44("Matrix", m); },
                [=](TextWriter &w) { TxM44(w, "Matrix", m); });
            return;
        }
        if (skip())
            return;
        target->setMatrix(m);
    }
    void didTranslate(SkScalar dx, SkScalar dy) override
    {
        if (collect())
        {
            record(
                "Translate",
                [=]
                {
                    ImGui::Text("dx: %.2f", dx);
                    ImGui::Text("dy: %.2f", dy);
                },
                [=](TextWriter &w)
                {
                    w.Line("dx: %.2f", dx);
                    w.Line("dy: %.2f", dy);
                });
            return;
        }
        if (skip())
            return;
        target->translate(dx, dy);
    }
    void didScale(SkScalar sx, SkScalar sy) override
    {
        if (collect())
        {
            record(
                "Scale",
                [=]
                {
                    ImGui::Text("sx: %.2f", sx);
                    ImGui::Text("sy: %.2f", sy);
                },
                [=](TextWriter &w)
                {
                    w.Line("sx: %.2f", sx);
                    w.Line("sy: %.2f", sy);
                });
            return;
        }
        if (skip())
            return;
        target->scale(sx, sy);
    }
};

static GLuint g_checker_tex = 0;

static GLuint GetCheckerTex()
{
    if (g_checker_tex)
        return g_checker_tex;
    uint32_t pixels[kCheckerTileSize * kCheckerTileSize];
    for (int y = 0; y < kCheckerTileSize; y++)
        for (int x = 0; x < kCheckerTileSize; x++)
            pixels[y * kCheckerTileSize + x] =
                ((x / 8 + y / 8) % 2) ? 0xFFCCCCCC : 0xFF999999;
    glGenTextures(1, &g_checker_tex);
    glBindTexture(GL_TEXTURE_2D, g_checker_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kCheckerTileSize, kCheckerTileSize,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return g_checker_tex;
}

void DrawPixelMagnifier(int &hover_px, int &hover_py, int tex_w, int tex_h,
                        const uint8_t *pixels, int mag, float max_width)
{
    int half = mag / 2;
    float cell = floorf(fminf(max_width / mag, kMagnifierCellSize));
    float mag_size = mag * cell;
    ImVec2 mag_pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##mag", ImVec2(mag_size, mag_size));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse = ImGui::GetIO().MousePos;
        int dx = (int)((mouse.x - mag_pos.x) / cell) - half;
        int dy = (int)((mouse.y - mag_pos.y) / cell) - half;
        int new_px = hover_px + dx;
        int new_py = hover_py + dy;
        if (new_px >= 0 && new_px < tex_w && new_py >= 0 && new_py < tex_h)
        {
            hover_px = new_px;
            hover_py = new_py;
        }
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    for (int dy = -half; dy <= half; dy++)
    {
        for (int dx = -half; dx <= half; dx++)
        {
            int sx = hover_px + dx;
            int sy = hover_py + dy;
            uint32_t c = 0;
            if (sx >= 0 && sx < tex_w && sy >= 0 && sy < tex_h)
                memcpy(&c, pixels + (sy * tex_w + sx) * 4, 4);
            uint8_t cr = c & 0xFF;
            uint8_t cg = (c >> 8) & 0xFF;
            uint8_t cb = (c >> 16) & 0xFF;
            uint8_t ca = (c >> 24) & 0xFF;
            float fx = mag_pos.x + (dx + half) * cell;
            float fy = mag_pos.y + (dy + half) * cell;
            dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + cell, fy + cell),
                              IM_COL32(cr, cg, cb, ca));
        }
    }
    float cx = mag_pos.x + half * cell + cell * 0.5f;
    float cy = mag_pos.y + half * cell + cell * 0.5f;
    dl->AddRect(ImVec2(cx - cell * 0.5f, cy - cell * 0.5f),
                ImVec2(cx + cell * 0.5f, cy + cell * 0.5f),
                IM_COL32(255, 255, 255, 200));
}

void ImageViewer::Fit(float view_w, float view_h, int img_w, int img_h)
{
    if (img_w <= 0 || img_h <= 0)
        return;
    float sx = view_w / img_w;
    float sy = view_h / img_h;
    zoom = fminf(sx, sy);
    pan_x = (view_w - img_w * zoom) * 0.5f;
    pan_y = (view_h - img_h * zoom) * 0.5f;
}

void ImageViewer::Clamp(float view_w, float view_h, int img_w, int img_h)
{
    float qw = img_w * zoom;
    float qh = img_h * zoom;
    float margin = kPanMargin;
    if (pan_x > view_w - margin)
        pan_x = view_w - margin;
    if (pan_y > view_h - margin)
        pan_y = view_h - margin;
    if (pan_x + qw < margin)
        pan_x = margin - qw;
    if (pan_y + qh < margin)
        pan_y = margin - qh;
}

void ImageViewer::DrawImage(GLuint tex, int tex_w, int tex_h,
                            const uint8_t *pixels, bool flip_y)
{
    float sidebar_w = (pixels && zoom >= 4.0f) ? kViewportPanelWidth : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float img_area_w = avail.x - sidebar_w;
    ImVec2 img_area(img_area_w, avail.y);
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##iv", img_area);
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    focused = ImGui::IsWindowFocused();

    last_view_w = img_area_w;
    last_view_h = avail.y;
    last_img_w = tex_w;
    last_img_h = tex_h;
    Clamp(img_area_w, avail.y, tex_w, tex_h);

    float qx = cursor.x + pan_x;
    float qy = cursor.y + pan_y;
    float qw = tex_w * zoom;
    float qh = tex_h * zoom;

    ImDrawList *dl = ImGui::GetWindowDrawList();

    // Checkerboard behind for alpha visualization.
    GLuint checker = GetCheckerTex();
    float tile = 16.0f;
    dl->AddImage((ImTextureID)(uintptr_t)checker, ImVec2(qx, qy),
                 ImVec2(qx + qw, qy + qh), ImVec2(0, 0),
                 ImVec2(qw / tile, qh / tile));

    ImVec2 uv0 = flip_y ? ImVec2(0, 1) : ImVec2(0, 0);
    ImVec2 uv1 = flip_y ? ImVec2(1, 0) : ImVec2(1, 1);
    dl->AddImage((ImTextureID)(uintptr_t)tex, ImVec2(qx, qy),
                 ImVec2(qx + qw, qy + qh), uv0, uv1);

    // Hold Alt/Option to sample the pixel under the cursor.
    if (hovered && ImGui::GetIO().KeyAlt)
    {
        ImGuiIO &io = ImGui::GetIO();
        float mx = io.MousePos.x - cursor.x;
        float my = io.MousePos.y - cursor.y;
        int px = (int)((mx - pan_x) / zoom);
        int py = (int)((my - pan_y) / zoom);
        if (px >= 0 && px < tex_w && py >= 0 && py < tex_h)
        {
            hover_px = px;
            hover_py = py;
            if (pixels)
            {
                int idx = (py * tex_w + px) * 4;
                hover_color = *(uint32_t *)(pixels + idx);
            }
        }
    }

    // Scroll zoom to cursor
    if (hovered)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f)
        {
            float mx = io.MousePos.x - cursor.x;
            float my = io.MousePos.y - cursor.y;
            float old_zoom = zoom;
            ZoomTo(zoom * (1.0f + io.MouseWheel * 0.1f));
            float wx = (mx - pan_x) / old_zoom;
            float wy = (my - pan_y) / old_zoom;
            pan_x = mx - wx * zoom;
            pan_y = my - wy * zoom;
        }

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            Reset();
    }

    // Left-drag pan
    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        pan_x += ImGui::GetIO().MouseDelta.x;
        pan_y += ImGui::GetIO().MouseDelta.y;
    }

    // Sidebar: magnifier + pixel info
    if (sidebar_w > 0 && pixels && hover_px >= 0)
    {
        ImGui::SameLine();
        ImGui::BeginGroup();

        // Pixel info
        uint8_t r = hover_color & 0xFF;
        uint8_t g = (hover_color >> 8) & 0xFF;
        uint8_t b = (hover_color >> 16) & 0xFF;
        uint8_t a = (hover_color >> 24) & 0xFF;
        ImGui::Text("X: %d  Y: %d", hover_px, hover_py);
        ImVec4 col(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        ImGui::ColorButton("##pxcol", col, ImGuiColorEditFlags_AlphaPreview,
                           ImVec2(20, 20));
        ImGui::SameLine();
        ImGui::Text("#%02X%02X%02X%02X", r, g, b, a);

        DrawPixelMagnifier(hover_px, hover_py, tex_w, tex_h, pixels,
                           kImageMagnifierSize, kViewportPanelWidth);

        ImGui::EndGroup();
    }
}

struct ImageTile
{
    uint32_t image_id = 0;
    GLuint tex = 0;
    int width = 0;
    int height = 0;
    std::string label;
};

static std::vector<ImageTile> g_image_tiles;
static uint32_t g_scroll_to_image = 0; // nonzero = tile id to center on
uint32_t g_selected_image = 0;

void ResetImageTiles()
{
    for (auto &t : g_image_tiles)
        if (t.tex)
            glDeleteTextures(1, &t.tex);
    g_image_tiles.clear();
    g_scroll_to_image = 0;
    g_selected_image = 0;
}

static GLuint UploadImage(const SkImage *img)
{
    int w = img->width(), h = img->height();
    if (w <= 0 || h <= 0)
        return 0;

    // Explicit RGBA so glTexImage2D(..., GL_RGBA, ...) gets bytes in the
    // order it expects. N32Premul is platform-dependent (BGRA on many
    // platforms) which would swap R/B channels.
    SkImageInfo info =
        SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    SkBitmap bm;
    bm.allocPixels(info);
    if (!img->readPixels(bm.pixmap(), 0, 0))
        return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 bm.getPixels());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

// Ensure the given image has a GPU-uploaded tile in the Images canvas.
// Safe to call multiple times with the same id.
void AddImageTile(uint32_t id, const SkImage *img)
{
    for (const auto &t : g_image_tiles)
        if (t.image_id == id)
            return;

    int iw = img->width(), ih = img->height();
    GLuint tex = UploadImage(img);
    if (!tex)
        return;

    ImageTile t;
    t.image_id = id;
    t.tex = tex;
    t.width = iw;
    t.height = ih;
    t.label = sfmt("[%u] %dx%d", id, iw, ih);
    g_image_tiles.push_back(std::move(t));
}

// Called by the Resources panel "View" button. Uploads if necessary, then
// requests the Images canvas scroll to this tile and take focus.
void FocusImageTile(uint32_t id, const SkImage *img)
{
    AddImageTile(id, img);
    g_scroll_to_image = id;
    g_selected_image = id;
    ImGui::SetWindowFocus("Images");
}

// Single "Shaders" panel: all SkSL-bearing shaders in the current frame
// stacked vertically as collapsible sections. Each has a Copy button.
void DrawShadersPanel(const std::vector<ResourceInfo> &resources)
{
    if (!ImGui::Begin("Shaders"))
    {
        ImGui::End();
        return;
    }

    bool any = false;
    for (const auto &r : resources)
    {
        if (r.type != ResourceInfo::Shader || !r.effect)
            continue;
        const SkFlattenable *f = (const SkFlattenable *)r.effect;
        if (!HasSksl(f))
            continue;
        any = true;

        std::string header = sfmt("%s [%u]", r.type_name.c_str(), r.id);
        if (g_scroll_to_shader == r.id)
        {
            ImGui::SetNextItemOpen(true);
            ImGui::SetScrollHereY(0.0f);
            g_scroll_to_shader = 0;
        }
        ImGui::PushID((int)r.id);
        if (ImGui::CollapsingHeader(header.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            std::string source = ExtractSksl(f);
            if (ImGui::Button("Copy"))
                ImGui::SetClipboardText(source.c_str());

            // Compute height for this shader's source block so long shaders
            // don't eat the whole window.
            float line_h = ImGui::GetTextLineHeightWithSpacing();
            int lines = 1;
            for (char c : source)
                if (c == '\n')
                    lines++;
            float h = std::min(line_h * (lines + 1), line_h * 24);

            ImGui::BeginChild("##sksl", ImVec2(-1, h), ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_HorizontalScrollbar);
            const char *src = source.c_str();
            const char *end = src + source.size();
            while (src < end)
            {
                const char *nl = (const char *)memchr(src, '\n', end - src);
                if (!nl)
                    nl = end;
                ImGui::TextUnformatted(src, nl);
                src = nl + 1;
            }
            ImGui::EndChild();
        }
        ImGui::PopID();
    }

    if (!any)
        ImGui::TextDisabled("No shaders with SkSL source.");

    ImGui::End();
}

// Single "Images" window: all uploaded image tiles laid out in a wrapping
// row flow. Scroll-to-zoom, middle-drag or left-drag on empty space to pan.
static float g_images_zoom = 1.0f;
static ImVec2 g_images_pan = {0, 0};

void DrawImageWindows(const std::vector<ResourceInfo> &resources)
{
    // Build the set of image IDs in the current frame so we only show
    // tiles for images actually referenced by this frame.
    std::set<uint32_t> visible_ids;
    for (const auto &r : resources)
        if (r.type == ResourceInfo::Image)
            visible_ids.insert(r.id);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin("Images"))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    if (visible_ids.empty())
    {
        ImGui::TextDisabled("No images.");
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

    // Fullscreen invisible button catches drag/scroll over empty regions.
    ImGui::InvisibleButton("##images_canvas", avail,
                           ImGuiButtonFlags_MouseButtonLeft |
                               ImGuiButtonFlags_MouseButtonMiddle);
    bool canvas_hovered = ImGui::IsItemHovered();
    bool canvas_active = ImGui::IsItemActive();

    ImGuiIO &io = ImGui::GetIO();

    // Scroll to zoom, centered on mouse.
    if (canvas_hovered && io.MouseWheel != 0.0f)
    {
        float old_zoom = g_images_zoom;
        float factor = 1.0f + io.MouseWheel * 0.1f;
        g_images_zoom = fmaxf(0.05f, fminf(g_images_zoom * factor, 32.0f));
        ImVec2 m = {io.MousePos.x - canvas_pos.x, io.MousePos.y - canvas_pos.y};
        g_images_pan.x =
            m.x - (m.x - g_images_pan.x) * (g_images_zoom / old_zoom);
        g_images_pan.y =
            m.y - (m.y - g_images_pan.y) * (g_images_zoom / old_zoom);
    }

    // Drag to pan.
    if (canvas_active)
    {
        g_images_pan.x += io.MouseDelta.x;
        g_images_pan.y += io.MouseDelta.y;
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(canvas_pos,
                     ImVec2(canvas_pos.x + avail.x, canvas_pos.y + avail.y),
                     true);

    constexpr float kPadding = 24.0f;
    constexpr float kLabelH = 24.0f;
    constexpr float kLayoutWidth = 800.0f; // fixed world-space row width

    // Layout in fixed "world" coords so tiles don't reflow when zooming.
    float cursor_x = 0, cursor_y = 0;
    float row_h = 0;

    float zoom = g_images_zoom;
    ImVec2 origin = {canvas_pos.x + g_images_pan.x,
                     canvas_pos.y + g_images_pan.y};

    for (size_t i = 0; i < g_image_tiles.size(); i++)
    {
        const ImageTile &t = g_image_tiles[i];
        if (!visible_ids.count(t.image_id))
            continue;
        float tw = (float)t.width;
        float th = (float)t.height;
        float cell_w = fmaxf(tw, 80.0f);
        float cell_h = th + kLabelH;

        if (cursor_x > 0 && cursor_x + kPadding + cell_w > kLayoutWidth)
        {
            cursor_x = 0;
            cursor_y += row_h + kPadding;
            row_h = 0;
        }

        // Screen-space tile rect.
        ImVec2 p0 = {origin.x + cursor_x * zoom, origin.y + cursor_y * zoom};
        ImVec2 img_br = {p0.x + tw * zoom, p0.y + th * zoom};

        // Scroll-to marker: when requested, adjust pan so tile is centered.
        if (g_scroll_to_image == t.image_id)
        {
            float target_x = (tw * zoom) * 0.5f;
            float target_y = (th * zoom) * 0.5f;
            g_images_pan.x = avail.x * 0.5f - cursor_x * zoom - target_x;
            g_images_pan.y = avail.y * 0.5f - cursor_y * zoom - target_y;
            g_scroll_to_image = 0;
        }

        dl->AddImage((ImTextureID)(uintptr_t)t.tex, p0, img_br);

        // Click-pick: use ImGui::IsMouseHoveringRect + left-click not
        // consumed by canvas drag. We check only on click-release (no drag).
        if (canvas_hovered && !ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            io.MousePos.x >= p0.x && io.MousePos.x <= img_br.x &&
            io.MousePos.y >= p0.y && io.MousePos.y <= img_br.y)
        {
            g_selected_image = t.image_id;
        }

        bool selected = (g_selected_image == t.image_id);
        if (selected)
            dl->AddRect(p0, img_br, IM_COL32(255, 180, 0, 255), 0, 0, 2.0f);
        else if (canvas_hovered && io.MousePos.x >= p0.x &&
                 io.MousePos.x <= img_br.x && io.MousePos.y >= p0.y &&
                 io.MousePos.y <= img_br.y)
            dl->AddRect(p0, img_br, IM_COL32(255, 255, 255, 120), 0, 0, 2.0f);

        // Label underneath. Scale the font with zoom (ImGui dynamic font
        // rasterization handles arbitrary sizes) so text tracks the image
        // tile size, and skip drawing once it's too small to read.
        float label_size = ImGui::GetFontSize() * zoom;
        if (label_size >= 6.0f)
        {
            ImVec2 label_pos = {p0.x, p0.y + th * zoom + 2.0f};
            dl->AddText(ImGui::GetFont(), label_size, label_pos,
                        IM_COL32(200, 200, 200, 255), t.label.c_str());
        }

        cursor_x += cell_w + kPadding;
        row_h = fmaxf(row_h, cell_h);
    }

    dl->PopClipRect();

    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugCanvas::CollectFromPicture(const SkPicture *picture)
{
    SkRect cull = picture->cullRect();
    InterceptCanvas ic((int)cull.width(), (int)cull.height(), nullptr, -1);
    picture->playback(&ic);
    commands_ = std::move(ic.commands);
    resources_ = std::move(ic.resources);
    layers_ = std::move(ic.layers);

    // Render each offscreen layer to a CPU raster image so it shows up in
    // the Images panel and so the SurfaceID drawImageRect composite can
    // substitute it at replay time without needing a GPU surface.
    for (auto &L : layers_)
    {
        if (L.cached_image || !L.picture)
            continue;
        SkRect lcull = L.picture->cullRect();
        int w = (int)lcull.width(), h = (int)lcull.height();
        if (w <= 0 || h <= 0)
            continue;
        SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType,
                                             kPremul_SkAlphaType);
        sk_sp<SkSurface> surf = SkSurfaces::Raster(info);
        if (!surf)
            continue;
        surf->getCanvas()->clear(SK_ColorTRANSPARENT);
        L.picture->playback(surf->getCanvas());
        L.cached_image = surf->makeImageSnapshot();
        if (L.cached_image)
            resources_.push_back(
                {ResourceInfo::Image, L.cached_image->uniqueID(),
                 sfmt("Layer #%d %dx%d", L.id, w, h), L.cached_image, nullptr});
    }

    // Fill the DrawLayerImage UI closures' image boxes now that every
    // layer has a rendered snapshot. Lambdas captured the same shared
    // box so they'll see the assignment immediately.
    for (auto &pending : ic.pending_layer_image_boxes)
        for (const auto &L : layers_)
            if (L.id == pending.layer_id && L.cached_image)
            {
                *pending.box = L.cached_image;
                break;
            }

    // Pair Save / SaveLayer with their matching Restore so the UI can
    // collapse the whole group. Unbalanced Saves (no closing Restore)
    // keep range_end = -1 and render like normal leaf commands.
    std::vector<int> stack;
    for (int i = 0; i < (int)commands_.size(); i++)
    {
        CommandKind k = commands_[i].kind;
        if (k == CommandKind::Save || k == CommandKind::SaveLayer)
            stack.push_back(i);
        else if (k == CommandKind::Restore && !stack.empty())
        {
            commands_[stack.back()].range_end = i;
            stack.pop_back();
        }
    }
}

void DebugCanvas::DrawToPicture(SkCanvas *canvas, const SkPicture *picture,
                                int stopIndex, bool show_clip,
                                bool highlight_geometry)
{
    // Layer images were pre-rendered (CPU raster) in CollectFromPicture
    // and live on each LayerSpan::cached_image — used at replay time to
    // substitute the SurfaceID composite drawImageRect's placeholder.
    SkRect cull = picture->cullRect();
    InterceptCanvas ic((int)cull.width(), (int)cull.height(), canvas, stopIndex,
                       highlight_geometry, &layers_);
    picture->playback(&ic);

    if (show_clip)
    {
        // The target canvas now has the clip state at stopIndex.
        // Fill the entire cull rect — Skia clips it to the active region.
        SkPaint overlay;
        overlay.setColor(0x600099FF);
        canvas->drawRect(cull, overlay);
    }
}

void DebugCanvas::DrawLayerToPicture(SkCanvas *canvas, const LayerSpan &layer,
                                     int relStopIndex, bool show_clip,
                                     bool highlight_geometry)
{
    SkRect cull = layer.picture->cullRect();
    InterceptCanvas ic((int)cull.width(), (int)cull.height(), canvas,
                       relStopIndex, highlight_geometry);
    layer.picture->playback(&ic);

    if (show_clip)
    {
        SkPaint overlay;
        overlay.setColor(0x600099FF);
        canvas->drawRect(cull, overlay);
    }
}
