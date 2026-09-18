#include <android/native_window.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <jni.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <algorithm>

#include "storm/math/Mat4.hpp"
#include "storm/render/GlesContext.hpp"
#include "storm/render/GlesRenderDevice.hpp"
#include "storm/render/Material.hpp"

namespace {
std::mutex gMutex;
std::thread gThread;
std::atomic_bool gRunning{false};

constexpr float kPi = 3.14159265358979323846f;

void renderLoop(ANativeWindow* window) {
    storm::render::GlesContext context;
    if (!context.initializeWindow(window)) {
        ANativeWindow_release(window);
        gRunning.store(false);
        return;
    }

    storm::render::GlesRenderDevice device;

    // 36 vertices: six independent faces, which keeps the demo simple and
    // leaves room for future normal/UV seams without changing the API.
    constexpr float vertices[] = {
        // position                  color
        -1,-1, 1,  1,0,0,1,   1,-1, 1,  0,1,0,1,   1, 1, 1,  0,0,1,1,
        -1,-1, 1,  1,0,0,1,   1, 1, 1,  0,0,1,1,  -1, 1, 1,  1,1,0,1,
         1,-1,-1,  1,0,1,1,  -1,-1,-1,  0,1,1,1,  -1, 1,-1,  1,1,1,1,
         1,-1,-1,  1,0,1,1,  -1, 1,-1,  1,1,1,1,   1, 1,-1,  0.5f,0.5f,0,1,
        -1,-1,-1,  0,1,1,1,  -1,-1, 1,  1,0.5f,0,1, -1, 1, 1,  0,0.5f,1,1,
        -1,-1,-1,  0,1,1,1,  -1, 1, 1,  0,0.5f,1,1, -1, 1,-1,  1,0,0.5f,1,
         1,-1, 1,  0.5f,0,1,1,  1,-1,-1,  1,0,1,1,   1, 1,-1,  0.5f,0.5f,0,1,
         1,-1, 1,  0.5f,0,1,1,  1, 1,-1,  0.5f,0.5f,0,1,   1, 1, 1,  0,0.5f,0.5f,1,
        -1, 1, 1,  1,1,0,1,   1, 1, 1,  0,0,1,1,   1, 1,-1,  0.5f,0.5f,0,1,
        -1, 1, 1,  1,1,0,1,   1, 1,-1,  0.5f,0.5f,0,1,  -1, 1,-1,  1,1,1,1,
        -1,-1,-1,  0,1,1,1,   1,-1,-1,  1,0,1,1,    1,-1, 1,  0.5f,0,1,1,
        -1,-1,-1,  0,1,1,1,   1,-1, 1,  0.5f,0,1,1,   -1,-1, 1,  1,0.5f,0,1
    };

    constexpr const char* vertexSource = R"glsl(#version 300 es
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() {
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vColor = aColor;
})glsl";

    constexpr const char* fragmentSource = R"glsl(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 outColor;
void main() { outColor = vColor; })glsl";

    const auto vb = device.createBuffer({sizeof(vertices), storm::render::BufferUsage::Static});
    const auto vs = device.createShader({storm::render::ShaderStage::Vertex}, vertexSource);
    const auto fs = device.createShader({storm::render::ShaderStage::Fragment}, fragmentSource);
    if (!vb.valid() || !vs.valid() || !fs.valid() || !device.updateBuffer(vb, vertices, sizeof(vertices))) {
        gRunning.store(false);
        device.destroyShader(vs);
        device.destroyShader(fs);
        device.destroyBuffer(vb);
        context.shutdown();
        ANativeWindow_release(window);
        return;
    }

    storm::render::VertexLayout layout{};
    layout.attributeCount = 2;
    layout.stride = 7 * sizeof(float);
    layout.attributes[0] = {0, storm::render::VertexFormat::Float32x3, 0};
    layout.attributes[1] = {1, storm::render::VertexFormat::Float32x4, 3 * sizeof(float)};

    storm::render::Material material;
    storm::render::DrawCommand command{};
    command.topology = storm::render::PrimitiveTopology::Triangles;
    command.vertexBuffer = vb;
    command.vertexCount = 36;
    command.vertexLayout = layout;
    command.shader = vs;
    command.fragmentShader = fs;
    command.materialData = &material;

    storm::render::Camera3D camera;
    camera.setPosition({0.0f, 0.0f, 5.0f});
    camera.setPerspective(60.0f * kPi / 180.0f, 0.1f, 100.0f);

    float angle = 0.0f;
    auto previous = std::chrono::steady_clock::now();

    while (gRunning.load()) {
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - previous).count();
        previous = now;
        if (!std::isfinite(dt) || dt < 0.0f) dt = 0.0f;
        dt = std::min(dt, 0.1f);
        angle += dt;

        const int width = ANativeWindow_getWidth(window);
        const int height = ANativeWindow_getHeight(window);
        if (width <= 0 || height <= 0) continue;

        camera.setViewport(static_cast<float>(width), static_cast<float>(height));
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glClearColor(0.015f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const storm::math::Mat4 model =
            storm::math::Mat4::rotationY(angle) *
            storm::math::Mat4::rotationX(angle * 0.67f);
        const storm::math::Mat4 mvp = camera.viewProjectionMatrix() * model;

        storm::render::MaterialMat4 matrix{};
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                // Mat4 is row-major and multiplies column vectors; transpose
                // is required for glUniformMatrix4fv(..., GL_FALSE, ...).
                matrix[static_cast<std::size_t>(col * 4 + row)] = mvp.m[row][col];
        material.setParameter("uMVP", matrix);

        device.beginFrame();
        const bool submitted = device.submit(command);
        device.endFrame();

        if (!submitted || !context.swap()) {
            gRunning.store(false);
            break;
        }
    }

    device.destroyShader(vs);
    device.destroyShader(fs);
    device.destroyBuffer(vb);
    context.shutdown();
    ANativeWindow_release(window);
}

} // namespace

extern "C" JNIEXPORT void JNICALL
Java_storm_engine_smoke_MainActivity_nativeStart(JNIEnv* env, jclass, jobject surface) {
    if (!env || !surface) return;

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) return;

    {
        std::lock_guard<std::mutex> lock(gMutex);
        if (gRunning.load() || gThread.joinable()) {
            ANativeWindow_release(window);
            return;
        }
        gRunning.store(true);
        gThread = std::thread(renderLoop, window);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_storm_engine_smoke_MainActivity_nativeStop(JNIEnv*, jclass) {
    gRunning.store(false);
    std::thread oldThread;
    {
        std::lock_guard<std::mutex> lock(gMutex);
        if (gThread.joinable()) oldThread = std::move(gThread);
    }
    if (oldThread.joinable()) oldThread.join();
}
