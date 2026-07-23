//========== Copyleft © 2011, Team Sandbox, Some rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "luamanager.h"
#include "tier0/icommandline.h"
#include "zip/miniz.h"
#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Weapon classes discovered in mounted addons this session. The HL2:SB++ spawn
// menu is built entirely from settings/spawnlists/*.kv manifest files and never
// enumerates registered SWEPs, so a mounted addon's weapon would load but never
// appear. We collect the classes here and emit a generated spawnlist tab.
static CUtlVector<CUtlString> g_AddonWeaponClasses;

static void CollectAddonWeaponClass( const char *fileRelName )
{
	// fileRelName is a GMA-relative path like "lua/weapons/weapon_x.lua" or
	// "lua/weapons/weapon_x/shared.lua". Extract the class (weapon_x).
	static const char *kPrefix = "lua/weapons/";
	const int		   prefixLen = 12; // strlen("lua/weapons/")
	if ( Q_strnicmp( fileRelName, kPrefix, prefixLen ) != 0 )
		return;

	const char *rest = fileRelName + prefixLen;
	if ( rest[0] == '\0' )
		return;

	char cls[128];
	int	 i = 0;
	for ( ; rest[i] && rest[i] != '/' && rest[i] != '.' && i < (int)sizeof( cls ) - 1; i++ )
		cls[i] = rest[i];
	cls[i] = '\0';
	if ( cls[0] == '\0' )
		return;

	FOR_EACH_VEC( g_AddonWeaponClasses, k )
	{
		if ( Q_stricmp( g_AddonWeaponClasses[k].Get(), cls ) == 0 )
			return; // already have it (folder SWEP has several files)
	}
	g_AddonWeaponClasses.AddToTail( CUtlString( cls ) );
}

// Emit settings/spawnlists/zz_addons.kv listing every mounted addon weapon so
// the spawn menu shows them in an "Addons" tab. Uses ent_create (FCVAR_GAMEDLL,
// no sv_cheats needed) so clicking an entry spawns the weapon to pick up.
static void WriteAddonSpawnlist()
{
	const char *kPath = "settings/spawnlists/zz_addons.kv";

	if ( g_AddonWeaponClasses.Count() == 0 )
	{
		// Nothing to list; remove any stale generated file.
		if ( filesystem->FileExists( kPath, "MOD" ) )
			filesystem->RemoveFile( kPath, "MOD" );
		return;
	}

	filesystem->CreateDirHierarchy( "settings/spawnlists", "MOD" );

	FileHandle_t fh = g_pFullFileSystem->Open( kPath, "wt", "MOD" );
	if ( fh == FILESYSTEM_INVALID_HANDLE )
	{
		Warning( "Failed to write generated addon spawnlist: %s\n", kPath );
		return;
	}

	g_pFullFileSystem->FPrintf( fh, "\"Addons\"\n{\n" );
	g_pFullFileSystem->FPrintf( fh, "\t\"page_icon\" \"materials/icon16/bricks.png\"\n" );
	g_pFullFileSystem->FPrintf( fh, "\t\"header\"\n\t{\n\t\t\"text\" \"Mounted Addon Weapons\"\n\t}\n" );

	FOR_EACH_VEC( g_AddonWeaponClasses, k )
	{
		const char *cls = g_AddonWeaponClasses[k].Get();

		// Icon lookup, in GMod-preference order:
		//  1. materials/vgui/entities/<class>.vmt  (the standard SWEP spawn
		//     icon; the menu loads .vmt/.vtf as a VGUI material - pass the
		//     material-relative path WITH extension, no "materials/" prefix)
		//  2. materials/entities/<class>.png       (loaded as a PNG file)
		//  3. a stock fallback icon
		char icon[MAX_PATH];
		char probe[MAX_PATH];
		Q_snprintf( probe, sizeof( probe ), "materials/vgui/entities/%s.vmt", cls );
		if ( filesystem->FileExists( probe, "GAME" ) )
		{
			Q_snprintf( icon, sizeof( icon ), "vgui/entities/%s.vmt", cls );
		}
		else
		{
			Q_snprintf( icon, sizeof( icon ), "materials/entities/%s.png", cls );
			if ( !filesystem->FileExists( icon, "GAME" ) )
				Q_strncpy( icon, "materials/entities/weapon_crowbar.png", sizeof( icon ) );
		}

		g_pFullFileSystem->FPrintf( fh, "\t\"image_button\"\n\t{\n" );
		g_pFullFileSystem->FPrintf( fh, "\t\t\"name\" \"%s\"\n", cls );
		g_pFullFileSystem->FPrintf( fh, "\t\t\"image\" \"%s\"\n", icon );
		g_pFullFileSystem->FPrintf( fh, "\t\t\"command\" \"ent_create %s\"\n", cls );
		g_pFullFileSystem->FPrintf( fh, "\t}\n" );
	}

	g_pFullFileSystem->FPrintf( fh, "}\n" );
	g_pFullFileSystem->Close( fh );

	DevMsg( "Wrote generated addon spawnlist with %d weapon(s): %s\n", g_AddonWeaponClasses.Count(), kPath );
}

