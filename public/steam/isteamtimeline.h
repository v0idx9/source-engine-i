//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: public interface to the Steam Timeline
//
//			Added in a later Steamworks SDK than the one bundled here. TF2's
//			client mode clears the timeline state description on disconnect, so
//			the interface has to exist to compile. There is no Steam client on
//			iOS, so SteamTimeline() returns NULL from stub_steam and the calls
//			are skipped at runtime by the caller's own null check.
//
//=============================================================================

#ifndef ISTEAMTIMELINE_H
#define ISTEAMTIMELINE_H
#ifdef _WIN32
#pragma once
#endif

#include "isteamclient.h"

//-----------------------------------------------------------------------------
// Purpose: The state of the game, shown on the Steam Timeline
//-----------------------------------------------------------------------------
enum ETimelineGameMode
{
	k_ETimelineGameMode_Invalid = 0,
	k_ETimelineGameMode_Playing = 1,
	k_ETimelineGameMode_Staging = 2,
	k_ETimelineGameMode_Menus = 3,
	k_ETimelineGameMode_LoadingScreen = 4,

	k_ETimelineGameMode_Max,
};

//-----------------------------------------------------------------------------
// Purpose: Interface to the Steam Timeline
//-----------------------------------------------------------------------------
class ISteamTimeline
{
public:
	// Sets a description for the current game state, shown on the timeline bar.
	virtual void SetTimelineStateDescription( const char *pchDescription, float flTimeDelta ) = 0;
	virtual void ClearTimelineStateDescription( float flTimeDelta ) = 0;

	// Marks the current game mode, which colours the timeline bar.
	virtual void SetTimelineGameMode( ETimelineGameMode eMode ) = 0;
};

#define STEAMTIMELINE_INTERFACE_VERSION "STEAMTIMELINE_INTERFACE_V001"

// Declared unconditionally: VERSION_SAFE_STEAM_API_INTERFACES hides the other
// global accessors, and this one has no CSteamAPIContext member to reach it by.
S_API ISteamTimeline *SteamTimeline();

#endif // ISTEAMTIMELINE_H
