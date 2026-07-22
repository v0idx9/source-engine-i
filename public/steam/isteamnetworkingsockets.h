//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Networking sockets interface.
//
//			Postdates the Steamworks SDK bundled here. TF2's GC client
//			includes it but does not reference anything it declares -- the
//			relay warm-up it actually performs goes through
//			ISteamNetworkingUtils. Rather than reconstruct an interface that
//			nothing calls, this is a placeholder so the include resolves; if a
//			caller ever needs a symbol from it, that will show up as an
//			undeclared identifier rather than silently wrong behaviour.
//
//=============================================================================

#ifndef ISTEAMNETWORKINGSOCKETS_H
#define ISTEAMNETWORKINGSOCKETS_H
#ifdef _WIN32
#pragma once
#endif

#include "isteamnetworkingutils.h"

#endif // ISTEAMNETWORKINGSOCKETS_H
