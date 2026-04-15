// Generates an MSKP that mimics how Android HWUI captures offscreen
// layers AND how Android serializes images across pages:
//  - Each layer's draw commands live in a child SkPicture introduced by
//    an "OffscreenLayerDraw|<id>" annotation, then later composited
//    into the parent canvas via a "SurfaceID|<id>" annotation followed
//    by a drawImageRect of a placeholder image.
//  - Images that appear on more than one page are serialized with a
//    "sharing" context: full PNG bytes on the first page, and a 4-byte
//    in-file id reference on every subsequent page. Without a matching
//    deserializer this shows up in skpviz as 1x1 placeholder images on
//    every frame after the first.
// A "logo" image is referenced inside layer A each frame so the
// cross-page sharing path is exercised via real draw content (not just
// the SurfaceID composite placeholder).

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkData.h"
#include "include/core/SkDocument.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRect.h"
#include "include/core/SkSerialProcs.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/docs/SkMultiPictureDocument.h"
#include "include/encode/SkPngEncoder.h"

#include <cmath>
#include <cstdio>
#include <unordered_map>

static constexpr int kWidth = 600;
static constexpr int kHeight = 400;
static constexpr int kFrames = 8;

static sk_sp<SkImage> MakeDummyImage()
{
    SkBitmap bm;
    bm.allocPixels(
        SkImageInfo::Make(8, 8, kRGBA_8888_SkColorType, kPremul_SkAlphaType));
    bm.eraseColor(SK_ColorMAGENTA);
    return bm.asImage();
}

// A recognizable 64x64 image. Shared across every frame so the MSKP
// writer deduplicates it via the sharing serial proc — exercising the
// "4-byte id reference" path on frames >= 1.
static sk_sp<SkImage> MakeLogoImage()
{
    SkBitmap bm;
    bm.allocPixels(
        SkImageInfo::Make(64, 64, kRGBA_8888_SkColorType, kPremul_SkAlphaType));
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
        {
            uint8_t r = (uint8_t)(x * 4);
            uint8_t g = (uint8_t)(y * 4);
            uint8_t b = (uint8_t)((x + y) * 2);
            uint32_t c = 0xFF000000 | (b << 16) | (g << 8) | r;
            *bm.getAddr32(x, y) = c;
        }
    return bm.asImage();
}

static sk_sp<SkPicture> MakeLayerA(int frame, const sk_sp<SkImage> &logo)
{
    SkPictureRecorder rec;
    SkCanvas *c = rec.beginRecording(160, 160);
    c->clear(SK_ColorTRANSPARENT);
    SkPaint disc;
    disc.setAntiAlias(true);
    disc.setColor(0xFF22AAFF);
    c->drawCircle(80, 80, 70, disc);

    // Cross-page shared image drawn inside the layer. On frames >= 1
    // this arrives at the deserializer as a 4-byte id reference; only
    // a correctly wired SharingDeserialContext can resolve it.
    c->drawImage(logo.get(), 16, 16);

    SkPaint inner;
    inner.setAntiAlias(true);
    inner.setColor(0xFFFFFFFF);
    float t = frame * 0.4f;
    float ox = 80 + std::cos(t) * 30;
    float oy = 80 + std::sin(t) * 30;
    c->drawCircle(ox, oy, 18, inner);
    return rec.finishRecordingAsPicture();
}

static sk_sp<SkPicture> MakeLayerB(int frame)
{
    SkPictureRecorder rec;
    SkCanvas *c = rec.beginRecording(240, 100);
    c->clear(SK_ColorTRANSPARENT);
    SkPaint pill;
    pill.setAntiAlias(true);
    pill.setColor(0xFFFFCC44);
    c->drawRRect(SkRRect::MakeRectXY(SkRect::MakeXYWH(4, 4, 232, 92), 16, 16),
                 pill);
    c->save();
    c->translate(120, 50);
    c->rotate(frame * 18.0f);
    SkPaint stroke;
    stroke.setAntiAlias(true);
    stroke.setColor(0xFF333333);
    stroke.setStyle(SkPaint::kStroke_Style);
    stroke.setStrokeWidth(6);
    c->drawLine(-34, 0, 34, 0, stroke);
    c->drawLine(0, -34, 0, 34, stroke);
    c->restore();
    return rec.finishRecordingAsPicture();
}

