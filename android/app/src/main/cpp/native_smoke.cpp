#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

#include "storm/math/Mat4.hpp"
#include "storm/render/Camera3D.hpp"
#include "storm/render/GlesContext.hpp"
#include "storm/render/GlesRenderDevice.hpp"
#include "storm/render/Material.hpp"

namespace {
std::mutex gMutex;
std::thread gThread;
std::atomic_bool gRunning{false};

std::array<float, 16> toGlMatrix(const storm::math::Mat4& m) {
    std::array<float, 16> out{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            out[col * 4 + row] = m.m[row][col];
    return out;
}

void renderLoop(ANativeWindow* window) {
    storm::render::GlesContext context;
    if (!context.initializeWindow(window)) {
        ANativeWindow_release(window);
        gRunning = false;
        return;
    }

    storm::render::GlesRenderDevice device;

    struct Vertex { float x,y,z,r,g,b,a; };
    const Vertex vertices[] = {
        {-1,-1,-1,1,0,0,1},{1,-1,-1,0,1,0,1},{1,1,-1,0,0,1,1},{-1,1,-1,1,1,0,1},
        {-1,-1,1,1,0,1,1},{1,-1,1,0,1,1,1},{1,1,1,1,1,1,1},{-1,1,1,.2f,.6f,1,1}
    };
    const std::uint16_t indices[] = {
        0,1,2,2,3,0, 4,6,5,6,4,7, 0,4,5,5,1,0,
        3,2,6,6,7,3, 0,3,7,7,4,0, 1,5,6,6,2,1
    };

    const auto vb=device.createBuffer({sizeof(vertices),storm::render::BufferUsage::Static});
    const auto ib=device.createBuffer({sizeof(indices),storm::render::BufferUsage::Static});
    if(!vb.valid()||!ib.valid()||!device.updateBuffer(vb,vertices,sizeof(vertices))||
       !device.updateBuffer(ib,indices,sizeof(indices))) {
        context.shutdown(); ANativeWindow_release(window); gRunning=false; return;
    }

    constexpr const char* vs=R"glsl(#version 300 es
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main(){gl_Position=uMVP*vec4(aPosition,1.0);vColor=aColor;})glsl";
    constexpr const char* fs=R"glsl(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 outColor;
void main(){outColor=vColor;})glsl";

    const auto vsh=device.createShader({storm::render::ShaderStage::Vertex},vs);
    const auto fsh=device.createShader({storm::render::ShaderStage::Fragment},fs);
    storm::render::VertexLayout layout{};
    layout.attributeCount=2; layout.stride=sizeof(Vertex);
    layout.attributes[0]={0,storm::render::VertexFormat::Float32x3,0};
    layout.attributes[1]={1,storm::render::VertexFormat::Float32x4,3*sizeof(float)};

    storm::render::Material material;
    storm::render::Camera3D camera;
    camera.setPosition({0,0,5});
    camera.setPerspective(1.0471975512f,1,0.1f,100);

    float angle=0;
    auto last=std::chrono::steady_clock::now();
    while(gRunning.load()) {
        if(!context.makeCurrent()) break;
        const int w=ANativeWindow_getWidth(window), h=ANativeWindow_getHeight(window);
        if(w<=0||h<=0) break;
        glViewport(0,0,w,h);
        camera.setViewport((float)w,(float)h);
        glClearColor(.015f,.01f,.03f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        const auto now=std::chrono::steady_clock::now();
        float dt=std::chrono::duration<float>(now-last).count(); last=now;
        angle += dt<0.05f?dt:0.05f;
        const auto model=storm::math::Mat4::rotationY(angle)*storm::math::Mat4::rotationX(angle*.7f);
        material.setParameter("uMVP",toGlMatrix(camera.viewProjectionMatrix()*model));

        storm::render::DrawCommand cmd{};
        cmd.topology=storm::render::PrimitiveTopology::Triangles;
        cmd.vertexBuffer=vb; cmd.indexBuffer=ib; cmd.vertexCount=8; cmd.indexCount=36;
        cmd.indexType=storm::render::IndexType::UInt16; cmd.vertexLayout=layout;
        cmd.shader=vsh; cmd.fragmentShader=fsh; cmd.materialData=&material;
        device.beginFrame(); device.submit(cmd); device.endFrame();
        context.swap();
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    device.destroyShader(vsh); device.destroyShader(fsh);
    device.destroyBuffer(ib); device.destroyBuffer(vb);
    context.shutdown(); ANativeWindow_release(window); gRunning=false;
}
}

extern "C" JNIEXPORT void JNICALL
Java_storm_engine_smoke_MainActivity_nativeStart(JNIEnv* env,jclass,jobject surface) {
    if(!env||!surface) return;
    std::lock_guard<std::mutex> lock(gMutex);
    if(gRunning.load()) return;
    ANativeWindow* window=ANativeWindow_fromSurface(env,surface);
    if(!window) return;
    gRunning=true;
    gThread=std::thread(renderLoop,window);
}

extern "C" JNIEXPORT void JNICALL
Java_storm_engine_smoke_MainActivity_nativeStop(JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(gMutex);
    gRunning=false;
    if(gThread.joinable()) gThread.join();
}
