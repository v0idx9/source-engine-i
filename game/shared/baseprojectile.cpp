//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseprojectile.h"


IMPLEMENT_NETWORKCLASS_ALIASED( BaseProjectile, DT_BaseProjectile )

BEGIN_NETWORK_TABLE( CBaseProjectile, DT_BaseProjectile )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOriginalLauncher ) ),
#else
	SendPropEHandle( SENDINFO( m_hOriginalLauncher ) ),
#endif
END_NETWORK_TABLE()


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CBaseProjectile::CBaseProjectile()
{
#ifdef GAME_DLL
	m_iDestroyableHitCount = 0;
#endif
	m_hOriginalLauncher = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Records the weapon that first fired this projectile. Kept even if
//			the projectile is later deflected, so attribution survives.
//-----------------------------------------------------------------------------
void CBaseProjectile::SetLauncher( CBaseEntity *pLauncher )
{
	if ( m_hOriginalLauncher == NULL )
	{
		m_hOriginalLauncher = pLauncher;
	}
}
