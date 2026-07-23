//========== Copyleft � 2011, Team Sandbox, Some rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//


#define lsrcinit_cpp

#include "cbase.h"
#include "lua.hpp"

#include "luasrclib.h"
#include "lauxlib.h"


static const luaL_Reg luasrclibs[] = {
  {LUA_BASEANIMATINGLIBNAME, luaopen_CBaseAnimating},
  {LUA_BASECOMBATWEAPONLIBNAME, luaopen_CBaseCombatWeapon},
  {LUA_BASEENTITYLIBNAME, luaopen_CBaseEntity},
  {LUA_BASEENTITYLIBNAME, luaopen_CBaseEntity_shared},
  {LUA_BASEPLAYERLIBNAME, luaopen_CBasePlayer},
  {LUA_BASEPLAYERLIBNAME, luaopen_CBasePlayer_shared},
  {LUA_EFFECTDATALIBNAME, luaopen_CEffectData},
  {LUA_GAMETRACELIBNAME, luaopen_CGameTrace},
#ifdef SBPP
  {LUA_NETLIBNAME, luaopen_net},
#endif
#ifndef CLIENT_DLL
  {LUA_EFFECTSLIBNAME, luaopen_Effects},
  {LUA_HL2MPPLAYERLIBNAME, luaopen_CHL2MP_Player},
#ifdef SBPP
  {LUA_NAVAREALIBNAME, luaopen_CNavArea},
  {LUA_NAVMESHLIBNAME, luaopen_navmesh},
  {LUA_MOVEDATALIBNAME, luaopen_CMoveData},
	{LUA_BASEFLEXLIBNAME, luaopen_CBaseFlex},
	{LUA_BASEANIMATINGOVERLAYLIBNAME, luaopen_CBaseAnimatingOverlay},
  
	{LUA_NEXTBOT_LOCOMOTIONLIBNAME, luaopen_NextBotLocomotion},
	{LUA_NEXTBOT_PATHLIBNAME, luaopen_CNextBotPath},
	{LUA_NEXTBOTLIBNAME, luaopen_CLuaNextBot},

#endif
#else
#ifdef SBPP
  {LUA_SPAWNMENULIBNAME, luaopen_sm},
	{LUA_BASEFLEXLIBNAME, luaopen_CBaseFlex},
	{LUA_BASEANIMATINGOVERLAYLIBNAME, luaopen_CBaseAnimatingOverlay},

  // material crap
  {LUA_IMESHLIBNAME, luaopen_IMesh},
  {"materials", luaopen_materials},
  {LUA_MESHBUILDERLIBNAME, luaopen_CMeshBuilder},
  {LUA_TEXTURELIBNAME, luaopen_ITexture},
  {LUA_MATRENDERCONTEXTLIBNAME, luaopen_IMatRenderContext},
  {LUA_MATERIALVARLIBNAME, luaopen_IMaterialVar},
#endif
#endif
  {LUA_AMMODEFLIB_NAME, luaopen_AmmoDef},
  {LUA_HL2MPPLAYERLIBNAME, luaopen_CHL2MP_Player_shared},
  {LUA_COLORLIBNAME, luaopen_Color},
  {LUA_CONCOMMANDLIBNAME, luaopen_ConCommand},
  {LUA_CONTENTSLIBNAME, luaopen_CONTENTS},
  {LUA_CONVARLIBNAME, luaopen_ConVar},
  {LUA_PASFILTERLIBNAME, luaopen_CPASFilter},
  {LUA_RECIPIENTFILTERLIBNAME, luaopen_CRecipientFilter},
  {LUA_TAKEDAMAGEINFOLIBNAME, luaopen_CTakeDamageInfo},
  {LUA_CVARLIBNAME, luaopen_cvar},
  {LUA_DBGLIBNAME, luaopen_dbg},
  {LUA_DEBUGOVERLAYLIBNAME, luaopen_debugoverlay},
  {LUA_ENGINELIBNAME, luaopen_engine},
#ifdef CLIENT_DLL
  // FIXME: obsolete? should be passing VPANELs, but passes Panel instead,
  // which always ends up being invalid (we can't access them by pointer)
  {LUA_ENGINEVGUILIBNAME, luaopen_enginevgui},
#endif
  {LUA_FCVARLIBNAME, luaopen_FCVAR},
  {LUA_FILESYSTEMLIBNAME, luaopen_filesystem},
#ifdef CLIENT_DLL
  {LUA_FONTFLAGLIBNAME, luaopen_FONTFLAG},
#endif
#ifndef CLIENT_DLL
  {LUA_ENTLISTLIBNAME, luaopen_gEntList},
#endif
  {LUA_GLOBALSLIBNAME, luaopen_gpGlobals},
#ifdef CLIENT_DLL
  {LUA_CLIENTSHADOWMGRLIBNAME, luaopen_g_pClientShadowMgr},
  {LUA_FONTLIBNAME, luaopen_HFont},
  {LUA_HSCHEMELIBNAME, luaopen_HScheme},
#endif
  {LUA_MATERIALLIBNAME, luaopen_IMaterial},
  {LUA_MOVEHELPERLIBNAME, luaopen_IMoveHelper},
  {LUA_INLIBNAME, luaopen_IN},
#ifndef CLIENT_DLL
  {LUA_NETCHANNELINFOLIBNAME, luaopen_INetChannelInfo},
#endif
  {LUA_INETWORKSTRINGTABLELIBNAME, luaopen_INetworkStringTable},
#ifdef CLIENT_DLL
  {LUA_INPUTLIBNAME, luaopen_input},
#endif
  {LUA_PHYSICSOBJECTLIBNAME, luaopen_IPhysicsObject},
  {LUA_PHYSICSSURFACEPROPSLIBNAME, luaopen_IPhysicsSurfaceProps},
  {LUA_PREDICTIONSYSTEMLIBNAME, luaopen_IPredictionSystem},
#ifdef CLIENT_DLL
  {LUA_ISCHEMELIBNAME, luaopen_IScheme},
#endif
//  {LUA_STEAMFRIENDSLIBNAME, luaopen_ISteamFriends},
  {LUA_KEYVALUESLIBNAME, luaopen_KeyValues},
  {LUA_MASKLIBNAME, luaopen_MASK},
  {LUA_MATHLIBLIBNAME, luaopen_mathlib},
  {LUA_MATRIXLIBNAME, luaopen_matrix3x4_t},
  {LUA_NETWORKSTRINGTABLELIBNAME, luaopen_networkstringtable},
#ifdef CLIENT_DLL
  {LUA_PANELLIBNAME, luaopen_Panel},
#endif
  {LUA_PHYSENVLIBNAME, luaopen_physenv},
#ifdef CLIENT_DLL
  {LUA_PREDICTIONLIBNAME, luaopen_prediction},
#endif
  {LUA_QANGLELIBNAME, luaopen_QAngle},
  {LUA_RANDOMLIBNAME, luaopen_random},
#ifdef CLIENT_DLL
  {LUA_SCHEMELIBNAME, luaopen_scheme},
#endif
//  {LUA_STEAMAPICONTEXTLIBNAME, luaopen_steamapicontext},
  {LUA_SURFLIBNAME, luaopen_SURF},
#ifdef CLIENT_DLL
  {LUA_SURFACELIBNAME, luaopen_surface},
#endif
  {LUA_UTILLIBNAME, luaopen_UTIL},
  {LUA_UTILLIBNAME, luaopen_UTIL_shared},
  {LUA_VECTORLIBNAME, luaopen_Vector},
#ifdef CLIENT_DLL
  {LUA_VGUILIBNAME, luaopen_vgui},
#endif
  {LUA_VMATRIXLIBNAME, luaopen_VMatrix},
  {LUA_COLLISION_GROUPNAME, luaopen_COLLISION_GROUP},
  {NULL, NULL}
};


