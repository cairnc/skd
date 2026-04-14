#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkData.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkImage.h"
#include "include/core/SkSerialProcs.h"
#include "include/core/SkStream.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkRRect.h"
#include "include/core/SkStream.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkGradient.h"
#include "include/effects/SkImageFilters.h"
#include "include/core/SkBlendMode.h"
#include "include/effects/SkRuntimeEffect.h"

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "test.skp";

    SkPictureRecorder recorder;
    SkCanvas *c = recorder.beginRecording(512, 512);

    // Background
    SkPaint bg;
    bg.setColor(SK_ColorWHITE);
    c->drawPaint(bg);

    // Annotation
    sk_sp<SkData> anno_data = SkData::MakeWithCString("layer-root");
    c->drawAnnotation(SkRect::MakeWH(512, 512), "RenderNode", anno_data.get());

    // Red rect
    SkPaint red;
    red.setColor(SK_ColorRED);
    red.setAntiAlias(true);
    c->drawRect(SkRect::MakeXYWH(20, 20, 200, 100), red);

    // Rounded rect with linear gradient
    SkPoint gpts[] = {{50, 150}, {230, 270}};
    SkColor4f gcols[] = {SkColors::kBlue, SkColors::kCyan};
    SkGradient grad(SkGradient::Colors({gcols, 2}, SkTileMode::kClamp), {});
    SkPaint gradient_paint;
    gradient_paint.setShader(SkShaders::LinearGradient(gpts, grad));
    gradient_paint.setAntiAlias(true);
    c->drawRRect(
        SkRRect::MakeRectXY(SkRect::MakeXYWH(50, 150, 180, 120), 20, 20),
        gradient_paint);

    // Annotation for the next group
    sk_sp<SkData> group_data = SkData::MakeWithCString("shapes-group");
    c->drawAnnotation(SkRect::MakeXYWH(0, 0, 512, 512), "LayerGroup",
                      group_data.get());

    // Stroked triangle with dash path effect
    float intervals[] = {12, 6};
    SkPaint dashed;
    dashed.setColor(SK_ColorGREEN);
    dashed.setAntiAlias(true);
    dashed.setStyle(SkPaint::kStroke_Style);
    dashed.setStrokeWidth(3);
    dashed.setPathEffect(SkDashPathEffect::Make(intervals, 0));
    SkPath triangle = SkPathBuilder()
                          .moveTo(256, 50)
                          .lineTo(450, 400)
                          .lineTo(62, 400)
                          .close()
                          .detach();
    c->drawPath(triangle, dashed);

    // Yellow oval with blur mask filter
    SkPaint blurry;
    blurry.setColor(SK_ColorYELLOW);
    blurry.setAntiAlias(true);
    blurry.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
    c->drawOval(SkRect::MakeXYWH(280, 80, 180, 120), blurry);

    // Rect with color matrix filter (desaturate)
    float matrix[20] = {
        0.2126f, 0.7152f, 0.0722f, 0, 0, 0.2126f, 0.7152f, 0.0722f, 0, 0,
        0.2126f, 0.7152f, 0.0722f, 0, 0, 0,       0,       0,       1, 0,
    };
    SkPaint filtered;
    filtered.setColor(0xFFFF6600);
    filtered.setColorFilter(SkColorFilters::Matrix(matrix));
    filtered.setAntiAlias(true);
    c->drawRect(SkRect::MakeXYWH(300, 300, 100, 80), filtered);

    // Circles
    for (int i = 0; i < 5; i++)
    {
        SkPaint dot;
        dot.setColor(SkColorSetARGB(255, 50 * i, 100, 255 - 50 * i));
        dot.setAntiAlias(true);
        c->drawCircle(100 + i * 70, 450, 20, dot);
    }

    // Draw a small generated bitmap image
    SkBitmap checkerboard;
    checkerboard.allocPixels(SkImageInfo::MakeN32Premul(32, 32));
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            *checkerboard.getAddr32(x, y) =
                ((x / 4 + y / 4) % 2) ? 0xFFFFFFFF : 0xFF000000;
    sk_sp<SkImage> checker_img = SkImages::RasterFromBitmap(checkerboard);
    c->drawImage(checker_img, 400, 400);

    // Save/restore + translate + rotate with blur image filter
    c->save();
    c->translate(300, 300);
    c->rotate(30);
    SkPaint rotated;
    rotated.setColor(0x88FF00FF);
    rotated.setAntiAlias(true);
    rotated.setImageFilter(
        SkImageFilters::Blur(4, 4, SkTileMode::kClamp, nullptr));
    c->drawRect(SkRect::MakeXYWH(-40, -40, 80, 80), rotated);
    c->restore();

    // Clip RRect
    c->save();
    c->clipRRect(
        SkRRect::MakeRectXY(SkRect::MakeXYWH(350, 50, 120, 80), 10, 10));
    SkPaint clipped;
    clipped.setColor(0xFF009900);
    c->drawPaint(clipped);
    c->restore();

    // Clip difference: outer rect minus inner rect
    c->save();
    c->clipRect(SkRect::MakeXYWH(20, 400, 150, 80));
    c->clipRect(SkRect::MakeXYWH(50, 415, 90, 50), SkClipOp::kDifference);
    SkPaint diff_paint;
    diff_paint.setColor(0xFF8800CC);
    c->drawPaint(diff_paint);
    c->restore();

    // Clip path: star shape
    SkPathBuilder star;
    float cx = 420, cy = 440, r1 = 40, r2 = 18;
    for (int j = 0; j < 10; j++)
    {
        float angle = j * 3.14159f / 5.0f - 3.14159f / 2.0f;
        float r = (j % 2 == 0) ? r1 : r2;
        float px = cx + cosf(angle) * r;
        float py = cy + sinf(angle) * r;
        if (j == 0)
            star.moveTo(px, py);
        else
            star.lineTo(px, py);
    }
    star.close();
    c->save();
    c->clipPath(star.detach(), true);
    SkPaint star_paint;
    star_paint.setColor(0xFFFF8800);
    c->drawPaint(star_paint);
    c->restore();

    // Nested clips: RRect inside rotated rect
    c->save();
    c->translate(300, 440);
    c->rotate(15);
    c->clipRect(SkRect::MakeXYWH(-50, -30, 100, 60));
    c->clipRRect(SkRRect::MakeOval(SkRect::MakeXYWH(-40, -25, 80, 50)));
    SkPaint nested_paint;
    nested_paint.setColor(0xFF00CCCC);
    c->drawPaint(nested_paint);
    c->restore();

    // Radial gradient oval
    SkColor4f radial_cols[] = {SkColors::kRed, SkColors::kTransparent};
    SkGradient radial_grad(
        SkGradient::Colors({radial_cols, 2}, SkTileMode::kClamp), {});
    SkPaint radial_paint;
    radial_paint.setShader(
        SkShaders::RadialGradient({256, 256}, 150, radial_grad));
    radial_paint.setAntiAlias(true);
    c->drawOval(SkRect::MakeXYWH(156, 156, 200, 200), radial_paint);

    // Runtime SkSL shader
    auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(R"(
        uniform float2 iResolution;
        half4 main(float2 fragCoord) {
            float2 uv = fragCoord / iResolution;
            half3 col = half3(uv.x, uv.y, 0.5 + 0.5 * sin(uv.x * 10.0));
            return half4(col, 1.0);
        }
    )"));
    if (effect)
    {
        SkRuntimeShaderBuilder builder(effect);
        builder.uniform("iResolution") = SkV2{100, 100};
        SkPaint sksl_paint;
        sksl_paint.setShader(builder.makeShader());
        sksl_paint.setAntiAlias(true);
        c->drawRect(SkRect::MakeXYWH(20, 300, 100, 100), sksl_paint);
    }

    // Composed shaders: blend two runtime effects together
    auto [fx_a, err_a] = SkRuntimeEffect::MakeForShader(SkString(R"(
        half4 main(float2 p) {
            float d = length(p - float2(50)) / 50.0;
            return half4(half3(1.0 - d), 1.0);
        }
    )"));
    auto [fx_b, err_b] = SkRuntimeEffect::MakeForShader(SkString(R"(
        half4 main(float2 p) {
            float s = sin(p.x * 0.1) * cos(p.y * 0.1);
            return half4(half3(s * 0.5 + 0.5, 0.2, 0.8), 1.0);
        }
    )"));
    if (fx_a && fx_b)
    {
        sk_sp<SkShader> shader_a = SkRuntimeShaderBuilder(fx_a).makeShader();
        sk_sp<SkShader> shader_b = SkRuntimeShaderBuilder(fx_b).makeShader();
        sk_sp<SkShader> blended =
            SkShaders::Blend(SkBlendMode::kMultiply, shader_a, shader_b);

        // Composed color filter chain
        sk_sp<SkColorFilter> cf_desat = SkColorFilters::Matrix(matrix);
        sk_sp<SkColorFilter> cf_tint =
            SkColorFilters::Blend(0x4400FF00, SkBlendMode::kSrcOver);
        sk_sp<SkColorFilter> cf_chain =
            SkColorFilters::Compose(cf_tint, cf_desat);

        // Composed image filter: blur -> drop shadow -> color filter
        sk_sp<SkImageFilter> if_blur =
            SkImageFilters::Blur(2, 2, SkTileMode::kClamp, nullptr);
        sk_sp<SkImageFilter> if_shadow =
            SkImageFilters::DropShadow(4, 4, 3, 3, SK_ColorBLACK, if_blur);
        sk_sp<SkImageFilter> if_cf =
            SkImageFilters::ColorFilter(cf_desat, if_shadow);

        SkPaint composed;
        composed.setShader(blended);
        composed.setColorFilter(cf_chain);
        composed.setImageFilter(if_cf);
        composed.setAntiAlias(true);
        c->drawRRect(
            SkRRect::MakeRectXY(SkRect::MakeXYWH(150, 300, 120, 120), 12, 12),
            composed);
    }

    // Orientation-clear test images (asymmetric so flips are obvious).
    auto make_f_bitmap = [](int size)
    {
        SkBitmap bm;
        bm.allocPixels(SkImageInfo::MakeN32Premul(size, size));
        // White background.
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
                *bm.getAddr32(x, y) = 0xFFFFFFFF;
        // Black capital "F" shape — top-heavy, asymmetric, rotation-obvious.
        auto fill = [&](int x0, int y0, int w, int h)
        {
            for (int y = y0; y < y0 + h && y < size; y++)
                for (int x = x0; x < x0 + w && x < size; x++)
                    *bm.getAddr32(x, y) = 0xFF000000;
        };
        int u = size / 8;
        fill(2 * u, u, u, 6 * u);     // vertical stem
        fill(2 * u, u, 5 * u, u);     // top bar
        fill(2 * u, 3 * u, 4 * u, u); // middle bar (shorter)
        return SkImages::RasterFromBitmap(bm);
    };
    auto make_gradient_bitmap = [](int size)
    {
        // Red at top → blue at bottom. Clear "which end is up".
        SkBitmap bm;
        bm.allocPixels(SkImageInfo::MakeN32Premul(size, size));
        for (int y = 0; y < size; y++)
        {
            float t = (float)y / (float)(size - 1);
            uint8_t r = (uint8_t)((1.0f - t) * 255);
            uint8_t b = (uint8_t)(t * 255);
            uint32_t px = 0xFF000000 | (b << 16) | (r);
            for (int x = 0; x < size; x++)
                *bm.getAddr32(x, y) = px;
        }
        return SkImages::RasterFromBitmap(bm);
    };

    sk_sp<SkImage> f_img = make_f_bitmap(64);
    sk_sp<SkImage> grad_img = make_gradient_bitmap(64);

    // Draw them directly — if flip_y bug regresses, F is upside-down.
    c->drawImage(f_img, 20, 50);
    c->drawImage(grad_img, 100, 50);

    // Nest f_img deep inside a paint via shader chain:
    //   ImageShader -> LocalMatrix (rotation) -> Blend with another shader
    // This exercises the serialize-proc image collector through several
    // wrapper layers.
    sk_sp<SkShader> image_shader = f_img->makeShader(
        SkTileMode::kRepeat, SkTileMode::kRepeat, SkSamplingOptions());
    SkMatrix lm;
    lm.setRotate(20);
    sk_sp<SkShader> rotated_image = image_shader->makeWithLocalMatrix(lm);
    SkColor4f mix_cols[] = {SkColors::kYellow, SkColors::kTransparent};
    SkGradient mix_grad(SkGradient::Colors({mix_cols, 2}, SkTileMode::kClamp),
                        {});
    SkPoint mix_pts[] = {{0, 0}, {80, 80}};
    sk_sp<SkShader> tint = SkShaders::LinearGradient(mix_pts, mix_grad);
    sk_sp<SkShader> chain =
        SkShaders::Blend(SkBlendMode::kMultiply, rotated_image, tint);
    SkPaint nested_img_paint;
    nested_img_paint.setShader(chain);
    nested_img_paint.setAntiAlias(true);
    c->drawRect(SkRect::MakeXYWH(20, 130, 120, 80), nested_img_paint);

    // Also use grad_img as the source of an image filter.
    sk_sp<SkImageFilter> image_src_filter = SkImageFilters::Image(
        grad_img, SkRect::MakeWH(grad_img->width(), grad_img->height()),
        SkRect::MakeXYWH(160, 130, 120, 80), SkSamplingOptions());
    sk_sp<SkImageFilter> blurred =
        SkImageFilters::Blur(2, 2, SkTileMode::kClamp, image_src_filter);
    SkPaint if_paint;
    if_paint.setImageFilter(blurred);
    c->drawRect(SkRect::MakeXYWH(160, 130, 120, 80), if_paint);

    // Final annotation
    sk_sp<SkData> end_data = SkData::MakeWithCString("end-of-frame");
    c->drawAnnotation(SkRect::MakeEmpty(), "FrameEnd", end_data.get());

    sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

    SkSerialProcs sprocs;
    sprocs.fImageProc = [](SkImage *img, void *) -> sk_sp<const SkData>
    {
        SkBitmap bm;
        bm.allocPixels(SkImageInfo::MakeN32Premul(img->width(), img->height()));
        if (!img->readPixels(bm.pixmap(), 0, 0))
            return nullptr;
        SkDynamicMemoryWStream buf;
        if (!SkPngEncoder::Encode(&buf, bm.pixmap(), {}))
            return nullptr;
        return buf.detachAsData();
    };
    sk_sp<SkData> skp_data = picture->serialize(&sprocs);
    SkFILEWStream stream(path);
    stream.write(skp_data->data(), skp_data->size());
    printf("Wrote %s (%d ops, %zu bytes)\n", path,
           picture->approximateOpCount(), picture->approximateBytesUsed());
    return 0;
}
