//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SDL Manager - full implementation
//
//===========================================================================//

#include "sdlmgr.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "appframework/AppFramework.h"
#include "filesystem.h"
#include "materialsystem/IMaterialSystem.h"
#include "inputsystem/ButtonCode.h"
#include "inputsystem/IInputSystem.h"
#include "togl/rendermechanism.h"
#include "tier0/vprof_telemetry.h"
#include "tier0/icommandline.h"
#include "tier1/utllinkedlist.h"
#include "tier1/convar.h"
#include "tier0/vprof.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_gamecontroller.h>
#include <SDL_hints.h>

#ifdef _WIN32
#include <windows.h>
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

// ----------------------------------------------------------------------------
// EGL is ONLY for Android/Linux - NOT for iOS
// ----------------------------------------------------------------------------
#if !defined( IOS )

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef EGLBoolean (EGLAPIENTRY *t_eglBindAPI)(EGLenum api);
typedef EGLBoolean (EGLAPIENTRY *t_eglInitialize)(EGLDisplay display, EGLint *major, EGLint *minor);
typedef EGLDisplay (EGLAPIENTRY *t_eglGetDisplay)(NativeDisplayType native_display);
typedef const char * (EGLAPIENTRY *t_eglQueryString)(EGLDisplay display, EGLint name);
typedef EGLBoolean (EGLAPIENTRY *t_eglChooseConfig)(EGLDisplay display, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLContext (EGLAPIENTRY *t_eglCreateContext)(EGLDisplay display, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRY *t_eglMakeCurrent)(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);
typedef EGLBoolean (EGLAPIENTRY *t_eglSwapBuffers)(EGLDisplay display, EGLSurface surface);
typedef EGLSurface (EGLAPIENTRY *t_eglCreateWindowSurface)(EGLDisplay display, EGLConfig config, NativeWindowType window, const EGLint *attrib_list);

static t_eglBindAPI _eglBindAPI = NULL;
static t_eglInitialize _eglInitialize = NULL;
static t_eglGetDisplay _eglGetDisplay = NULL;
static t_eglQueryString _eglQueryString = NULL;
static t_eglChooseConfig _eglChooseConfig = NULL;
static t_eglCreateContext _eglCreateContext = NULL;
static t_eglMakeCurrent _eglMakeCurrent = NULL;
static t_eglSwapBuffers _eglSwapBuffers = NULL;
static t_eglCreateWindowSurface _eglCreateWindowSurface = NULL;

static void *l_egl = NULL;
static void *l_gles = NULL;

#include <dlfcn.h>

#endif // !IOS

// ----------------------------------------------------------------------------
// GL proc address fetcher (works on all platforms)
// ----------------------------------------------------------------------------
typedef void *(*t_glGetProcAddress)(const char *name);
static t_glGetProcAddress _glGetProcAddress = NULL;

// ----------------------------------------------------------------------------
// iOS helper - use SDL's function
// ----------------------------------------------------------------------------
#if defined( IOS )
static void *iOS_GetProcAddress(const char *name)
{
    return (void *)SDL_GL_GetProcAddress(name);
}
#endif

// ----------------------------------------------------------------------------
// LoadGL - platform specific
// ----------------------------------------------------------------------------
bool LoadGL()
{
#if !defined( IOS )
    l_egl = dlopen("libEGL.so", RTLD_LAZY);
    l_gles = dlopen("libGLESv2.so", RTLD_LAZY);

    if (!l_egl || !l_gles)
    {
        Warning("Failed to load EGL/GLES libraries\n");
        return false;
    }

    _eglBindAPI = (t_eglBindAPI)dlsym(l_egl, "eglBindAPI");
    _eglInitialize = (t_eglInitialize)dlsym(l_egl, "eglInitialize");
    _eglGetDisplay = (t_eglGetDisplay)dlsym(l_egl, "eglGetDisplay");
    _eglQueryString = (t_eglQueryString)dlsym(l_egl, "eglQueryString");
    _eglChooseConfig = (t_eglChooseConfig)dlsym(l_egl, "eglChooseConfig");
    _eglCreateContext = (t_eglCreateContext)dlsym(l_egl, "eglCreateContext");
    _eglMakeCurrent = (t_eglMakeCurrent)dlsym(l_egl, "eglMakeCurrent");
    _eglSwapBuffers = (t_eglSwapBuffers)dlsym(l_egl, "eglSwapBuffers");
    _eglCreateWindowSurface = (t_eglCreateWindowSurface)dlsym(l_egl, "eglCreateWindowSurface");

    _glGetProcAddress = (t_glGetProcAddress)dlsym(l_egl, "eglGetProcAddress");
    if (!_glGetProcAddress)
        _glGetProcAddress = (t_glGetProcAddress)dlsym(l_gles, "glXGetProcAddress");

    if (!_glGetProcAddress)
    {
        Warning("Failed to get GL proc address fetcher\n");
        return false;
    }

    if (_eglBindAPI)
        _eglBindAPI(EGL_OPENGL_ES_API);

    return true;
#else
    // iOS: no EGL, use SDL's native EAGL context
    _glGetProcAddress = iOS_GetProcAddress;
    return true;
#endif
}

// ============================================================================
// CSDLMgr implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------
static CSDLMgr g_SDLMgr;
ILauncherMgr *GetLauncherMgr()
{
    return &g_SDLMgr;
}

// ----------------------------------------------------------------------------
// Constructor / Destructor
// ----------------------------------------------------------------------------
CSDLMgr::CSDLMgr()
    : m_pWindow(NULL),
      m_GLContext(NULL),
      m_pController(NULL),
      m_nControllerID(-1),
      m_bInitialized(false),
      m_bFullscreen(false),
      m_bVSync(true),
      m_bMouseVisible(true),
      m_nWindowWidth(1024),
      m_nWindowHeight(768),
      m_nWindowX(SDL_WINDOWPOS_UNDEFINED),
      m_nWindowY(SDL_WINDOWPOS_UNDEFINED),
      m_nMouseX(0),
      m_nMouseY(0),
      m_nMouseDeltaX(0),
      m_nMouseDeltaY(0),
      m_bMouseFocus(false),
      m_bKeyboardFocus(false),
      m_bMinimized(false),
      m_bIsActive(false),
      m_nModifiers(0),
      m_bControllerConnected(false),
      m_nRefCount(0),
      m_bForbidMouseGrab(false),
      m_flPrevSwapTime(0.0),
      m_nRenderedWidth(0),
      m_nRenderedHeight(0),
      m_nDisplayedWidth(0),
      m_nDisplayedHeight(0)
{
    memset(m_bKeyState, 0, sizeof(m_bKeyState));
    memset(m_bKeyStatePrev, 0, sizeof(m_bKeyStatePrev));
    memset(m_fControllerAxes, 0, sizeof(m_fControllerAxes));
    memset(m_bControllerButtons, 0, sizeof(m_bControllerButtons));
}

CSDLMgr::~CSDLMgr()
{
    DestroyMainWindow();
}

// ----------------------------------------------------------------------------
// InitSDL / ShutdownSDL
// ----------------------------------------------------------------------------
bool CSDLMgr::InitSDL()
{
    if (m_bInitialized)
        return true;

    Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
#ifdef USE_SDL_GAMECONTROLLER
    flags |= SDL_INIT_GAMECONTROLLER;
#endif

    if (SDL_Init(flags) < 0)
    {
        Warning("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");
#ifdef SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE
    SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0");
#endif

    m_bInitialized = true;
    return true;
}

void CSDLMgr::ShutdownSDL()
{
    if (!m_bInitialized)
        return;

    if (m_pController)
    {
        SDL_GameControllerClose(m_pController);
        m_pController = NULL;
    }

    if (m_GLContext)
    {
        SDL_GL_DeleteContext(m_GLContext);
        m_GLContext = NULL;
    }

    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = NULL;
    }

    SDL_Quit();
    m_bInitialized = false;
}

// ----------------------------------------------------------------------------
// CreateMainWindow
// ----------------------------------------------------------------------------
bool CSDLMgr::CreateMainWindow()
{
    if (m_pWindow)
        return true;

    if (!InitSDL())
        return false;

    if (CommandLine()->CheckParm("-width") && CommandLine()->CheckParm("-height"))
    {
        m_nWindowWidth = atoi(CommandLine()->ParmValue("-width"));
        m_nWindowHeight = atoi(CommandLine()->ParmValue("-height"));
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#if defined( IOS )
    // iOS: force OpenGL ES 3.0 via SDL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined( ANDROID )
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

    if (!LoadGL())
    {
        Error("Failed to load OpenGL\n");
        return false;
    }

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

    if (CommandLine()->FindParm("-fullscreen") || CommandLine()->FindParm("-full"))
    {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        m_bFullscreen = true;
    }

    m_pWindow = SDL_CreateWindow("Source Engine",
                                 m_nWindowX,
                                 m_nWindowY,
                                 m_nWindowWidth,
                                 m_nWindowHeight,
                                 flags);

    if (!m_pWindow)
    {
        Error("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GetWindowSize(m_pWindow, &m_nWindowWidth, &m_nWindowHeight);
    SDL_GetWindowPosition(m_pWindow, &m_nWindowX, &m_nWindowY);

    m_GLContext = SDL_GL_CreateContext(m_pWindow);
    if (!m_GLContext)
    {
        Error("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    m_bVSync = true;
    SDL_GL_SetSwapInterval(m_bVSync ? 1 : 0);

    if (CommandLine()->CheckParm("-title"))
    {
        SetWindowTitle(CommandLine()->ParmValue("-title"));
    }

#ifdef USE_SDL_GAMECONTROLLER
    if (SDL_NumJoysticks() > 0)
    {
        m_pController = SDL_GameControllerOpen(0);
        if (m_pController)
        {
            m_nControllerID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_pController));
            m_bControllerConnected = true;
        }
    }
#endif

    m_bIsActive = true;
    return true;
}

// ----------------------------------------------------------------------------
// DestroyMainWindow
// ----------------------------------------------------------------------------
void CSDLMgr::DestroyMainWindow()
{
    ShutdownSDL();
}

// ----------------------------------------------------------------------------
// SwapBuffers
// ----------------------------------------------------------------------------
void CSDLMgr::SwapBuffers()
{
    if (m_pWindow)
    {
        SDL_GL_SwapWindow(m_pWindow);
        m_flPrevSwapTime = SDL_GetTicks() / 1000.0;
    }
}

// ----------------------------------------------------------------------------
// SetWindowTitle
// ----------------------------------------------------------------------------
void CSDLMgr::SetWindowTitle(const char *pTitle)
{
    if (m_pWindow)
    {
        SDL_SetWindowTitle(m_pWindow, pTitle);
    }
}

// ----------------------------------------------------------------------------
// GetWindowSize / SetWindowSize
// ----------------------------------------------------------------------------
void CSDLMgr::GetWindowSize(int &width, int &height)
{
    if (m_pWindow)
    {
        SDL_GetWindowSize(m_pWindow, &width, &height);
    }
    else
    {
        width = m_nWindowWidth;
        height = m_nWindowHeight;
    }
}

void CSDLMgr::SetWindowSize(int width, int height)
{
    if (m_pWindow)
    {
        SDL_SetWindowSize(m_pWindow, width, height);
        m_nWindowWidth = width;
        m_nWindowHeight = height;
    }
}

// ----------------------------------------------------------------------------
// GetWindow / GetGLContext
// ----------------------------------------------------------------------------
void *CSDLMgr::GetWindow()
{
    return m_pWindow;
}

void *CSDLMgr::GetGLContext()
{
    return m_GLContext;
}

// ----------------------------------------------------------------------------
// IsWindowVisible
// ----------------------------------------------------------------------------
bool CSDLMgr::IsWindowVisible()
{
    if (!m_pWindow)
        return false;
    return (SDL_GetWindowFlags(m_pWindow) & SDL_WINDOW_SHOWN) != 0;
}

// ----------------------------------------------------------------------------
// PumpWindowsMessageLoop
// ----------------------------------------------------------------------------
void CSDLMgr::PumpWindowsMessageLoop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        HandleSDLEvent(event);
    }

    memcpy(m_bKeyStatePrev, m_bKeyState, sizeof(m_bKeyStatePrev));
    m_nMouseDeltaX = 0;
    m_nMouseDeltaY = 0;
}

// ----------------------------------------------------------------------------
// PumpAndPeekMessages
// ----------------------------------------------------------------------------
void CSDLMgr::PumpAndPeekMessages()
{
    PumpWindowsMessageLoop();
}

// ----------------------------------------------------------------------------
// GetMouseDelta (both overloads)
// ----------------------------------------------------------------------------
void CSDLMgr::GetMouseDelta(int &x, int &y)
{
    x = m_nMouseDeltaX;
    y = m_nMouseDeltaY;
    m_nMouseDeltaX = 0;
    m_nMouseDeltaY = 0;
}

void CSDLMgr::GetMouseDelta( int &x, int &y, bool bIgnoreNextMouseDelta )
{
    GetMouseDelta(x, y);
    if (bIgnoreNextMouseDelta)
    {
        m_nMouseDeltaX = 0;
        m_nMouseDeltaY = 0;
    }
}

// ----------------------------------------------------------------------------
// SetMouseVisible
// ----------------------------------------------------------------------------
void CSDLMgr::SetMouseVisible(bool bVisible)
{
    m_bMouseVisible = bVisible;
    if (m_pWindow)
    {
        SDL_ShowCursor(bVisible ? SDL_ENABLE : SDL_DISABLE);
    }
}

// ----------------------------------------------------------------------------
// SetMouseFocus
// ----------------------------------------------------------------------------
void CSDLMgr::SetMouseFocus(bool bFocus)
{
    // Not implemented - handled by events
}

// ----------------------------------------------------------------------------
// SetCursorPos / GetCursorPos
// ----------------------------------------------------------------------------
void CSDLMgr::SetCursorPos(int x, int y)
{
    if (m_pWindow)
    {
        SDL_WarpMouseInWindow(m_pWindow, x, y);
        m_nMouseX = x;
        m_nMouseY = y;
    }
}

void CSDLMgr::GetCursorPos(int &x, int &y)
{
    if (m_pWindow)
    {
        SDL_GetMouseState(&x, &y);
        m_nMouseX = x;
        m_nMouseY = y;
    }
    else
    {
        x = m_nMouseX;
        y = m_nMouseY;
    }
}

// ----------------------------------------------------------------------------
// IsCursorVisible
// ----------------------------------------------------------------------------
bool CSDLMgr::IsCursorVisible()
{
    return m_bMouseVisible;
}

// ----------------------------------------------------------------------------
// SetWindowPosition / GetWindowPosition
// ----------------------------------------------------------------------------
void CSDLMgr::SetWindowPosition(int x, int y)
{
    if (m_pWindow)
    {
        SDL_SetWindowPosition(m_pWindow, x, y);
        m_nWindowX = x;
        m_nWindowY = y;
    }
}

void CSDLMgr::GetWindowPosition(int &x, int &y)
{
    if (m_pWindow)
    {
        SDL_GetWindowPosition(m_pWindow, &x, &y);
        m_nWindowX = x;
        m_nWindowY = y;
    }
    else
    {
        x = m_nWindowX;
        y = m_nWindowY;
    }
}

// ----------------------------------------------------------------------------
// SetFullscreen / IsFullscreen
// ----------------------------------------------------------------------------
void CSDLMgr::SetFullscreen(bool bFullscreen)
{
    if (m_pWindow && m_bFullscreen != bFullscreen)
    {
        m_bFullscreen = bFullscreen;
        Uint32 flags = bFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        SDL_SetWindowFullscreen(m_pWindow, flags);
    }
}

bool CSDLMgr::IsFullscreen()
{
    return m_bFullscreen;
}

// ----------------------------------------------------------------------------
// SetVSyncEnabled / IsVSyncEnabled
// ----------------------------------------------------------------------------
void CSDLMgr::SetVSyncEnabled(bool bEnabled)
{
    m_bVSync = bEnabled;
    if (m_pWindow)
    {
        SDL_GL_SetSwapInterval(bEnabled ? 1 : 0);
    }
}

bool CSDLMgr::IsVSyncEnabled()
{
    return m_bVSync;
}

// ----------------------------------------------------------------------------
// GetProcAddress
// ----------------------------------------------------------------------------
void *CSDLMgr::GetProcAddress(const char *name)
{
    if (_glGetProcAddress)
        return _glGetProcAddress(name);
    return NULL;
}

// ----------------------------------------------------------------------------
// GetDisplayWidth / GetDisplayHeight
// ----------------------------------------------------------------------------
int CSDLMgr::GetDisplayWidth()
{
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0)
        return mode.w;
    return 1024;
}

int CSDLMgr::GetDisplayHeight()
{
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0)
        return mode.h;
    return 768;
}

