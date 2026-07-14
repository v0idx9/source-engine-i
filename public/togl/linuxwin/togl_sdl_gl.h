#ifndef TOGL_SDL_GL_H
#define TOGL_SDL_GL_H

#ifdef USE_SDL
#if defined( IOS ) || defined( _IOS ) || defined( __IPHONEOS__ )
#include "SDL_opengles2.h"
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif
#else
#include "SDL_opengl.h"
#endif
#endif

#endif