static void DrawOffscreenLayer(SkCanvas *parent, int id,
                               const sk_sp<SkPicture> &layer_pic)
{
    SkRect dirty = layer_pic->cullRect();
    char key[64];
    std::snprintf(key, sizeof(key), "OffscreenLayerDraw|%d", id);
    parent->drawAnnotation(dirty, key, nullptr);
    parent->drawPicture(layer_pic);
}

static void CompositeLayer(SkCanvas *parent, int id, const SkRect &dst,
                           const sk_sp<SkImage> &dummy, const SkRect &src)
{
    char key[64];
    std::snprintf(key, sizeof(key), "SurfaceID|%d", id);
    parent->drawAnnotation(dst, key, nullptr);
    parent->drawImageRect(dummy.get(), src, dst, SkSamplingOptions(), nullptr,
                          SkCanvas::kStrict_SrcRectConstraint);
}

// Sharing serial context — mirrors Skia's tools/SkSharingProc.cpp
// behavior inline so the test doesn't depend on that cpp (which isn't
// in libskia.a). First occurrence of an image ID writes PNG bytes;
// subsequent occurrences write the 4-byte in-file id Android emits.
struct SharingCtx
{
    std::unordered_map<uint32_t, int> id_to_fid;
};
static sk_sp<const SkData> SerializeImageShared(SkImage *img, void *ctx)
{
    SharingCtx *c = static_cast<SharingCtx *>(ctx);
    uint32_t id = img->uniqueID();
    auto it = c->id_to_fid.find(id);
    if (it != c->id_to_fid.end())
    {
        int fid = it->second;
        return SkData::MakeWithCopy(&fid, sizeof(fid));
    }
    auto data = SkPngEncoder::Encode(nullptr, img, {});
    if (!data)
        return SkData::MakeEmpty();
    int fid = (int)c->id_to_fid.size();
    c->id_to_fid[id] = fid;
    return data;
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "test_layers.mskp";

    SkFILEWStream out(path);
    if (!out.isValid())
    {
        std::fprintf(stderr, "Could not open %s\n", path);
        return 1;
    }

    SharingCtx share_ctx;
    SkSerialProcs sprocs;
    sprocs.fImageProc = SerializeImageShared;
    sprocs.fImageCtx = &share_ctx;

    sk_sp<SkDocument> doc = SkMultiPictureDocument::Make(&out, &sprocs);
    sk_sp<SkImage> dummy = MakeDummyImage();
    sk_sp<SkImage> logo = MakeLogoImage();

    for (int f = 0; f < kFrames; f++)
    {
        sk_sp<SkPicture> layer_a = MakeLayerA(f, logo);
        sk_sp<SkPicture> layer_b = MakeLayerB(f);

        SkCanvas *c = doc->beginPage(kWidth, kHeight);

        SkPaint bg;
        bg.setColor(0xFF1E1E1E);
        c->drawPaint(bg);

        DrawOffscreenLayer(c, /*id=*/1, layer_a);
        DrawOffscreenLayer(c, /*id=*/2, layer_b);

        SkPaint hdr;
        hdr.setColor(0xFF888888);
        c->drawRect(SkRect::MakeXYWH(20, 20, 560, 4), hdr);

        SkRect dst_a = SkRect::MakeXYWH(40 + f * 6.0f, 60, 160, 160);
        SkRect dst_b = SkRect::MakeXYWH(260, 240 + f * 4.0f, 240, 100);
        CompositeLayer(c, 1, dst_a, dummy, SkRect::MakeWH(160, 160));
        CompositeLayer(c, 2, dst_b, dummy, SkRect::MakeWH(240, 100));

        SkPaint label;
        label.setColor(0xFFCCCCCC);
        c->drawRect(SkRect::MakeXYWH(20, kHeight - 20, 560, 4), label);

        doc->endPage();
    }

    doc->close();
    std::printf("Wrote %s (%d frames, %dx%d, 2 offscreen layers per frame, "
                "sharing-serial images)\n",
                path, kFrames, kWidth, kHeight);
    return 0;
}
