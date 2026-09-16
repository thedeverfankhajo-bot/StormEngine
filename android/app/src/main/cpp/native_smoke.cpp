#include <cstdio>
#include <jni.h>
#include <GLES3/gl3.h>

#include "storm/render/GlesContext.hpp"
#include "storm/render/GlesRenderDevice.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_storm_engine_smoke_MainActivity_nativeRunSmoke(JNIEnv* env, jobject) {
    storm::render::GlesContext context;
    if (!context.initializePbuffer(16, 16)) return env->NewStringUTF("FAIL:EGL initialization");

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

    storm::render::GlesRenderDevice device;
    const auto vertexBuffer = device.createBuffer({3 * 7 * sizeof(float), storm::render::BufferUsage::Static});
    const float triangle[] = {
        -0.70f, -0.70f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         0.70f, -0.70f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,
         0.00f,  0.70f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f,
    };
    const bool bufferOk = vertexBuffer.valid() && device.updateBuffer(vertexBuffer, triangle, sizeof(triangle));

    constexpr const char* vertexSource = R"glsl(#version 300 es
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
out vec4 vColor;
void main(){ gl_Position=vec4(aPosition,1.0); vColor=aColor; })glsl";
    constexpr const char* fragmentSource = R"glsl(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 outColor;
void main(){ outColor=vColor; })glsl";

    const auto vertexShader = device.createShader({storm::render::ShaderStage::Vertex}, vertexSource);
    const auto fragmentShader = device.createShader({storm::render::ShaderStage::Fragment}, fragmentSource);
    const bool shaderOk = vertexShader.valid() && fragmentShader.valid() && device.liveShaderCount() == 2;

    storm::render::VertexLayout layout{};
    layout.attributeCount = 2;
    layout.stride = 7 * sizeof(float);
    layout.attributes[0] = {0, storm::render::VertexFormat::Float32x3, 0};
    layout.attributes[1] = {1, storm::render::VertexFormat::Float32x4, 3 * sizeof(float)};

    storm::render::DrawCommand command{};
    command.topology = storm::render::PrimitiveTopology::Triangles;
    command.vertexBuffer = vertexBuffer;
    command.vertexCount = 3;
    command.vertexLayout = layout;
    command.shader = vertexShader;
    command.fragmentShader = fragmentShader;

    device.beginFrame();
    const bool drawOk = bufferOk && shaderOk && device.submit(command) && device.submittedDrawCount() == 1;
    device.endFrame();

    glFinish();
    if (glGetError() != GL_NO_ERROR) {
        device.destroyShader(vertexShader);
        device.destroyShader(fragmentShader);
        device.destroyBuffer(vertexBuffer);
        context.shutdown();
        return env->NewStringUTF("FAIL:GPU draw");
    }

    unsigned char pixel[4] = {0, 0, 0, 0};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    const bool triangleVisible = pixel[0] > 20 || pixel[1] > 20 || pixel[2] > 20;

    const auto liveBeforeDestroy = device.liveBufferCount();
    device.destroyShader(vertexShader);
    device.destroyShader(fragmentShader);
    device.destroyBuffer(vertexBuffer);
    const bool resourcesOk = liveBeforeDestroy == 1 && device.liveBufferCount() == 0 && device.liveShaderCount() == 0;

    char result[1024];
    std::snprintf(result, sizeof(result),
                  "GLES SHADER TRIANGLE %s\nVENDOR=%s\nRENDERER=%s\nVERSION=%s\nPIXEL=%u,%u,%u,%u",
                  (drawOk && triangleVisible && resourcesOk) ? "OK" : "FAIL",
                  vendor, renderer, version, pixel[0], pixel[1], pixel[2], pixel[3]);
    context.shutdown();

    if (!drawOk) return env->NewStringUTF("FAIL:GLES shader draw submission");
    if (!triangleVisible) return env->NewStringUTF("FAIL:triangle readback");
    if (!resourcesOk) return env->NewStringUTF("FAIL:GLES shader resources");
    return env->NewStringUTF(result);
}
