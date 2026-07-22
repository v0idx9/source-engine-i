//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Assorted tier1 helpers.
//
//			TF2's code includes this as both "tier1/util_misc.h" and
//			"util_misc.h", but Valve did not ship the header in the public
//			Source SDK, so there is nothing to copy. This pulls in the tier1
//			utilities those translation units rely on; anything genuinely
//			missing will surface as an undeclared identifier rather than being
//			silently wrong.
//
//=============================================================================

#ifndef UTIL_MISC_H
#define UTIL_MISC_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier1/strtools.h"
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"
#include "tier1/KeyValues.h"

#endif // UTIL_MISC_H