// ----------------------------------------------------------------------------
// Additional ILauncherMgr stubs
// ----------------------------------------------------------------------------
bool CSDLMgr::CreateGameWindow( const char *pTitle, bool bWindowed, int width, int height )
{
    m_nWindowWidth = width;
    m_nWindowHeight = height;
    return CreateMainWindow();
}

void CSDLMgr::IncWindowRefCount()
{
    m_nRefCount++;
}

void CSDLMgr::DecWindowRefCount()
{
    if (m_nRefCount > 0)
        m_nRefCount--;
    if (m_nRefCount == 0)
        DestroyMainWindow();
}

int CSDLMgr::GetEvents( CCocoaEvent *pEvents, int nMaxEventsToReturn, bool debugEvents )
{
    return 0;
}

int CSDLMgr::PeekAndRemoveKeyboardEvents( bool *pbEsc, bool *pbReturn, bool *pbSpace, bool debugEvents )
{
    return 0;
}

void CSDLMgr::SetCursorPosition( int x, int y )
{
    SetCursorPos(x, y);
}

void CSDLMgr::SetWindowFullScreen( bool bFullScreen, int nWidth, int nHeight )
{
    SetFullscreen(bFullScreen);
}

bool CSDLMgr::IsWindowFullScreen()
{
    return IsFullscreen();
}

