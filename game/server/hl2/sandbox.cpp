//========= Sandbox mode for the iOS port ================================//
//
// Purpose: Prop spawning and cleanup, the foundation of sandbox play.
//
//			The spawn path follows the engine's own ent_create (see
//			CC_Ent_Create in baseentity.cpp): bracket the creation with
//			SetAllowPrecache( true ) so a model that was not precached at map
//			load can still be brought in at runtime, then trace from the
//			player's eye to decide where the prop lands.
//
//			Everything spawned here is tracked so sbox_cleanup can remove it
//			without disturbing entities that belong to the map.
//
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "props.h"
#include "datacache/imdlcache.h"
#include "vphysics_interface.h"
#include "weapon_physcannon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar sbox_maxprops( "sbox_maxprops", "64", FCVAR_GAMEDLL,
	"Maximum number of props sandbox spawning will keep alive at once. The oldest is removed to make room." );


//-----------------------------------------------------------------------------
// Everything sandbox spawning has created, oldest first. Handles rather than
// raw pointers so entities removed by other means (killed, map change) simply
// go invalid instead of leaving us with danglers.
//-----------------------------------------------------------------------------
static CUtlVector< EHANDLE > g_SandboxProps;


//-----------------------------------------------------------------------------
// Purpose: Drop dead handles, then trim to the prop budget by removing oldest
//			first. Returns the number of live props left.
//-----------------------------------------------------------------------------
static int SandboxPruneProps( int nBudget )
{
	// Compact away anything that died on its own.
	for ( int i = g_SandboxProps.Count() - 1; i >= 0; --i )
	{
		if ( g_SandboxProps[i].Get() == NULL )
		{
			g_SandboxProps.Remove( i );
		}
	}

	// Then make room, oldest first.
	while ( g_SandboxProps.Count() > nBudget && g_SandboxProps.Count() > 0 )
	{
		CBaseEntity *pOldest = g_SandboxProps[0].Get();
		if ( pOldest )
		{
			UTIL_Remove( pOldest );
		}

		g_SandboxProps.Remove( 0 );
	}

	return g_SandboxProps.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Spawn a physics prop where the player is looking.
//			Usage: sbox_spawn <model path>
//-----------------------------------------------------------------------------
static void CC_Sandbox_Spawn( const CCommand &args )
{
	MDLCACHE_CRITICAL_SECTION();

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: sbox_spawn <model path>\n" );
		return;
	}

	const char *pszModel = args[1];

	// Trace first: if there is nowhere to put the prop, do not create one at all
	// rather than leaving an orphan at the world origin.
	trace_t tr;
	Vector vecForward;
	pPlayer->EyeVectors( &vecForward );
	UTIL_TraceLine( pPlayer->EyePosition(),
		pPlayer->EyePosition() + vecForward * MAX_TRACE_LENGTH, MASK_SOLID,
		pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction == 1.0f || tr.allsolid )
	{
		Msg( "sbox_spawn: nothing in front of you to place a prop on.\n" );
		return;
	}

	// Make room before spawning so the new prop is never the one evicted.
	SandboxPruneProps( MAX( 0, sbox_maxprops.GetInt() - 1 ) );

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	CBaseEntity *pProp = CreateEntityByName( "prop_physics" );
	if ( pProp )
	{
		// prop_physics reads its model during Spawn to find the physics data, so
		// the key has to be set before DispatchSpawn rather than after.
		pProp->KeyValue( "model", pszModel );
		pProp->Precache();

		// Lift off the surface along its normal so the prop is not born
		// intersecting whatever it was placed against.
		Vector vecOrigin = tr.endpos + tr.plane.normal * 16.0f;
		pProp->SetAbsOrigin( vecOrigin );

		// Face the player, upright - spawning props tilted feels broken.
		QAngle angSpawn( 0.0f, pPlayer->EyeAngles().y + 180.0f, 0.0f );
		pProp->SetAbsAngles( angSpawn );

		DispatchSpawn( pProp );
		pProp->Activate();

		// A bad model path leaves the entity alive but with no physics object,
		// which would be an invisible immovable nuisance. Drop it and say so.
		if ( pProp->VPhysicsGetObject() == NULL )
		{
			Msg( "sbox_spawn: '%s' is not a valid physics model.\n", pszModel );
			UTIL_Remove( pProp );
		}
		else
		{
			EHANDLE hProp;
			hProp = pProp;
			g_SandboxProps.AddToTail( hProp );
		}
	}

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

static ConCommand sbox_spawn( "sbox_spawn", CC_Sandbox_Spawn,
	"Spawn a physics prop where you are looking. Usage: sbox_spawn <model path>", FCVAR_GAMEDLL );


//-----------------------------------------------------------------------------
// Purpose: Remove every prop sandbox spawning created, leaving the map alone.
//-----------------------------------------------------------------------------
static void CC_Sandbox_Cleanup( const CCommand &args )
{
	int nRemoved = 0;

	for ( int i = 0; i < g_SandboxProps.Count(); ++i )
	{
		CBaseEntity *pProp = g_SandboxProps[i].Get();
		if ( pProp )
		{
			UTIL_Remove( pProp );
			++nRemoved;
		}
	}

	g_SandboxProps.Purge();

	Msg( "sbox_cleanup: removed %d prop%s.\n", nRemoved, ( nRemoved == 1 ) ? "" : "s" );
}

static ConCommand sbox_cleanup( "sbox_cleanup", CC_Sandbox_Cleanup,
	"Remove all props spawned by sandbox mode.", FCVAR_GAMEDLL );


//-----------------------------------------------------------------------------
// Purpose: Trace for whatever physics entity the player is looking at.
//			Returns NULL if there is nothing grabbable in range.
//-----------------------------------------------------------------------------
static CBaseEntity *SandboxFindTargetEntity( CBasePlayer *pPlayer, float flRange )
{
	trace_t tr;
	Vector vecForward;
	pPlayer->EyeVectors( &vecForward );
	UTIL_TraceLine( pPlayer->EyePosition(),
		pPlayer->EyePosition() + vecForward * flRange, MASK_SHOT,
		pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction == 1.0f || !tr.m_pEnt )
		return NULL;

	// The world is never grabbable. Entity index 0 is the worldspawn; its
	// physics object is static anyway, so the IsMoveable check downstream would
	// also reject it, but naming it here keeps the intent obvious.
	if ( tr.m_pEnt->entindex() == 0 )
		return NULL;

	return tr.m_pEnt;
}


//-----------------------------------------------------------------------------
// Purpose: Grab whatever is under the crosshair and carry it.
//
//			Reuses the player's own carry machinery rather than driving a
//			motion controller by hand. Passing false for bLimitMassAndSize is
//			what separates this from the +USE carry: a physgun picks up
//			anything, regardless of how heavy or bulky it is.
//
//			Bind to a button: bind <key> +sbox_grab
//-----------------------------------------------------------------------------
static void CC_Sandbox_GrabStart( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	// Already carrying something - ignore rather than swapping mid-hold.
	if ( GetPlayerHeldEntity( pPlayer ) )
		return;

	CBaseEntity *pTarget = SandboxFindTargetEntity( pPlayer, MAX_TRACE_LENGTH );
	if ( !pTarget )
		return;

	IPhysicsObject *pPhys = pTarget->VPhysicsGetObject();
	if ( !pPhys || !pPhys->IsMoveable() )
		return;

	// A frozen prop has to be woken up or it will be carried as a dead weight
	// that never responds - freezing then grabbing should just work.
	pPhys->EnableMotion( true );
	pPhys->Wake();

	pPlayer->PickupObject( pTarget, false );
}

static ConCommand sbox_grab_start( "+sbox_grab", CC_Sandbox_GrabStart,
	"Grab and carry the physics object under your crosshair.", FCVAR_GAMEDLL );


//-----------------------------------------------------------------------------
// Purpose: Release whatever is being carried.
//-----------------------------------------------------------------------------
static void CC_Sandbox_GrabStop( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	pPlayer->ForceDropOfCarriedPhysObjects( NULL );
}

static ConCommand sbox_grab_stop( "-sbox_grab", CC_Sandbox_GrabStop,
	"Release the carried physics object.", FCVAR_GAMEDLL );


//-----------------------------------------------------------------------------
// Purpose: Toggle motion on the prop under the crosshair, so props can be
//			frozen in place to build with and thawed again.
//-----------------------------------------------------------------------------
static void CC_Sandbox_Freeze( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	// Freezing what you are carrying is the normal way to place something, so
	// drop it first - otherwise the carry controller keeps fighting the freeze.
	CBaseEntity *pHeld = GetPlayerHeldEntity( pPlayer );
	if ( pHeld )
	{
		pPlayer->ForceDropOfCarriedPhysObjects( NULL );
	}

	CBaseEntity *pTarget = pHeld ? pHeld : SandboxFindTargetEntity( pPlayer, MAX_TRACE_LENGTH );
	if ( !pTarget )
		return;

	IPhysicsObject *pPhys = pTarget->VPhysicsGetObject();
	if ( !pPhys )
		return;

	if ( pPhys->IsMoveable() )
	{
		pPhys->EnableMotion( false );
	}
	else
	{
		pPhys->EnableMotion( true );
		pPhys->Wake();
	}
}

static ConCommand sbox_freeze( "sbox_freeze", CC_Sandbox_Freeze,
	"Freeze or unfreeze the prop under your crosshair.", FCVAR_GAMEDLL );