// GMod compatibility prelude. Defines the pieces of the Garry's Mod standard
// library that this engine lacks, so Workshop SWEPs/SENTs load. It is strictly
// defensive: every definition is gated on the name being absent, so it never
// overwrites a real implementation provided by the engine or base content.
// Hard-to-emulate features degrade to safe no-ops (a mounted SWEP still
// registers and appears even if e.g. its constraints or undo entries do
// nothing yet).
static const char *s_pGModCompatPrelude = R"GLUACOMPAT(
-- type helpers
if isfunction == nil then function isfunction(v) return type(v) == "function" end end
if istable    == nil then function istable(v)    return type(v) == "table" end end
if isstring   == nil then function isstring(v)   return type(v) == "string" end end
if isnumber   == nil then function isnumber(v)   return type(v) == "number" end end
if isbool     == nil then function isbool(v)     return type(v) == "boolean" end end
if isvector   == nil then function isvector(v)   return type(v) == "Vector" end end
if isangle    == nil then function isangle(v)    return type(v) == "Angle" end end
if ispanel    == nil then function ispanel(v)    return type(v) == "Panel" end end
if isentity   == nil then
  function isentity(v)
    local t = type(v)
    return t == "Entity" or t == "Player" or t == "NPC" or t == "Vehicle" or t == "Weapon" or t == "NextBot"
  end
end

-- math / misc helpers
math.Clamp = math.Clamp or function(v, lo, hi) if v < lo then return lo elseif v > hi then return hi end return v end
math.Round = math.Round or function(v, d) local m = 10 ^ (d or 0) return math.floor(v * m + 0.5) / m end
math.Rand  = math.Rand  or function(a, b) return a + (b - a) * math.random() end
math.AngleDifference = math.AngleDifference or function(a, b) local d = (a - b) % 360 if d > 180 then d = d - 360 end return d end
if Lerp == nil then function Lerp(t, a, b) return a + (b - a) * t end end

