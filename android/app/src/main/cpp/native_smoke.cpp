#include <cstdio>
#include <jni.h>
#include <GLES3/gl3.h>

#include "storm/render/GlesContext.hpp"
#include "storm/render/GlesRenderDevice.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_storm_engine_smoke_MainActivity_nativeRunSmoke(JNIEnv* env, jobject) {
    storm::render::GlesContext context;
    if (!context.initializePbuffer(16, 16))
        return env->NewStringUTF("FAIL:EGL initialization");

    const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (vendor == nullptr || renderer == nullptr || version == nullptr) {
        context.shutdown();
        return env->NewStringUTF("FAIL:GL strings");
    }

    glViewport(0, 0, 16, 16);
    glClearColor(0.05f, 0.02f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    if (glGetError() != GL_NO_ERROR) {
        context.shutdown();
        return env->NewStringUTF("FAIL:GPU clear");
    }

    storm::render::GlesRenderDevice device;
    const auto buffer = device.createBuffer({256, storm::render::BufferUsage::Static});
    const auto texture = device.createTexture({16, 16, 1});
    const bool resourcesOk = buffer.valid() && texture.valid() &&
                             device.liveBufferCount() == 1 &&
                             device.liveTextureCount() == 1;
    device.destroyBuffer(buffer);
    device.destroyTexture(texture);

    char result[1024];
    std::snprintf(result, sizeof(result), "OK\nVENDOR=%s\nRENDERER=%s\nVERSION=%s",
                  vendor, renderer, version);
    context.shutdown();

    if (!resourcesOk)
        return env->NewStringUTF("FAIL:GLES resources");
    return env->NewStringUTF(result);
}