static void ExtractZIP( const char *zipPath, const char *cacheBase, const char *gamePath )
{
	if ( !zipPath || !cacheBase )
		return;

	const char *baseNamePtr = V_UnqualifiedFileName( zipPath ); // "popcorn_swep.zip"
	char		baseName[MAX_PATH];
	Q_strncpy( baseName, baseNamePtr ? baseNamePtr : zipPath, sizeof( baseName ) );

	char addonName[MAX_PATH];
	Q_strncpy( addonName, baseName, sizeof( addonName ) );
	char *dot = Q_strrchr( addonName, '.' );
	if ( dot )
		*dot = '\0';

	char relOutBase[MAX_PATH];
	Q_snprintf( relOutBase, sizeof( relOutBase ), "%s/zips/%s", cacheBase, addonName );
	Q_FixSlashes( relOutBase );

	filesystem->CreateDirHierarchy( relOutBase, "MOD" );

	mz_zip_archive zip;
	memset( &zip, 0, sizeof( zip ) );
	if ( !mz_zip_reader_init_file( &zip, zipPath, 0 ) )
	{
		Warning( "Failed to open ZIP: %s\n", zipPath );
		return;
	}

	int numFiles = (int)mz_zip_reader_get_num_files( &zip );
	for ( int i = 0; i < numFiles; ++i )
	{
		mz_zip_archive_file_stat fileStat;
		if ( !mz_zip_reader_file_stat( &zip, i, &fileStat ) )
			continue;

		if ( mz_zip_reader_is_file_a_directory( &zip, i ) )
			continue;

		const char *fileName = fileStat.m_filename;
		const char *firstSlash = strchr( fileName, '/' );
		if ( firstSlash )
		{
			fileName = firstSlash + 1;
		}

		if ( !fileName || fileName[0] == '\0' )
		{
			DevMsg( "Skipping empty path from: %s\n", fileStat.m_filename );
			continue;
		}

		DevMsg( "Extracting: %s -> %s/%s\n", fileStat.m_filename, relOutBase, fileName );

		char relOutPath[MAX_PATH];
		Q_snprintf( relOutPath, sizeof( relOutPath ), "%s/%s", relOutBase, fileName );
		Q_FixSlashes( relOutPath );

		char dirBuf[MAX_PATH];
		Q_strncpy( dirBuf, relOutPath, sizeof( dirBuf ) );
		char *lastSlash = Q_strrchr( dirBuf, '/' );
		if ( lastSlash )
		{
			*lastSlash = '\0'; // keep directory only
			filesystem->CreateDirHierarchy( dirBuf, "MOD" );
		}
		else
			filesystem->CreateDirHierarchy( relOutBase, "MOD" );

		size_t uncomp_size = 0;
		void  *p = mz_zip_reader_extract_to_heap( &zip, i, &uncomp_size, 0 );
		if ( !p )
		{
			Warning( "Failed to extract (to heap) %s from ZIP %s\n", fileStat.m_filename, zipPath );
			continue;
		}

		FileHandle_t fh = g_pFullFileSystem->Open( relOutPath, "wb", "MOD" );
		if ( fh == FILESYSTEM_INVALID_HANDLE )
		{
			Warning( "Failed to open output '%s' for writing in Source FS\n", relOutPath );
			mz_free( p );
			continue;
		}

		if ( uncomp_size > INT_MAX )
		{
			Warning( "File too large to write via Source FS: %s (size=%zu)\n", relOutPath, uncomp_size );
			g_pFullFileSystem->Close( fh );
			mz_free( p );
			continue;
		}

		int written = g_pFullFileSystem->Write( p, (int)uncomp_size, fh );
		if ( written != (int)uncomp_size )
		{
			Warning( "Write mismatch for %s (wrote %d, expected %zu)\n", relOutPath, written, uncomp_size );
		}
		g_pFullFileSystem->Close( fh );

		// free buffer from miniz
		mz_free( p );
	}

	mz_zip_reader_end( &zip );

	char absOutBase[MAX_PATH];
	Q_snprintf( absOutBase, sizeof( absOutBase ), "%s/%s", gamePath, relOutBase );
	Q_FixSlashes( absOutBase );

	filesystem->AddSearchPath( absOutBase, "MOD", PATH_ADD_TO_HEAD );
	filesystem->AddSearchPath( absOutBase, "GAME", PATH_ADD_TO_HEAD );

	DevMsg( "Mounted ZIP addon: %s -> %s\n", zipPath, absOutBase );
}