void CSDLMgr::MoveWindow( int x, int y )
{
    SetWindowPosition(x, y);
}

void CSDLMgr::SizeWindow( int width, int tall )
{
    SetWindowSize(width, tall);
}

void CSDLMgr::DestroyGameWindow()
{
    DestroyMainWindow();
}

void CSDLMgr::SetApplicationIcon( const char *pchAppIconFile )
{
    // stub
}

void CSDLMgr::GetNativeDisplayInfo( int nDisplay, uint &nWidth, uint &nHeight, uint &nRefreshHz )
{
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(nDisplay, &mode) == 0)
    {
        nWidth = mode.w;
        nHeight = mode.h;
        nRefreshHz = mode.refresh_rate;
    }
    else
    {
        nWidth = 1024;
        nHeight = 768;
        nRefreshHz = 60;
    }
}

void CSDLMgr::RenderedSize( uint &width, uint &height, bool set )
{
    if (set)
    {
        m_nRenderedWidth = width;
        m_nRenderedHeight = height;
    }
    else
    {
        width = m_nRenderedWidth;
        height = m_nRenderedHeight;
    }
}

void CSDLMgr::DisplayedSize( uint &width, uint &height )
{
    if (m_pWindow)
    {
        int w, h;
        SDL_GetWindowSize(m_pWindow, &w, &h);
        m_nDisplayedWidth = w;
        m_nDisplayedHeight = h;
    }
    width = m_nDisplayedWidth;
    height = m_nDisplayedHeight;
}

