/*
 * Copyright (C) 2021 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AsyncScreenCaptureListener.h"

#include <android-base/file.h>
#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SyncScreenCaptureListener.h>
#include <ui/DisplayState.h>

#include <cstdio>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using android::AsyncScreenCaptureListener;
using android::DisplayCaptureArgs;
using android::GraphicBuffer;
using android::IBinder;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;
using android::base::WriteStringToFile;
using android::gui::ScreenCaptureResults;
using android::ui::DisplayState;
using android::ui::PixelFormat;

#define ALS_COLOR_PATH "/sys/kernel/pix_manager/als_color"

constexpr int ALS_RADIUS = 64;
constexpr int ALS_POS_X = 651;
constexpr int ALS_POS_Y = 87;
constexpr int SCREENSHOT_INTERVAL = 1;

// See frameworks/base/services/core/jni/com_android_server_display_DisplayControl.cpp and
// frameworks/base/core/java/android/view/SurfaceControl.java
static sp<IBinder> getInternalDisplayToken() {
    const auto displayIds = SurfaceComposerClient::getPhysicalDisplayIds();
    sp<IBinder> token = SurfaceComposerClient::getPhysicalDisplayToken(displayIds[0]);
    return token;
}

void updateScreenBuffer() {
    static time_t lastScreenUpdate = 0;
    static sp<GraphicBuffer> outBuffer = new GraphicBuffer(
            10, 10, android::PIXEL_FORMAT_RGB_888,
            GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (now.tv_sec - lastScreenUpdate < SCREENSHOT_INTERVAL) {
        ALOGV("Update skipped because interval not expired at %ld", now.tv_sec);
        return;
    }

    sp<IBinder> display = getInternalDisplayToken();

    DisplayCaptureArgs captureArgs;
    captureArgs.displayToken = getInternalDisplayToken();
    captureArgs.pixelFormat = PixelFormat::RGBA_8888;
    captureArgs.sourceCrop = Rect(
            ALS_POS_X - ALS_RADIUS, ALS_POS_Y - ALS_RADIUS,
            ALS_POS_X + ALS_RADIUS, ALS_POS_Y + ALS_RADIUS);
    captureArgs.width = ALS_RADIUS * 2;
    captureArgs.height = ALS_RADIUS * 2;
    captureArgs.captureSecureLayers = true;

    DisplayState state;
    SurfaceComposerClient::getDisplayState(display, &state);

    sp<AsyncScreenCaptureListener> captureListener = new AsyncScreenCaptureListener(
        [](const ScreenCaptureResults& captureResults) {
            ALOGV("Capture results received");

            uint8_t *out;
            auto resultWidth = outBuffer->getWidth();
            auto resultHeight = outBuffer->getHeight();
            auto stride = outBuffer->getStride();

            captureResults.buffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void **>(&out));
            // we can sum this directly on linear light
            uint32_t rsum = 0, gsum = 0, bsum = 0;
            for (int y = 0; y < resultHeight; y++) {
                for (int x = 0; x < resultWidth; x++) {
                    rsum += out[y * (stride * 4) + x * 4];
                    gsum += out[y * (stride * 4) + x * 4 + 1];
                    bsum += out[y * (stride * 4) + x * 4 + 2];
                }
            }
            uint32_t max = resultWidth * resultHeight;
            WriteStringToFile("A:255R:" + std::to_string(rsum / max) +
                "G:" + std::to_string(gsum / max) +
                "B:" + std::to_string(bsum / max), ALS_COLOR_PATH);
            captureResults.buffer->unlock();
        }, 500);

    ScreenshotClient::captureDisplay(captureArgs, captureListener);
    ALOGV("Capture started at %ld", now.tv_sec);

    lastScreenUpdate = now.tv_sec;
}

static bool dummyRead(int fd) {
    char c;
    int rc;

    rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        ALOGE("failed to seek fd, err: %d", rc);
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        ALOGE("failed to read bool from fd, err: %d", rc);
        return false;
    }

    return c == 'A';
}

int main() {
    android::ProcessState::self()->setThreadPoolMaxThreadCount(0);
    android::ProcessState::self()->startThreadPool();

    int fd = open(ALS_COLOR_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("failed to open fd, err: %d", fd);
        return fd;
    }

    struct pollfd alsColorPoll = {
        .fd = fd,
        .events = POLLERR | POLLPRI,
        .revents = 0,
    };

    while (true) {
        if (!dummyRead(fd))
            return -1;

        int rc = poll(&alsColorPoll, 1, -1);
        if (rc < 0) {
            ALOGE("failed to poll fd, err: %d", rc);
            continue;
        }

        updateScreenBuffer();
    }

    return 0;
}
