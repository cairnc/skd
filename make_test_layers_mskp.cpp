// Generates an MSKP that mimics how Android HWUI captures offscreen
// layers: each layer's draw commands live in a child SkPicture introduced
// by an "OffscreenLayerDraw|<id>" annotation, then later composited into
// the parent canvas via a "SurfaceID|<id>" annotation followed by a
// drawImageRect of a placeholder image. skpviz substitutes the real
// rendered layer for the placeholder at replay time.

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
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/docs/SkMultiPictureDocument.h"

#include <cmath>
#include <cstdio>

static constexpr int kWidth = 600;
static constexpr int kHeight = 400;
static constexpr int kFrames = 8;

// Encode a tiny magenta bitmap as the placeholder image Android would have
// recorded for each SurfaceID drawImageRect. SkMultiPictureDocument
// serializes images into the page stream, so a real (encodable) SkImage
// is needed — bare raster-from-pixels won't survive the round trip.
static sk_sp<SkImage> MakeDummyImage()
{
    SkBitmap bm;
    bm.allocPixels(
        SkImageInfo::Make(8, 8, kRGBA_8888_SkColorType, kPremul_SkAlphaType));
    bm.eraseColor(SK_ColorMAGENTA);
    return bm.asImage();
}

static sk_sp<SkPicture> MakeLayerA(int frame)
{
    SkPictureRecorder rec;
    SkCanvas *c = rec.beginRecording(160, 160);
    c->clear(SK_ColorTRANSPARENT);
    SkPaint disc;
    disc.setAntiAlias(true);
    disc.setColor(0xFF22AAFF);
    c->drawCircle(80, 80, 70, disc);

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

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "test_layers.mskp";

    SkFILEWStream out(path);
    if (!out.isValid())
    {
        std::fprintf(stderr, "Could not open %s\n", path);
        return 1;
    }

    sk_sp<SkDocument> doc = SkMultiPictureDocument::Make(&out);
    sk_sp<SkImage> dummy = MakeDummyImage();

    for (int f = 0; f < kFrames; f++)
    {
        sk_sp<SkPicture> layer_a = MakeLayerA(f);
        sk_sp<SkPicture> layer_b = MakeLayerB(f);

        SkCanvas *c = doc->beginPage(kWidth, kHeight);

        SkPaint bg;
        bg.setColor(0xFF1E1E1E);
        c->drawPaint(bg);

        // Two offscreen layer draws. In Android the layer's commands are
        // recorded into a child SkPicture between the dirty annotation
        // and the drawPicture call.
        DrawOffscreenLayer(c, /*id=*/1, layer_a);
        DrawOffscreenLayer(c, /*id=*/2, layer_b);

        // A non-layer main draw between the layer recordings and the
        // composites — exercises the "command in main, between layers"
        // navigation case.
        SkPaint hdr;
        hdr.setColor(0xFF888888);
        c->drawRect(SkRect::MakeXYWH(20, 20, 560, 4), hdr);

        // Composite the layers back. dst rects move per-frame to make the
        // animation visible.
        SkRect dst_a = SkRect::MakeXYWH(40 + f * 6.0f, 60, 160, 160);
        SkRect dst_b = SkRect::MakeXYWH(260, 240 + f * 4.0f, 240, 100);
        CompositeLayer(c, 1, dst_a, dummy, SkRect::MakeWH(160, 160));
        CompositeLayer(c, 2, dst_b, dummy, SkRect::MakeWH(240, 100));

        // A trailing main draw after the composites.
        SkPaint label;
        label.setColor(0xFFCCCCCC);
        c->drawRect(SkRect::MakeXYWH(20, kHeight - 20, 560, 4), label);

        doc->endPage();
    }

    doc->close();
    std::printf("Wrote %s (%d frames, %dx%d, 2 offscreen layers per frame)\n",
                path, kFrames, kWidth, kHeight);
    return 0;
}