PseudoGLContextPtr CSDLMgr::GetMainContext()
{
    return (PseudoGLContextPtr)m_GLContext;
}

PseudoGLContextPtr CSDLMgr::GetGLContextForWindow( void* windowref )
{
    return (PseudoGLContextPtr)m_GLContext;
}

PseudoGLContextPtr CSDLMgr::CreateExtraContext()
{
    return (PseudoGLContextPtr)m_GLContext;
}

void CSDLMgr::DeleteContext( PseudoGLContextPtr hContext )
{
}

bool CSDLMgr::MakeContextCurrent( PseudoGLContextPtr hContext )
{
    if (hContext == (PseudoGLContextPtr)m_GLContext)
        return true;
    return false;
}

GLMDisplayDB *CSDLMgr::GetDisplayDB( void )
{
    return NULL;
}

void CSDLMgr::GetDesiredPixelFormatAttribsAndRendererInfo( uint **ptrOut, uint *countOut, GLMRendererInfoFields *rendInfoOut )
{
    *ptrOut = NULL;
    *countOut = 0;
}

void CSDLMgr::ShowPixels( CShowPixelsParams *params )
{
}

void CSDLMgr::GetStackCrawl( CStackCrawlParams *params )
{
}

void CSDLMgr::WaitUntilUserInput( int msSleepTime )
{
    SDL_Delay(msSleepTime);
}

