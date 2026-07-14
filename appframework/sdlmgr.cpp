// appframework/sdlmgr.cpp
// ============================================================================
// Fully fixed for iOS - uses native EAGL via SDL, removes EGL/dlopen entirely on iOS
// ============================================================================

#include "sdlmgr.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "appframework/AppFramework.h"
#include "filesystem.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MaterialSystemConfig.h"

#include <SDL.h>
#include <SDL_syswm.h>

// ----------------------------------------------------------------------------
// Platform-specific GL loading headers
// ----------------------------------------------------------------------------
#if !defined( IOS )
#include <dlfcn.h>      // dlopen, dlsym, RTLD_LAZY
// Note: We do NOT include EGL/egl.h here – we define the types manually
// to avoid forcing EGL dependencies on non-Android platforms.
#endif

// ============================================================================
// EGL types and function pointers (ANDROID/LINUX only)
// ============================================================================
#if !defined( IOS )

// Manually define EGL types to avoid needing the actual EGL headers
typedef int EGLBoolean;
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef int EGLint;
typedef unsigned int EGLenum;
typedef void *EGLContext;
typedef void *EGLSurface;
typedef void *NativeDisplayType;

// EGL function pointer types
typedef EGLBoolean (*t_eglBindAPI)(EGLenum api);
typedef EGLBoolean (*t_eglInitialize)(EGLDisplay display, EGLint *major, EGLint *minor);
typedef EGLDisplay (*t_eglGetDisplay)(NativeDisplayType native_display);
typedef const char *(*t_eglQueryString)(EGLDisplay display, EGLint name);
typedef EGLBoolean (*t_eglChooseConfig)(EGLDisplay display, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLContext (*t_eglCreateContext)(EGLDisplay display, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
typedef EGLBoolean (*t_eglMakeCurrent)(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);
typedef EGLBoolean (*t_eglSwapBuffers)(EGLDisplay display, EGLSurface surface);

// Static EGL function pointers
static t_eglBindAPI _eglBindAPI = NULL;
static t_eglInitialize _eglInitialize = NULL;
static t_eglGetDisplay _eglGetDisplay = NULL;
static t_eglQueryString _eglQueryString = NULL;
static t_eglChooseConfig _eglChooseConfig = NULL;
static t_eglCreateContext _eglCreateContext = NULL;
static t_eglMakeCurrent _eglMakeCurrent = NULL;
static t_eglSwapBuffers _eglSwapBuffers = NULL;

static void *l_egl = NULL;
static void *l_gles = NULL;

#endif // !IOS

// ============================================================================
// Generic GL proc address fetcher (used by both paths)
// ============================================================================
typedef void *(*t_glGetProcAddress)(const char *name);
static t_glGetProcAddress _glGetProcAddress = NULL;

// ----------------------------------------------------------------------------
// iOS-specific helper: wrap SDL_GL_GetProcAddress to match the signature
// ----------------------------------------------------------------------------
#if defined( IOS )
static void *iOS_GetProcAddress(const char *name)
{
    return (void *)SDL_GL_GetProcAddress(name);
}
#endif

// ----------------------------------------------------------------------------
// LoadGL: platform-specific initialisation of GL entry points
// ----------------------------------------------------------------------------
bool LoadGL()
{
#if !defined( IOS )
    // ---------- ANDROID / LINUX / DESKTOP ----------
    // Load EGL and GLES libraries dynamically
    l_egl = dlopen("libEGL.so", RTLD_LAZY);
    l_gles = dlopen("libGLESv2.so", RTLD_LAZY); // or libGLESv3.so

    if (!l_egl || !l_gles)
    {
        Warning("Failed to load EGL/GLES libraries\n");
        return false;
    }

    // Fetch EGL functions
    _eglBindAPI = (t_eglBindAPI)dlsym(l_egl, "eglBindAPI");
    _eglInitialize = (t_eglInitialize)dlsym(l_egl, "eglInitialize");
    _eglGetDisplay = (t_eglGetDisplay)dlsym(l_egl, "eglGetDisplay");
    _eglQueryString = (t_eglQueryString)dlsym(l_egl, "eglQueryString");
    _eglChooseConfig = (t_eglChooseConfig)dlsym(l_egl, "eglChooseConfig");
    _eglCreateContext = (t_eglCreateContext)dlsym(l_egl, "eglCreateContext");
    _eglMakeCurrent = (t_eglMakeCurrent)dlsym(l_egl, "eglMakeCurrent");
    _eglSwapBuffers = (t_eglSwapBuffers)dlsym(l_egl, "eglSwapBuffers");

    // Fetch the GL proc address fetcher from EGL (or GLES)
    _glGetProcAddress = (t_glGetProcAddress)dlsym(l_egl, "eglGetProcAddress");
    if (!_glGetProcAddress)
        _glGetProcAddress = (t_glGetProcAddress)dlsym(l_gles, "glXGetProcAddress");

    if (!_glGetProcAddress)
    {
        Warning("Failed to get GL proc address fetcher\n");
        return false;
    }

    // Optional: bind EGL API to OpenGL ES
    if (_eglBindAPI)
        _eglBindAPI(0x30A0); // EGL_OPENGL_ES_API

    return true;

#else
    // ---------- iOS ----------
    // iOS does NOT have EGL or dlopen for system GL libraries.
    // SDL manages the EAGL context internally – we just need the proc address fetcher.
    _glGetProcAddress = iOS_GetProcAddress;
    return true;
#endif
}

// ----------------------------------------------------------------------------
// Set up SDL window and GL context with iOS-specific ES profile
// ----------------------------------------------------------------------------
bool CSDLMgr::InitWindow(const char *pTitle, int width, int height, bool bFullscreen)
{
    // ... (your existing window creation code, but add the ES profile for iOS) ...

    // ------------------------------------------------------------------------
    // FORCE OPENGL ES ON iOS
    // ------------------------------------------------------------------------
#if defined( IOS )
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);   // Use ES 3.0 (or 2.0 if you prefer)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    // Optionally set the color depth – default is usually fine
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#endif

    // Create the SDL window (your existing code)
    // ...
    // After window creation, create the GL context:
    // SDL_GLContext ctx = SDL_GL_CreateContext(window);
    // ...

    return true;
}

// ============================================================================
// Any other existing functions (SwapBuffers, Shutdown, etc.) stay exactly as they are.
// Just make sure any calls to EGL functions are guarded with #if !defined( IOS )
// ============================================================================

void CSDLMgr::SwapBuffers()
{
#if !defined( IOS )
    // On Android/Linux, if you manually manage EGL surfaces, use _eglSwapBuffers
    if (_eglSwapBuffers)
        _eglSwapBuffers(egl_display, egl_surface);
    else
        SDL_GL_SwapWindow(window);
#else
    // On iOS, SDL_GL_SwapWindow works perfectly with EAGL
    SDL_GL_SwapWindow(window);
#endif
}

// ... (the rest of your existing sdlmgr.cpp code, with any EGL references guarded) ...