//-----------------------------------------------------------------------------
// GMA (Garry's Mod addon) support.
//
// A .gma is not a zip and the Source filesystem has no reader for it, so
// AddSearchPath() on a .gma silently does nothing. Instead we parse the GMAD
// container and stream every file out into cache/gma/<addon>/, then mount that
// folder just like an extracted zip. Format (little-endian, matches iOS):
//   char[4] "GMAD" | u8 version | u64 steamid | u64 timestamp
//   [version>1] required-content: NUL strings until an empty one
//   string name | string description | string author | i32 addonVersion
//   index: repeat{ u32 fileNumber(0=end) | string name | i64 size | u32 crc }
//   then every file's bytes concatenated in index order, then u32 addonCRC.
//-----------------------------------------------------------------------------
#define GMAD_MAX_VERSION 3
#define GMAD_MAX_FILES   65536

static bool GMA_ReadRaw( FILE *f, void *out, size_t n )
{
	return fread( out, 1, n, f ) == n;
}

// Reads a NUL-terminated string. Pass out==NULL to skip/discard it.
static bool GMA_ReadString( FILE *f, char *out, size_t outSize )
{
	size_t i = 0;
	for ( ;; )
	{
		int c = fgetc( f );
		if ( c == EOF )
			return false;
		if ( c == 0 )
			break;
		if ( out && i + 1 < outSize )
			out[i++] = (char)c;
	}
	if ( out )
		out[i] = '\0';
	return true;
}

struct gma_entry_t
{
	char	  name[MAX_PATH];
	long long size;
};