void *CSDLMgr::GetWindowRef()
{
    return GetWindow();
}

void CSDLMgr::SetMouseCursor( SDL_Cursor *hCursor )
{
    SDL_SetCursor(hCursor);
}

void CSDLMgr::SetForbidMouseGrab( bool bForbidMouseGrab )
{
    m_bForbidMouseGrab = bForbidMouseGrab;
}

void CSDLMgr::OnFrameRendered()
{
}

void CSDLMgr::SetGammaRamp( const uint16 *pRed, const uint16 *pGreen, const uint16 *pBlue )
{
}

double CSDLMgr::GetPrevGLSwapWindowTime()
{
    return m_flPrevSwapTime;
}

// ----------------------------------------------------------------------------
// IAppSystem implementation
// ----------------------------------------------------------------------------
bool CSDLMgr::Connect( CreateInterfaceFn factory )
{
    return true;
}

void CSDLMgr::Disconnect()
{
}

void *CSDLMgr::QueryInterface( const char *pInterfaceName )
{
    return NULL;
}

InitReturnVal_t CSDLMgr::Init()
{
    if ( !m_bInitialized )
    {
        if ( !InitSDL() )
            return INIT_FAILED;
        if ( !CreateMainWindow() )
            return INIT_FAILED;
    }
    return INIT_OK;
}

