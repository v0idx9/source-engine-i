//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Misc networking utilities.
//
//			Postdates the Steamworks SDK bundled here. TF2's GC client calls
//			InitRelayNetworkAccess() to warm up the Steam Datagram Relay. There
//			is no Steam client on iOS, so SteamNetworkingUtils() returns NULL
//			from stub_steam and the caller's own null check skips it.
//
//=============================================================================

#ifndef ISTEAMNETWORKINGUTILS_H
#define ISTEAMNETWORKINGUTILS_H
#ifdef _WIN32
#pragma once
#endif

#include "isteamclient.h"

//-----------------------------------------------------------------------------
// Purpose: Miscellaneous networking utilities
//-----------------------------------------------------------------------------
class ISteamNetworkingUtils
{
public:
	// Begin fetching the network configuration needed to talk over the relay
	// network. Safe to call repeatedly.
	virtual void InitRelayNetworkAccess() = 0;
};

#define STEAMNETWORKINGUTILS_INTERFACE_VERSION "SteamNetworkingUtils003"

// Declared unconditionally: VERSION_SAFE_STEAM_API_INTERFACES hides the other
// global accessors and there is no CSteamAPIContext member for this one.
S_API ISteamNetworkingUtils *SteamNetworkingUtils();

#endif // ISTEAMNETWORKINGUTILS_H
