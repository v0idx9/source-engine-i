//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#define lutil_shared_cpp

#include "cbase.h"
#include "luamanager.h"
#include "lbaseentity_shared.h"
#include "lbaseplayer_shared.h"
#include "lgametrace.h"
#include "mathlib/lvector.h"
#include "luasrclib.h"
// lnet_shared.cpp is always part of the SB++ file set, so LuaNet_* symbols
// link on both realms; include its header unconditionally so util's
// network-string helpers are always available (the SBPP *macro* is not
// actually defined by the build, which previously compiled them out).
#include "lnet_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static int luasrc_UTIL_GetAllPlayers(lua_State *L) {
    lua_newtable(L);

    int index = 1;
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
        if (pPlayer && pPlayer->IsPlayer()) {
            lua_pushplayer(L, pPlayer);
            lua_rawseti(L, -2, index);
            index++;
        }
    }

    return 1;
}

static int luasrc_UTIL_VecToYaw (lua_State *L) {
  lua_pushnumber(L, UTIL_VecToYaw(luaL_checkvector(L, 1)));
  return 1;
}

static int luasrc_UTIL_VecToPitch (lua_State *L) {
  lua_pushnumber(L, UTIL_VecToPitch(luaL_checkvector(L, 1)));
  return 1;
}

static int luasrc_UTIL_YawToVector (lua_State *L) {
  Vector v = UTIL_YawToVector(luaL_checknumber(L, 1));
  lua_pushvector(L, v);
  return 1;
}

