//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: native screen geometry for iOS.
//
// This lives in its own translation unit on purpose: UIKit and the Source
// headers cannot be combined. Source's basetypes.h does "typedef int BOOL",
// which collides with objc.h's BOOL, and Source's MIN/MAX macros break
// MacTypes.h if UIKit is imported after them. So this file imports UIKit and
// nothing else.
//
//=============================================================================//

#import <UIKit/UIKit.h>

// Native screen size in PIXELS (not points).
//
// SDL reports both the desktop display mode and the GL drawable in points on
// iOS, but the EGL/Metal surface the engine renders into is the full native
// drawable. Sizing the engine from points makes it render into a corner of a
// much larger surface (~1/3 on a 3x device).
//
// UIScreen's nativeBounds is physical pixels and is always expressed
// portrait-side-up regardless of the current orientation; the game runs
// landscape, so return the long edge as the width.
extern "C" void IOS_GetScreenPixelSize( int *pWidth, int *pHeight )
{
	CGRect bounds = [[UIScreen mainScreen] nativeBounds];

	int w = (int)bounds.size.width;
	int h = (int)bounds.size.height;

	if ( w < h )
	{
		int t = w;
		w = h;
		h = t;
	}

	if ( pWidth )
	{
		*pWidth = w;
	}

	if ( pHeight )
	{
		*pHeight = h;
	}
}