void CSDLMgr::Shutdown()
{
    DestroyMainWindow();
    ShutdownSDL();
}

// ----------------------------------------------------------------------------
// HandleSDLEvent
// ----------------------------------------------------------------------------
void CSDLMgr::HandleSDLEvent(SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        break;

    case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
            m_nWindowWidth = event.window.data1;
            m_nWindowHeight = event.window.data2;
            break;
        case SDL_WINDOWEVENT_MOVED:
            m_nWindowX = event.window.data1;
            m_nWindowY = event.window.data2;
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            m_bKeyboardFocus = true;
            m_bIsActive = true;
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            m_bKeyboardFocus = false;
            m_bIsActive = false;
            break;
        case SDL_WINDOWEVENT_ENTER:
            m_bMouseFocus = true;
            break;
        case SDL_WINDOWEVENT_LEAVE:
            m_bMouseFocus = false;
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            m_bMinimized = true;
            break;
        case SDL_WINDOWEVENT_RESTORED:
            m_bMinimized = false;
            break;
        case SDL_WINDOWEVENT_CLOSE:
            break;
        }
        break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        SDL_Keysym sym = event.key.keysym;
        m_bKeyState[sym.scancode] = (event.type == SDL_KEYDOWN);
        UpdateModifierKeys();
        break;
    }

    case SDL_MOUSEMOTION:
        m_nMouseX = event.motion.x;
        m_nMouseY = event.motion.y;
        m_nMouseDeltaX += event.motion.xrel;
        m_nMouseDeltaY += event.motion.yrel;
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        break;

    case SDL_MOUSEWHEEL:
        break;

    case SDL_CONTROLLERDEVICEADDED:
#ifdef USE_SDL_GAMECONTROLLER
        if (!m_pController && event.cdevice.which == 0)
        {
            m_pController = SDL_GameControllerOpen(event.cdevice.which);
            if (m_pController)
            {
                m_nControllerID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_pController));
                m_bControllerConnected = true;
            }
        }
#endif
        break;

    case SDL_CONTROLLERDEVICEREMOVED:
#ifdef USE_SDL_GAMECONTROLLER
        if (m_pController && event.cdevice.which == m_nControllerID)
        {
            SDL_GameControllerClose(m_pController);
            m_pController = NULL;
            m_bControllerConnected = false;
            m_nControllerID = -1;
        }
#endif
        break;

    case SDL_CONTROLLERAXISMOTION:
#ifdef USE_SDL_GAMECONTROLLER
        if (m_pController && event.caxis.which == m_nControllerID)
        {
            int axis = event.caxis.axis;
            if (axis >= 0 && axis < SDL_CONTROLLER_AXIS_MAX)
            {
                m_fControllerAxes[axis] = event.caxis.value / 32767.0f;
            }
        }
#endif
        break;

    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
#ifdef USE_SDL_GAMECONTROLLER
        if (m_pController && event.cbutton.which == m_nControllerID)
        {
            int button = event.cbutton.button;
            if (button >= 0 && button < SDL_CONTROLLER_BUTTON_MAX)
            {
                m_bControllerButtons[button] = (event.type == SDL_CONTROLLERBUTTONDOWN);
            }
        }
#endif
        break;

    default:
        break;
    }
}

// ----------------------------------------------------------------------------
// UpdateModifierKeys
// ----------------------------------------------------------------------------
void CSDLMgr::UpdateModifierKeys()
{
    m_nModifiers = 0;
    if (m_bKeyState[SDL_SCANCODE_LSHIFT] || m_bKeyState[SDL_SCANCODE_RSHIFT])
        m_nModifiers |= KMOD_SHIFT;
    if (m_bKeyState[SDL_SCANCODE_LCTRL] || m_bKeyState[SDL_SCANCODE_RCTRL])
        m_nModifiers |= KMOD_CTRL;
    if (m_bKeyState[SDL_SCANCODE_LALT] || m_bKeyState[SDL_SCANCODE_RALT])
        m_nModifiers |= KMOD_ALT;
    if (m_bKeyState[SDL_SCANCODE_LGUI] || m_bKeyState[SDL_SCANCODE_RGUI])
        m_nModifiers |= KMOD_GUI;
}