static int luasrc_SharedRandomFloat (lua_State *L) {
  lua_pushnumber(L, SharedRandomFloat(luaL_checkstring(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_optint(L, 4, 0)));
  return 1;
}

static int luasrc_SharedRandomInt (lua_State *L) {
  lua_pushinteger(L, SharedRandomInt(luaL_checkstring(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_optint(L, 4, 0)));
  return 1;
}

static int luasrc_SharedRandomVector (lua_State *L) {
  lua_pushvector(L, SharedRandomVector(luaL_checkstring(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_optint(L, 4, 0)));
  return 1;
}

static int luasrc_SharedRandomAngle (lua_State *L) {
  lua_pushangle(L, SharedRandomAngle(luaL_checkstring(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_optint(L, 4, 0)));
  return 1;
}

// Build a GMod-style trace result table from a completed trace.
static void PushGModTraceResult( lua_State *L, const trace_t &tr, const Vector &vStart, const Vector &vEnd )
{
  lua_newtable( L );

  lua_pushboolean( L, tr.DidHit() );                 lua_setfield( L, -2, "Hit" );
  lua_pushnumber ( L, tr.fraction );                 lua_setfield( L, -2, "Fraction" );
  lua_pushnumber ( L, tr.fractionleftsolid );        lua_setfield( L, -2, "FractionLeftSolid" );
  lua_pushboolean( L, tr.allsolid );                 lua_setfield( L, -2, "AllSolid" );
  lua_pushboolean( L, tr.startsolid );               lua_setfield( L, -2, "StartSolid" );
  lua_pushinteger( L, tr.hitgroup );                 lua_setfield( L, -2, "HitGroup" );
  lua_pushinteger( L, tr.hitbox );                   lua_setfield( L, -2, "HitBox" );
  lua_pushinteger( L, tr.physicsbone );              lua_setfield( L, -2, "PhysicsBone" );
  lua_pushinteger( L, tr.contents );                 lua_setfield( L, -2, "Contents" );

  Vector vEndPos = tr.endpos;      lua_pushvector( L, vEndPos ); lua_setfield( L, -2, "HitPos" );
  Vector vNormal = tr.plane.normal;lua_pushvector( L, vNormal ); lua_setfield( L, -2, "HitNormal" );
  Vector vStartOut = vStart;       lua_pushvector( L, vStartOut ); lua_setfield( L, -2, "StartPos" );
  Vector vDir = vEnd - vStart; VectorNormalize( vDir );
  lua_pushvector( L, vDir );        lua_setfield( L, -2, "Normal" );

  lua_pushentity( L, tr.m_pEnt );                    lua_setfield( L, -2, "Entity" );
  lua_pushboolean( L, tr.DidHitWorld() );            lua_setfield( L, -2, "HitWorld" );
  lua_pushboolean( L, tr.DidHitNonWorldEntity() );   lua_setfield( L, -2, "HitNonWorld" );

  if ( tr.surface.name ) { lua_pushstring( L, tr.surface.name ); lua_setfield( L, -2, "HitTexture" ); }
  lua_pushinteger( L, tr.surface.flags );            lua_setfield( L, -2, "SurfaceFlags" );
  lua_pushinteger( L, tr.surface.surfaceProps );     lua_setfield( L, -2, "SurfaceProps" );
}

static int luasrc_UTIL_TraceLine (lua_State *L) {
  // GMod form: util.TraceLine({ start=, endpos=, filter=, mask=, collisiongroup= })
  // returns a result table. This is what virtually all addons use.
  if ( lua_istable( L, 1 ) )
  {
    lua_getfield( L, 1, "start" );  int iStart = lua_gettop( L );
    Vector vStart = luaL_checkvector( L, iStart );
    lua_getfield( L, 1, "endpos" ); int iEnd = lua_gettop( L );
    Vector vEnd = luaL_checkvector( L, iEnd );

    int mask = MASK_SOLID;
    lua_getfield( L, 1, "mask" );
    if ( lua_isnumber( L, -1 ) ) mask = (int)lua_tointeger( L, -1 );
    lua_pop( L, 1 );

    int collgroup = COLLISION_GROUP_NONE;
    lua_getfield( L, 1, "collisiongroup" );
    if ( lua_isnumber( L, -1 ) ) collgroup = (int)lua_tointeger( L, -1 );
    lua_pop( L, 1 );

    CBaseEntity *pIgnore = NULL;
    lua_getfield( L, 1, "filter" );
    if ( lua_isuserdata( L, -1 ) ) pIgnore = lua_toentity( L, -1 );
    lua_pop( L, 1 );

    trace_t tr;
    UTIL_TraceLine( vStart, vEnd, mask, pIgnore, collgroup, &tr );

    // [fizzdbg] TEMP: report when a player's filtered trace (e.g. GetEyeTrace)
    // hits an NPC, so we can confirm the fizzler's trace actually finds a target.
    if ( pIgnore && pIgnore->IsPlayer() && tr.m_pEnt && tr.m_pEnt->IsNPC() )
      Msg( "[fizzdbg] eyetrace hit NPC cls=%s frac=%.3f\n", tr.m_pEnt->GetClassname(), tr.fraction );

    lua_pop( L, 2 ); // start, endpos temporaries
    PushGModTraceResult( L, tr, vStart, vEnd );
    return 1;
  }

  // Legacy positional form: (start, end, mask, ignore, collisionGroup, traceOut)
  UTIL_TraceLine(luaL_checkvector(L, 1), luaL_checkvector(L, 2), luaL_checkint(L, 3), lua_toentity(L, 4), luaL_checkint(L, 5), &luaL_checktrace(L, 6));
  return 0;
}

static int luasrc_UTIL_TraceHull (lua_State *L) {
  UTIL_TraceHull(luaL_checkvector(L, 1), luaL_checkvector(L, 2), luaL_checkvector(L, 3), luaL_checkvector(L, 4), luaL_checkint(L, 5), luaL_checkentity(L, 6), luaL_checkint(L, 7), &luaL_checktrace(L, 8));
  return 0;
}

static int luasrc_UTIL_TraceEntity (lua_State *L) {
  UTIL_TraceEntity(luaL_checkentity(L, 1), luaL_checkvector(L, 2), luaL_checkvector(L, 3), luaL_checkint(L, 4), luaL_checkentity(L, 5), luaL_checkint(L, 5), &luaL_checktrace(L, 6));
  return 0;
}

static int luasrc_UTIL_EntityHasMatchingRootParent (lua_State *L) {
  lua_pushboolean(L, UTIL_EntityHasMatchingRootParent(luaL_checkentity(L, 1), luaL_checkentity(L, 2)));
  return 1;
}

static int luasrc_UTIL_PointContents (lua_State *L) {
  lua_pushinteger(L, UTIL_PointContents(luaL_checkvector(L, 1)));
  return 1;
}

static int luasrc_UTIL_TraceModel (lua_State *L) {
  UTIL_TraceModel(luaL_checkvector(L, 1), luaL_checkvector(L, 2), luaL_checkvector(L, 3), luaL_checkvector(L, 4), luaL_checkentity(L, 5), luaL_checkint(L, 6), &luaL_checktrace(L, 7));
  return 0;
}

static int luasrc_UTIL_ParticleTracer (lua_State *L) {
  UTIL_ParticleTracer(luaL_checkstring(L, 1), luaL_checkvector(L, 2), luaL_checkvector(L, 3), luaL_optint(L, 4, 0), luaL_optint(L, 5, 0), luaL_optboolean(L, 6, 0));
  return 0;
}

static int luasrc_UTIL_Tracer (lua_State *L) {
  UTIL_Tracer(luaL_checkvector(L, 1), luaL_checkvector(L, 2), luaL_optint(L, 3, 0), luaL_optint(L, 4, -1), luaL_optnumber(L, 5, 0), luaL_optboolean(L, 6, 0), luaL_optstring(L, 7, 0), luaL_optint(L, 8, 0));
  return 0;
}

static int luasrc_UTIL_BloodDrips (lua_State *L) {
  UTIL_BloodDrips(luaL_checkvector(L, 1), luaL_checkvector(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
  return 0;
}

static int luasrc_UTIL_IsLowViolence (lua_State *L) {
  lua_pushboolean(L, UTIL_IsLowViolence());
  return 1;
}

static int luasrc_UTIL_ShouldShowBlood (lua_State *L) {
  lua_pushboolean(L, UTIL_ShouldShowBlood(luaL_checkint(L, 1)));
  return 1;
}

static int luasrc_UTIL_BloodImpact (lua_State *L) {
  UTIL_BloodImpact(luaL_checkvector(L, 1), luaL_checkvector(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
  return 0;
}

static int luasrc_UTIL_BloodDecalTrace (lua_State *L) {
  UTIL_BloodDecalTrace(&luaL_checktrace(L, 1), luaL_checkint(L, 2));
  return 0;
}

static int luasrc_UTIL_DecalTrace (lua_State *L) {
  UTIL_DecalTrace(&luaL_checktrace(L, 1), luaL_checkstring(L, 2));
  return 0;
}

static int luasrc_UTIL_IsSpaceEmpty (lua_State *L) {
  lua_pushboolean(L, UTIL_IsSpaceEmpty(luaL_checkentity(L, 1), luaL_checkvector(L, 2), luaL_checkvector(L, 3)));
  return 1;
}

static int luasrc_UTIL_PlayerByIndex (lua_State *L) {
  lua_pushplayer(L, UTIL_PlayerByIndex(luaL_checkint(L, 1)));
  return 1;
}

static int luasrc_UTIL_AddNetworkString( lua_State *L )
{
  const char *name = luaL_checkstring( L, 1 );
#ifdef CLIENT_DLL
  return luaL_error( L, "util.AddNetworkString can only be called on the server" );
#else
  int id = LuaNet_AddString( name );
  lua_pushinteger( L, id );
  return 1;
#endif
}

static int luasrc_UTIL_NetworkStringToID( lua_State *L )
{
  lua_pushinteger( L, LuaNet_GetStringID( luaL_checkstring( L, 1 ) ) );
  return 1;
}

static int luasrc_UTIL_NetworkIDToString( lua_State *L )
{
  const char *s = LuaNet_GetStringName( luaL_checkint( L, 1 ) );
  if ( s ) lua_pushstring( L, s );
  else     lua_pushnil( L );
  return 1;
}

static const luaL_Reg util_funcs[] = {
  // {"UTIL_VecToYaw",  luasrc_UTIL_VecToYaw},
  {"VecToYaw",  luasrc_UTIL_VecToYaw},
  // {"UTIL_VecToPitch",  luasrc_UTIL_VecToPitch},
  {"VecToPitch",  luasrc_UTIL_VecToPitch},
  // {"UTIL_YawToVector",  luasrc_UTIL_YawToVector},
  {"YawToVector",  luasrc_UTIL_YawToVector},
  {"SharedRandomFloat",  luasrc_SharedRandomFloat},
  {"SharedRandomInt",  luasrc_SharedRandomInt},
  {"SharedRandomVector",  luasrc_SharedRandomVector},
  {"SharedRandomAngle",  luasrc_SharedRandomAngle},
  // {"UTIL_TraceLine",  luasrc_UTIL_TraceLine},
  {"TraceLine",  luasrc_UTIL_TraceLine},
  // {"UTIL_TraceHull",  luasrc_UTIL_TraceHull},
  {"TraceHull",  luasrc_UTIL_TraceHull},
  // {"UTIL_TraceEntity",  luasrc_UTIL_TraceEntity},
  {"TraceEntity",  luasrc_UTIL_TraceEntity},
  // {"UTIL_EntityHasMatchingRootParent",  luasrc_UTIL_EntityHasMatchingRootParent},
  {"EntityHasMatchingRootParent",  luasrc_UTIL_EntityHasMatchingRootParent},
  // {"UTIL_PointContents",  luasrc_UTIL_PointContents},
  {"PointContents",  luasrc_UTIL_PointContents},
  // {"UTIL_TraceModel",  luasrc_UTIL_TraceModel},
  {"TraceModel",  luasrc_UTIL_TraceModel},
  // {"UTIL_ParticleTracer",  luasrc_UTIL_ParticleTracer},
  {"ParticleTracer",  luasrc_UTIL_ParticleTracer},
  // {"UTIL_Tracer",  luasrc_UTIL_Tracer},
  {"Tracer",  luasrc_UTIL_Tracer},
  // {"UTIL_IsLowViolence",  luasrc_UTIL_IsLowViolence},
  {"IsLowViolence",  luasrc_UTIL_IsLowViolence},
  // {"UTIL_ShouldShowBlood",  luasrc_UTIL_ShouldShowBlood},
  {"ShouldShowBlood",  luasrc_UTIL_ShouldShowBlood},
  // {"UTIL_BloodDrips",  luasrc_UTIL_BloodDrips},
  {"BloodDrips",  luasrc_UTIL_BloodDrips},
  // {"UTIL_BloodImpact",  luasrc_UTIL_BloodImpact},
  {"BloodImpact",  luasrc_UTIL_BloodImpact},
  // {"UTIL_BloodDecalTrace",  luasrc_UTIL_BloodDecalTrace},
  {"BloodDecalTrace",  luasrc_UTIL_BloodDecalTrace},
  // {"UTIL_DecalTrace",  luasrc_UTIL_DecalTrace},
  {"DecalTrace",  luasrc_UTIL_DecalTrace},
  // {"UTIL_IsSpaceEmpty",  luasrc_UTIL_IsSpaceEmpty},
  {"IsSpaceEmpty",  luasrc_UTIL_IsSpaceEmpty},
  // {"UTIL_PlayerByIndex",  luasrc_UTIL_PlayerByIndex},
  {"PlayerByIndex",  luasrc_UTIL_PlayerByIndex},
  {"GetAllPlayers", luasrc_UTIL_GetAllPlayers},
  {"AddNetworkString", luasrc_UTIL_AddNetworkString},
  {"NetworkStringToID", luasrc_UTIL_NetworkStringToID},
  {"NetworkIDToString", luasrc_UTIL_NetworkIDToString},
  {NULL, NULL}
};


LUALIB_API int luaopen_UTIL_shared (lua_State *L) {
  // luaL_register(L, "_G", util_funcs);
  luaL_register(L, LUA_UTILLIBNAME, util_funcs);
  return 1;
}

// The engine registers all native util functions into the global "UTIL"
// (uppercase, LUA_UTILLIBNAME), but GMod addons use "util" (lowercase). They
// were therefore two different tables and addons never saw TraceLine,
// AddNetworkString, EntitiesInSphere, etc. Mirror the shared util functions
// onto lowercase "util" (creating it if needed) so addon code and the
// GMod-compat shims can reach them (e.g. util.TraceLine's GMod table form).
LUALIB_API void luasrc_EnsureGModUtil (lua_State *L) {
  // Ensure lowercase "util" exists.
  lua_getglobal(L, "util");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "util");
  }
  int utilIdx = lua_gettop(L);

  // 1) Register this file's shared util functions (TraceLine w/ GMod table
  //    form, network strings, traces, decals, ...) onto lowercase util.
  for (const luaL_Reg *r = util_funcs; r->name; r++) {
    lua_pushcfunction(L, r->func);
    lua_setfield(L, utilIdx, r->name);
  }

  // 2) Copy everything the engine put on uppercase "UTIL" (server-side funcs
  //    like EntitiesInSphere/EntitiesInBox/EntityByIndex/ClientPrint, etc.)
  //    onto lowercase util too, so the whole native util API is reachable.
  lua_getglobal(L, LUA_UTILLIBNAME); // "UTIL"
  if (lua_istable(L, -1)) {
    int upperIdx = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, upperIdx) != 0) {
      // key at -2, value at -1; set util[key] = value (don't overwrite ours)
      lua_pushvalue(L, -2); // key
      lua_gettable(L, utilIdx);
      bool exists = !lua_isnil(L, -1);
      lua_pop(L, 1);
      if (!exists) {
        lua_pushvalue(L, -2); // key
        lua_pushvalue(L, -2); // value
        lua_settable(L, utilIdx);
      }
      lua_pop(L, 1); // pop value, keep key
    }
  }
  lua_pop(L, 1); // UTIL

  lua_pop(L, 1); // util
}