-- string helpers
string.Replace = string.Replace or function(s, find, rep)
  if find == nil or find == "" then return s end
  local pat = (string.gsub(find, "([%-%.%+%[%]%(%)%$%^%%%?%*])", "%%%1"))
  local sub = (string.gsub(rep, "%%", "%%%%"))
  return (string.gsub(s, pat, sub))
end
string.GetFileFromFilename = string.GetFileFromFilename or function(p) p = string.gsub(p, "\\", "/") return string.match(p, "[^/]*$") or p end
string.NiceName = string.NiceName or function(s) return (string.gsub(tostring(s), "[_%.]", " ")) end

-- table helpers
table.Count = table.Count or function(t) local n = 0 for _ in pairs(t) do n = n + 1 end return n end

-- language
if language == nil then
  language = {}
  local phrases = {}
  function language.Add(k, v) phrases[k] = v end
  function language.GetPhrase(k) return phrases[k] or k end
  function language.GetLanguages() return {} end
end

-- cvars
if cvars == nil then
  cvars = {}
  local callbacks = {}
  function cvars.AddChangeCallback(name, fn, id) callbacks[name] = callbacks[name] or {} callbacks[name][id or (#callbacks[name] + 1)] = fn end
  function cvars.RemoveChangeCallback(name, id) if callbacks[name] then callbacks[name][id] = nil end end
  function cvars.Number(n, d) local c = GetConVar(n) return c and c:GetFloat() or (d or 0) end
  function cvars.String(n, d) local c = GetConVar(n) return c and c:GetString() or (d or "") end
  function cvars.Bool(n, d) local c = GetConVar(n) return c and c:GetBool() or (d or false) end
end

-- sound registration (playback still uses the engine directly)
if sound == nil then sound = {} end
sound.Add = sound.Add or function() end
sound.AddSoundOverrides = sound.AddSoundOverrides or function() end

-- server/shared prop-management shims (no-op where unsupported)
if cleanup == nil then
  cleanup = {}
  function cleanup.Add() end
  function cleanup.Register() end
  function cleanup.GetTable() return {} end
  function cleanup.ReplaceEntities() end
end
if undo == nil then
  undo = {}
  function undo.Create() end
  function undo.AddEntity() end
  function undo.AddFunction() end
  function undo.SetPlayer() end
  function undo.SetCustomUndoText() end
  function undo.Finish() end
end
if constraint == nil then
  constraint = {}
  local function nilfn() return nil end
  constraint.Weld = nilfn
  constraint.NoCollide = nilfn
  constraint.Rope = nilfn
  constraint.Axis = nilfn
  constraint.Ballsocket = nilfn
  constraint.Elastic = nilfn
  constraint.Keepupright = nilfn
  function constraint.RemoveAll() end
  function constraint.RemoveConstraints() end
  function constraint.FindConstraint() return nil end
  function constraint.HasConstraints() return false end
end

-- spawnmenu tool-menu registration (the spawnmenu table itself is native)
if spawnmenu == nil then spawnmenu = {} end
spawnmenu.AddToolMenuOption = spawnmenu.AddToolMenuOption or function() end
spawnmenu.AddToolCategory = spawnmenu.AddToolCategory or function() end

-- client-only rendering / UI shims
if CLIENT then
  if killicon == nil then
    killicon = {}
    function killicon.Add() end
    function killicon.AddFont() end
    function killicon.AddAlias() end
    function killicon.Exists() return false end
    function killicon.Draw() end
    function killicon.GetSize() return 0, 0 end
  end
  if surface ~= nil then
    surface.CreateFont = surface.CreateFont or function() end
  end
  if cam == nil then
    cam = {}
    function cam.Start3D() end
    function cam.End3D() end
    function cam.Start2D() end
    function cam.End2D() end
  end
  if markup == nil then
    markup = {}
    function markup.Parse(text)
      local o = { text = tostring(text or "") }
      function o:Draw() end
      function o:GetWidth() return 0 end
      function o:GetHeight() return 0 end
      function o:Size() return 0, 0 end
      return o
    end
  end
  if Derma_DrawBackgroundBlur == nil then function Derma_DrawBackgroundBlur() end end
end
)GLUACOMPAT";

LUALIB_API void luasrc_openlibs (lua_State *L) {
  const luaL_Reg *lib = luasrclibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }

  // Fill in the GMod stdlib gaps after the native libraries are registered.
  if ( luaL_dostring( L, s_pGModCompatPrelude ) != 0 )
  {
    Warning( "GMod compat prelude error: %s\n", lua_tostring( L, -1 ) );
    lua_pop( L, 1 );
  }
}

