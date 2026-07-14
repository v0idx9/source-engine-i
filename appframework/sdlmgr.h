//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SDL Manager - header
//
//===========================================================================//

#ifndef SDLMGR_H
#define SDLMGR_H

#include "appframework/ilaunchermgr.h"
#include "togl/rendermechanism.h"  // for PseudoGLContextPtr, GLMDisplayDB, etc.
#include <SDL.h>

// ----------------------------------------------------------------------------
// CSDLMgr - full implementation of ILauncherMgr using SDL
// ----------------------------------------------------------------------------
class CSDLMgr : public ILauncherMgr
{
public:
    CSDLMgr();
    virtual ~CSDLMgr();

    // ---- ILauncherMgr interface ----
    virtual bool CreateMainWindow();
    virtual void DestroyMainWindow();
    virtual void SwapBuffers();
    virtual void SetWindowTitle(const char *pTitle);
    virtual void GetWindowSize(int &width, int &height);
    virtual void *GetWindow();
    virtual void *GetGLContext();
    virtual bool IsWindowVisible();
    virtual void PumpWindowsMessageLoop();
    virtual void PumpAndPeekMessages();
    virtual void GetMouseDelta(int &x, int &y);
    virtual void SetMouseVisible(bool bVisible);
    virtual void SetMouseFocus(bool bFocus);
    virtual void SetCursorPos(int x, int y);
    virtual void GetCursorPos(int &x, int &y);
    virtual bool IsCursorVisible();
    virtual void SetWindowPosition(int x, int y);
    virtual void GetWindowPosition(int &x, int &y);
    virtual void SetWindowSize(int width, int height);
    virtual void SetFullscreen(bool bFullscreen);
    virtual bool IsFullscreen();
    virtual void SetVSyncEnabled(bool bEnabled);
    virtual bool IsVSyncEnabled();
    virtual void *GetProcAddress(const char *name);
    virtual int GetDisplayWidth();
    virtual int GetDisplayHeight();

    // ---- Additional pure virtual functions from ILauncherMgr ----
    virtual bool CreateGameWindow( const char *pTitle, bool bWindowed, int width, int height );
    virtual void IncWindowRefCount();
    virtual void DecWindowRefCount();
    virtual int GetEvents( CCocoaEvent *pEvents, int nMaxEventsToReturn, bool debugEvents = false );
    virtual int PeekAndRemoveKeyboardEvents( bool *pbEsc, bool *pbReturn, bool *pbSpace, bool debugEvents = false );
    virtual void SetCursorPosition( int x, int y );          // alias for SetCursorPos
    virtual void SetWindowFullScreen( bool bFullScreen, int nWidth, int nHeight );
    virtual bool IsWindowFullScreen();
    virtual void MoveWindow( int x, int y );
    virtual void SizeWindow( int width, int tall );
    virtual void DestroyGameWindow();
    virtual void SetApplicationIcon( const char *pchAppIconFile );
    virtual void GetMouseDelta( int &x, int &y, bool bIgnoreNextMouseDelta );
    virtual void GetNativeDisplayInfo( int nDisplay, uint &nWidth, uint &nHeight, uint &nRefreshHz );
    virtual void RenderedSize( uint &width, uint &height, bool set );
    virtual void DisplayedSize( uint &width, uint &height );
    virtual PseudoGLContextPtr GetMainContext();
    virtual PseudoGLContextPtr GetGLContextForWindow( void* windowref );
    virtual PseudoGLContextPtr CreateExtraContext();
    virtual void DeleteContext( PseudoGLContextPtr hContext );
    virtual bool MakeContextCurrent( PseudoGLContextPtr hContext );
    virtual GLMDisplayDB *GetDisplayDB( void );
    virtual void GetDesiredPixelFormatAttribsAndRendererInfo( uint **ptrOut, uint *countOut, GLMRendererInfoFields *rendInfoOut );
    virtual void ShowPixels( CShowPixelsParams *params );
    virtual void GetStackCrawl( CStackCrawlParams *params );
    virtual void WaitUntilUserInput( int msSleepTime );
    virtual void *GetWindowRef();                            // alias for GetWindow
    virtual void SetMouseCursor( SDL_Cursor *hCursor );
    virtual void SetForbidMouseGrab( bool bForbidMouseGrab );
    virtual void OnFrameRendered();
    virtual void SetGammaRamp( const uint16 *pRed, const uint16 *pGreen, const uint16 *pBlue );
    virtual double GetPrevGLSwapWindowTime();

private:
    bool InitSDL();
    void ShutdownSDL();
    void HandleSDLEvent(SDL_Event &event);
    void UpdateModifierKeys();

    // ---- Member variables ----
    SDL_Window *m_pWindow;
    SDL_GLContext m_GLContext;
    SDL_GameController *m_pController;
    int m_nControllerID;
    bool m_bInitialized;
    bool m_bFullscreen;
    bool m_bVSync;
    bool m_bMouseVisible;
    int m_nWindowWidth;
    int m_nWindowHeight;
    int m_nWindowX;
    int m_nWindowY;
    int m_nMouseX;
    int m_nMouseY;
    int m_nMouseDeltaX;
    int m_nMouseDeltaY;
    bool m_bMouseFocus;
    bool m_bKeyboardFocus;
    bool m_bMinimized;
    bool m_bIsActive;

    bool m_bKeyState[SDL_NUM_SCANCODES];
    bool m_bKeyStatePrev[SDL_NUM_SCANCODES];
    int m_nModifiers;

    bool m_bControllerConnected;
    float m_fControllerAxes[SDL_CONTROLLER_AXIS_MAX];
    bool m_bControllerButtons[SDL_CONTROLLER_BUTTON_MAX];

    // ---- Additional state for stubs ----
    int m_nRefCount;
    bool m_bForbidMouseGrab;
    double m_flPrevSwapTime;
    uint m_nRenderedWidth, m_nRenderedHeight;
    uint m_nDisplayedWidth, m_nDisplayedHeight;
};

// ----------------------------------------------------------------------------
// Singleton accessor
// ----------------------------------------------------------------------------
ILauncherMgr *GetLauncherMgr();

#endif // SDLMGR_H
