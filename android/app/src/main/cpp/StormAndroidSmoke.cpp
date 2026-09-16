#include <jni.h>
#include <GLES3/gl3.h>

#include <string>

#include "storm/render/GlesContext.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_storm_engine_smoke_MainActivity_runGpuSmokeTest(JNIEnv* env, jclass) {
    storm::render::GlesContext context;
    if (!context.initializePbuffer(1, 1))
        return env->NewStringUTF("GLES INIT FAILED");

    const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    glViewport(0, 0, 1, 1);
    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    const GLenum error = glGetError();
    const std::string result =
        error == GL_NO_ERROR
            ? "GLES OK\nVendor: " + std::string(vendor ? vendor : "unknown") +
                  "\nRenderer: " + std::string(renderer ? renderer : "unknown") +
                  "\nVersion: " + std::string(version ? version : "unknown")
            : "GLES DRAW/CLEAR FAILED";

    context.shutdown();
    return env->NewStringUTF(result.c_str());
}
