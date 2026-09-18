#include "storm/render/GlesContext.hpp"

#if defined(__ANDROID__)

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

namespace storm::render {
namespace {
bool createContext(EGLDisplay display, EGLConfig config, EGLSurface surface,
                   EGLContext& outContext) noexcept {
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) return false;
    outContext = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (outContext == EGL_NO_CONTEXT) return false;
    if (eglMakeCurrent(display, surface, surface, outContext) != EGL_TRUE) {
        eglDestroyContext(display, outContext);
        outContext = EGL_NO_CONTEXT;
        return false;
    }
    // Android display surfaces should normally be synchronized to the display.
    // A failure here is not fatal: EGL may legally ignore the requested interval.
    (void)eglSwapInterval(display, 1);
    return true;
}

bool chooseConfig(EGLDisplay display, EGLint surfaceType, EGLConfig& config) noexcept {
    const EGLint attributes[] = {
        EGL_SURFACE_TYPE, surfaceType,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    EGLint count = 0;
    return eglChooseConfig(display, attributes, &config, 1, &count) == EGL_TRUE && count > 0;
}
}

GlesContext::~GlesContext() { shutdown(); }

bool GlesContext::initializePbuffer(int width, int height) noexcept {
    shutdown();
    if (width <= 0 || height <= 0) return false;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) return false;
    EGLint major = 0, minor = 0;
    if (eglInitialize(display, &major, &minor) != EGL_TRUE) return false;

    EGLConfig config = nullptr;
    if (!chooseConfig(display, EGL_PBUFFER_BIT, config)) {
        eglTerminate(display);
        return false;
    }

    const EGLint surfaceAttribs[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    EGLSurface surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
    if (surface == EGL_NO_SURFACE) {
        eglTerminate(display);
        return false;
    }

    EGLContext context = EGL_NO_CONTEXT;
    if (!createContext(display, config, surface, context)) {
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return false;
    }

    display_ = display;
    surface_ = surface;
    context_ = context;
    valid_ = true;
    return true;
}

bool GlesContext::initializeWindow(void* nativeWindow) noexcept {
    shutdown();
    if (nativeWindow == nullptr) return false;

    auto* window = static_cast<ANativeWindow*>(nativeWindow);
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) return false;
    EGLint major = 0, minor = 0;
    if (eglInitialize(display, &major, &minor) != EGL_TRUE) return false;

    EGLConfig config = nullptr;
    if (!chooseConfig(display, EGL_WINDOW_BIT, config)) {
        eglTerminate(display);
        return false;
    }

    EGLint nativeFormat = 0;
    if (eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nativeFormat) != EGL_TRUE ||
        nativeFormat == 0 || ANativeWindow_setBuffersGeometry(window, 0, 0, nativeFormat) != 0) {
        eglTerminate(display);
        return false;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        eglTerminate(display);
        return false;
    }

    EGLContext context = EGL_NO_CONTEXT;
    if (!createContext(display, config, surface, context)) {
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return false;
    }

    display_ = display;
    surface_ = surface;
    context_ = context;
    valid_ = true;
    return true;
}

bool GlesContext::makeCurrent() noexcept {
    if (!valid_) return false;
    return eglMakeCurrent(static_cast<EGLDisplay>(display_),
                          static_cast<EGLSurface>(surface_),
                          static_cast<EGLSurface>(surface_),
                          static_cast<EGLContext>(context_)) == EGL_TRUE;
}

bool GlesContext::swap() noexcept {
    if (!valid_) return false;
    return eglSwapBuffers(static_cast<EGLDisplay>(display_),
                          static_cast<EGLSurface>(surface_)) == EGL_TRUE;
}

void GlesContext::shutdown() noexcept {
    const auto display = static_cast<EGLDisplay>(display_);
    const auto surface = static_cast<EGLSurface>(surface_);
    const auto context = static_cast<EGLContext>(context_);

    if (display != EGL_NO_DISPLAY) {
        (void)eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        eglTerminate(display);
    }

    display_ = nullptr;
    surface_ = nullptr;
    context_ = nullptr;
    valid_ = false;
}

} // namespace storm::render

#else

namespace storm::render {
GlesContext::~GlesContext() = default;
bool GlesContext::initializePbuffer(int, int) noexcept { return false; }
bool GlesContext::initializeWindow(void*) noexcept { return false; }
void GlesContext::shutdown() noexcept {
    valid_ = false;
    display_ = nullptr;
    surface_ = nullptr;
    context_ = nullptr;
}
bool GlesContext::makeCurrent() noexcept { return false; }
bool GlesContext::swap() noexcept { return false; }
} // namespace storm::render

#endif
