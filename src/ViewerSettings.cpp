//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           ViewerSettings.cpp
// last modified:  May 29 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#include "ViewerSettings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mx/mx.h>
#include <time.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include <mx/mxSettings.h>
#include "SpriteModel.h"
#include "sprviewer.h"

ViewerSettings g_viewerSettings;

void InitViewerSettings( void )
{
	memset (&g_viewerSettings, 0, sizeof (ViewerSettings));

	g_viewerSettings.renderMode = SPR_NORMAL;
	g_viewerSettings.orientType = SPR_FWD_PARALLEL;

	g_viewerSettings.bgColor[0] = 0.5f;
	g_viewerSettings.bgColor[1] = 0.5f;
	g_viewerSettings.bgColor[2] = 0.5f;

	g_viewerSettings.gColor[0] = 0.85f;
	g_viewerSettings.gColor[1] = 0.85f;
	g_viewerSettings.gColor[2] = 0.69f;

	g_viewerSettings.lColor[0] = 1.0f;
	g_viewerSettings.lColor[1] = 1.0f;
	g_viewerSettings.lColor[2] = 1.0f;

	g_viewerSettings.speedScale = 1.0f;
	g_viewerSettings.textureLimit = 256;
	g_viewerSettings.showGround = 0;
	g_viewerSettings.showMaximized = 0;
	g_viewerSettings.sequence_autoplay = true;

	// init random generator
	srand( (unsigned)time( NULL ) );
}

int LoadViewerSettings( void )
{
	InitViewerSettings ();

	mx_init_usersettings (g_org, g_appTitle);

	mx_get_usersettings_vec4d( "Background Color", g_viewerSettings.bgColor );
	mx_get_usersettings_vec4d( "Light Color", g_viewerSettings.lColor );
	mx_get_usersettings_vec4d( "Ground Color", g_viewerSettings.gColor );
	mx_get_usersettings_int( "Sequence AutoPlay", &g_viewerSettings.sequence_autoplay );
	mx_get_usersettings_int( "Show Ground", &g_viewerSettings.showGround );
	mx_get_usersettings_string( "Ground TexPath", g_viewerSettings.groundTexFile );
	mx_get_usersettings_int( "Show Maximized", &g_viewerSettings.showMaximized );

	return 1;
}

int SaveViewerSettings( void )
{
	if (mx_create_usersettings ())
	{
		mx_set_usersettings_vec4d( "Background Color", g_viewerSettings.bgColor );
		mx_set_usersettings_vec4d( "Light Color", g_viewerSettings.lColor );
		mx_set_usersettings_vec4d( "Ground Color", g_viewerSettings.gColor );
		mx_set_usersettings_int( "Sequence AutoPlay", g_viewerSettings.sequence_autoplay );
		mx_set_usersettings_int( "Show Ground", g_viewerSettings.showGround );
		mx_set_usersettings_string( "Ground TexPath", g_viewerSettings.groundTexFile );
		mx_set_usersettings_int( "Show Maximized", g_viewerSettings.showMaximized );

		return 1;
	}

	return 0;
}

bool IsSpriteModel( const char *path )
{
	byte	buffer[256];
	FILE	*fp;

	if( !path ) return false;

	// load the sprite
	if(( fp = fopen( path, "rb" )) == NULL )
		return false;

	fread( buffer, sizeof( buffer ), 1, fp );
	fclose( fp );

	if( ftell( fp ) < sizeof( 36 ))
		return false;

	// skip invalid signature
	if( strncmp((const char *)buffer, "IDSP", 4 ))
		return false;

	// skip unknown version
	if( *(int *)&buffer[4] != 1 && *(int *)&buffer[4] != 2 )
		return false;
	return true;
}