static void ExtractGMA( const char *gmaPath, const char *cacheBase, const char *gamePath )
{
	if ( !gmaPath || !cacheBase )
		return;

	FILE *f = fopen( gmaPath, "rb" );
	if ( !f )
	{
		Warning( "Failed to open GMA: %s\n", gmaPath );
		return;
	}

	// Header.
	char ident[4];
	if ( !GMA_ReadRaw( f, ident, 4 ) || Q_strncmp( ident, "GMAD", 4 ) != 0 )
	{
		Warning( "Not a valid GMA (bad ident): %s\n", gmaPath );
		fclose( f );
		return;
	}

	unsigned char version = 0;
	unsigned long long steamid = 0, timestamp = 0;
	if ( !GMA_ReadRaw( f, &version, 1 ) ||
		 !GMA_ReadRaw( f, &steamid, 8 ) ||
		 !GMA_ReadRaw( f, &timestamp, 8 ) )
	{
		Warning( "Truncated GMA header: %s\n", gmaPath );
		fclose( f );
		return;
	}

	if ( version > GMAD_MAX_VERSION )
	{
		Warning( "Unsupported GMA version %d (max %d): %s\n", version, GMAD_MAX_VERSION, gmaPath );
		fclose( f );
		return;
	}

	// Required-content list (discarded).
	if ( version > 1 )
	{
		char tmp[256];
		do
		{
			if ( !GMA_ReadString( f, tmp, sizeof( tmp ) ) )
			{
				Warning( "Truncated GMA required-content list: %s\n", gmaPath );
				fclose( f );
				return;
			}
		} while ( tmp[0] != '\0' );
	}

	int addonVersion = 0;
	if ( !GMA_ReadString( f, NULL, 0 ) ||	// addon name
		 !GMA_ReadString( f, NULL, 0 ) ||	// description (json)
		 !GMA_ReadString( f, NULL, 0 ) ||	// author
		 !GMA_ReadRaw( f, &addonVersion, 4 ) )
	{
		Warning( "Truncated GMA metadata: %s\n", gmaPath );
		fclose( f );
		return;
	}

	// File index.
	CUtlVector<gma_entry_t> entries;
	for ( ;; )
	{
		unsigned int fileNumber = 0;
		if ( !GMA_ReadRaw( f, &fileNumber, 4 ) )
		{
			Warning( "Truncated GMA file index: %s\n", gmaPath );
			fclose( f );
			return;
		}
		if ( fileNumber == 0 )
			break;

		gma_entry_t e;
		unsigned int crc = 0;
		if ( !GMA_ReadString( f, e.name, sizeof( e.name ) ) ||
			 !GMA_ReadRaw( f, &e.size, 8 ) ||
			 !GMA_ReadRaw( f, &crc, 4 ) )
		{
			Warning( "Truncated GMA index entry: %s\n", gmaPath );
			fclose( f );
			return;
		}

		if ( e.size < 0 )
		{
			Warning( "Bad GMA entry size for '%s' in %s\n", e.name, gmaPath );
			fclose( f );
			return;
		}

		entries.AddToTail( e );
		if ( entries.Count() > GMAD_MAX_FILES )
		{
			Warning( "GMA has too many files (>%d): %s\n", GMAD_MAX_FILES, gmaPath );
			fclose( f );
			return;
		}
	}

	// Output base: cache/gma/<addonName>
	char baseName[MAX_PATH];
	const char *baseNamePtr = V_UnqualifiedFileName( gmaPath );
	Q_strncpy( baseName, baseNamePtr ? baseNamePtr : gmaPath, sizeof( baseName ) );
	char *dot = Q_strrchr( baseName, '.' );
	if ( dot )
		*dot = '\0';

	char relOutBase[MAX_PATH];
	Q_snprintf( relOutBase, sizeof( relOutBase ), "%s/gma/%s", cacheBase, baseName );
	Q_FixSlashes( relOutBase );
	filesystem->CreateDirHierarchy( relOutBase, "MOD" );

	// The file block starts right here, files stored in index order.
	char chunk[65536];
	FOR_EACH_VEC( entries, idx )
	{
		gma_entry_t &e = entries[idx];

		// Reject path traversal so a malformed/hostile addon can't escape cache.
		if ( Q_strstr( e.name, ".." ) )
		{
			Warning( "Skipping unsafe GMA path: %s\n", e.name );
			// Still must consume its bytes to stay aligned.
			long long skip = e.size;
			while ( skip > 0 )
			{
				size_t want = ( skip > (long long)sizeof( chunk ) ) ? sizeof( chunk ) : (size_t)skip;
				if ( fread( chunk, 1, want, f ) != want )
					break;
				skip -= (long long)want;
			}
			continue;
		}

		char relOutPath[MAX_PATH];
		Q_snprintf( relOutPath, sizeof( relOutPath ), "%s/%s", relOutBase, e.name );
		Q_FixSlashes( relOutPath );

		char dirBuf[MAX_PATH];
		Q_strncpy( dirBuf, relOutPath, sizeof( dirBuf ) );
		char *lastSlash = Q_strrchr( dirBuf, '/' );
		if ( lastSlash )
		{
			*lastSlash = '\0';
			filesystem->CreateDirHierarchy( dirBuf, "MOD" );
		}

		FileHandle_t ofh = g_pFullFileSystem->Open( relOutPath, "wb", "MOD" );
		if ( ofh == FILESYSTEM_INVALID_HANDLE )
			Warning( "Failed to open GMA output '%s'\n", relOutPath );

		long long remaining = e.size;
		while ( remaining > 0 )
		{
			size_t want = ( remaining > (long long)sizeof( chunk ) ) ? sizeof( chunk ) : (size_t)remaining;
			if ( fread( chunk, 1, want, f ) != want )
			{
				Warning( "Truncated GMA data for '%s' in %s\n", e.name, gmaPath );
				remaining = 0;
				if ( ofh != FILESYSTEM_INVALID_HANDLE )
					g_pFullFileSystem->Close( ofh );
				fclose( f );
				return;
			}
			if ( ofh != FILESYSTEM_INVALID_HANDLE )
				g_pFullFileSystem->Write( chunk, (int)want, ofh );
			remaining -= (long long)want;
		}

		if ( ofh != FILESYSTEM_INVALID_HANDLE )
			g_pFullFileSystem->Close( ofh );

		DevMsg( "Extracted GMA file: %s\n", relOutPath );
	}

	fclose( f );

	char absOutBase[MAX_PATH];
	Q_snprintf( absOutBase, sizeof( absOutBase ), "%s/%s", gamePath, relOutBase );
	Q_FixSlashes( absOutBase );

	// Mounting the addon root on MOD/GAME is enough: the entity/weapon scanner
	// globs "lua/entities/*" and "lua/weapons/*" across every MOD search path
	// (see luamanager.cpp), so a mounted addon's SWEPs/SENTs get registered too.
	filesystem->AddSearchPath( absOutBase, "MOD", PATH_ADD_TO_HEAD );
	filesystem->AddSearchPath( absOutBase, "GAME", PATH_ADD_TO_HEAD );

	DevMsg( "Mounted GMA addon: %s -> %s (%d files)\n", gmaPath, absOutBase, entries.Count() );

	// Record any SWEPs so they can be surfaced in the spawn menu.
	FOR_EACH_VEC( entries, wi )
		CollectAddonWeaponClass( entries[wi].name );
}

