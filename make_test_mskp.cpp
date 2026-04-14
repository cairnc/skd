#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkDocument.h"
#include "include/core/SkStream.h"
#include "include/docs/SkMultiPictureDocument.h"

#include <cmath>
#include <cstdio>

static constexpr int kWidth = 480;
static constexpr int kHeight = 320;
static constexpr int kFrames = 60;
static constexpr float kBallRadius = 24.0f;

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "test.mskp";

    SkFILEWStream out(path);
    if (!out.isValid())
    {
        fprintf(stderr, "Could not open %s\n", path);
        return 1;
    }

    sk_sp<SkDocument> doc = SkMultiPictureDocument::Make(&out);

    float x = kBallRadius, y = kBallRadius;
    float vx = 7.5f, vy = 4.2f;

    for (int i = 0; i < kFrames; i++)
    {
        x += vx;
        y += vy;
        if (x < kBallRadius || x > kWidth - kBallRadius)
        {
            vx = -vx;
            x = std::max(kBallRadius, std::min(x, (float)kWidth - kBallRadius));
        }
        if (y < kBallRadius || y > kHeight - kBallRadius)
        {
            vy = -vy;
            y = std::max(kBallRadius,
                         std::min(y, (float)kHeight - kBallRadius));
        }

        SkCanvas *c = doc->beginPage(kWidth, kHeight);

        SkPaint bg;
        bg.setColor(0xFF1E1E1E);
        c->drawPaint(bg);

        // Ball
        SkPaint ball;
        ball.setAntiAlias(true);
        ball.setColor(0xFFFF3344);
        c->drawCircle(x, y, kBallRadius, ball);

        SkPaint highlight;
        highlight.setAntiAlias(true);
        highlight.setColor(0xFFFFAAAA);
        c->drawCircle(x - kBallRadius * 0.3f, y - kBallRadius * 0.3f,
                      kBallRadius * 0.3f, highlight);

        doc->endPage();
    }

    doc->close();
    printf("Wrote %s (%d frames, %dx%d)\n", path, kFrames, kWidth, kHeight);
    return 0;
}