static bool ValidateSprite( const char *path )
{
	byte	buffer[256];
	dsprite_t	*phdr;
	FILE	*fp;

	if( !path ) return false;

	// load the sprite
	if(( fp = fopen( path, "rb" )) == NULL )
		return false;

	fread( buffer, sizeof( buffer ), 1, fp );
	fclose( fp );

	if( ftell( fp ) < sizeof( dsprite_t ))
		return false;

	// skip invalid signature
	if( strncmp((const char *)buffer, "IDSP", 4 ))
		return false;

	phdr = (dsprite_t *)buffer;

	// skip unknown version
	if( phdr->version != SPRITE_VERSION_Q1 && phdr->version != SPRITE_VERSION_HL )
		return false;

	return true;
}

static void AddPathToList( const char *path )
{
	char	spritePath[256];

	if( g_viewerSettings.numSpritePathes >= 2048 )
		return; // too many strings

	mx_snprintf( spritePath, sizeof( spritePath ), "%s%s", g_viewerSettings.oldSpritePath, path );

	if( !ValidateSprite( spritePath ))
		return;

	int i = g_viewerSettings.numSpritePathes++;

	strncpy( g_viewerSettings.spritePathList[i], spritePath, sizeof( g_viewerSettings.spritePathList[0] ) - 1 );
}

static void SortPathList( void )
{
	char	temp[256] = {0};
	int	i, j;

	// this is a selection sort (finds the best entry for each slot)
	for( i = 0; i < g_viewerSettings.numSpritePathes - 1; i++ )
	{
		for( j = i + 1; j < g_viewerSettings.numSpritePathes; j++ )
		{
			if( strcmp( g_viewerSettings.spritePathList[i], g_viewerSettings.spritePathList[j] ) > 0 )
			{
				strncpy( temp, g_viewerSettings.spritePathList[i], sizeof( temp ) - 1 );
				strncpy( g_viewerSettings.spritePathList[i], g_viewerSettings.spritePathList[j], sizeof( temp ) - 1 );
				strncpy( g_viewerSettings.spritePathList[j], temp, sizeof( temp ) - 1 );
			}
		}
	}
}

void ListDirectory( void )
{
	char spritePath[256];

	strcpy( spritePath, mx_getpath( g_viewerSettings.spritePath ) );

	if( !mx_strcasecmp( spritePath, g_viewerSettings.oldSpritePath ) )
		return;	// not changed

	strncpy( g_viewerSettings.oldSpritePath, spritePath, sizeof( g_viewerSettings.oldSpritePath ) - 1 );
	g_viewerSettings.numSpritePathes = 0;

#ifdef _WIN32
	WIN32_FIND_DATA n_file;
	HANDLE hFile;

	strncat( spritePath, "*.spr", sizeof( spritePath ) - strlen( spritePath ) - 1 );

	// ask for the directory listing handle
	hFile = FindFirstFileA(spritePath, &n_file);

	if( hFile == INVALID_HANDLE_VALUE ) return; // how this possible?

	// iterate through the directory
	do {
		AddPathToList( n_file.cFileName );
	} while( FindNextFileA( hFile, &n_file ) != 0 );

	FindClose( hFile );
#else
	DIR *dfd;
	struct dirent *entry;

	// ask for the directory listing handle
	dfd = opendir( spritePath );

	// iterate through the directory
	while((entry = readdir(dfd))) {
		AddPathToList(entry->d_name);
	}

	closedir(dfd);
#endif

	SortPathList();
}

const char *LoadNextSprite( void )
{
	int	i;

	for( i = 0; i < g_viewerSettings.numSpritePathes; i++ )
	{
		if( !mx_strcasecmp( g_viewerSettings.spritePathList[i], g_viewerSettings.spritePath ))
		{
			i++;
			break;
		}
	}

	if( i == g_viewerSettings.numSpritePathes )
		i = 0;
	return g_viewerSettings.spritePathList[i];
}

const char *LoadPrevSprite( void )
{
	int	i;

	for( i = 0; i < g_viewerSettings.numSpritePathes; i++ )
	{
		if( !mx_strcasecmp( g_viewerSettings.spritePathList[i], g_viewerSettings.spritePath ))
		{
			i--;
			break;
		}
	}

	if( i < 0 ) i = g_viewerSettings.numSpritePathes - 1;
	return g_viewerSettings.spritePathList[i];
}