static void MountAddonFolder( const char *folder, const char *gamePath )
{
	FileFindHandle_t fh;
	char			 searchPath[MAX_PATH];
	Q_snprintf( searchPath, sizeof( searchPath ), "%s/*", folder );

	const char *fn = g_pFullFileSystem->FindFirstEx( searchPath, "MOD", &fh );
	while ( fn )
	{
		if ( fn[0] != '.' )
		{
			char fullPath[MAX_PATH];
			Q_snprintf( fullPath, sizeof( fullPath ), "%s/%s/%s", gamePath, folder, fn );
			Q_FixSlashes( fullPath );

			if ( g_pFullFileSystem->FindIsDirectory( fh ) )
			{
				DevMsg( "Mounting %s dir: %s\n", folder, fullPath );
				filesystem->AddSearchPath( fullPath, "MOD", PATH_ADD_TO_HEAD );
				filesystem->AddSearchPath( fullPath, "GAME", PATH_ADD_TO_HEAD );
			}
			else if ( Q_stristr( fn, ".vpk" ) )
			{
				DevMsg( "Mounting %s VPK: %s\n", folder, fullPath );
				filesystem->AddSearchPath( fullPath, "MOD", PATH_ADD_TO_HEAD );
				filesystem->AddSearchPath( fullPath, "GAME", PATH_ADD_TO_HEAD );
			}
			else if ( Q_stristr( fn, ".gma" ) )
			{
				DevMsg( "Mounting %s GMA: %s\n", folder, fullPath );
				ExtractGMA( fullPath, "cache", gamePath );
			}
			else if ( Q_stristr( fn, ".zip" ) )
			{
				DevMsg( "Mounting %s ZIP: %s\n", folder, fullPath );
				ExtractZIP( fullPath, "cache", gamePath );
			}
		}

		fn = g_pFullFileSystem->FindNext( fh );
	}

	g_pFullFileSystem->FindClose( fh );
}

void MountAddons()
{
	if ( CommandLine()->CheckParm( "-noaddons" ) )
	{
		DevMsg( "Addons mounting skipped due to -noaddons parameter.\n" );
		return;
	}

	char gamePath[MAX_PATH] = { 0 };
#ifdef CLIENT_DLL
	Q_strncpy( gamePath, engine->GetGameDirectory(), sizeof( gamePath ) );
#else
	engine->GetGameDir( gamePath, sizeof( gamePath ) );
#endif

	// NOO! I DON'T WANT TO DIE!
	filesystem->CreateDirHierarchy( "cache/zips", "MOD" );
	filesystem->CreateDirHierarchy( "cache/gma", "MOD" );

	filesystem->AddSearchPath( LUA_PATH_CACHE, "MOD", PATH_ADD_TO_HEAD );
	filesystem->AddSearchPath( LUA_PATH_CACHE, "GAME", PATH_ADD_TO_HEAD );

	g_AddonWeaponClasses.RemoveAll();

	MountAddonFolder( "addons", gamePath );
	MountAddonFolder( "custom", gamePath );
	MountAddonFolder( "mods", gamePath );

	// Surface any addon SWEPs we found in a generated spawn-menu tab.
	WriteAddonSpawnlist();
}

extern void lcf_recursivedeletefile( const char *current );

void UnMountAddons()
{
	lcf_recursivedeletefile( "cache/zips" );
	lcf_recursivedeletefile( "cache/gma" );
}