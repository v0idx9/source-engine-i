#ifndef TOGL_SDL_GL_H
#define TOGL_SDL_GL_H

#ifdef USE_SDL
#if defined( IOS ) || defined( _IOS ) || defined( __IPHONEOS__ )
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef _APIENTRY
#define _APIENTRY APIENTRY
#endif
#ifndef GLAPI
#define GLAPI extern
#endif
#ifndef GLclampf
typedef float GLclampf;
#endif
#ifndef GLclampd
typedef double GLclampd;
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER GL_CLAMP_TO_BORDER_OES
#endif
#ifndef GL_TEXTURE_COMPARE_MODE_ARB
#define GL_TEXTURE_COMPARE_MODE_ARB GL_COMPARE_REF_TO_TEXTURE
#endif
#ifndef GL_COMPARE_R_TO_TEXTURE_ARB
#define GL_COMPARE_R_TO_TEXTURE_ARB GL_COMPARE_REF_TO_TEXTURE
#endif
#ifndef GL_TEXTURE_COMPARE_FUNC_ARB
#define GL_TEXTURE_COMPARE_FUNC_ARB GL_TEXTURE_COMPARE_FUNC
#endif
#else
#include "SDL_opengl.h"
#endif
#endif

#endif
