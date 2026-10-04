/*
gl_backend.c - rendering backend
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include "gl_local.h"
#include "xash3d_mathlib.h"

char		r_speeds_msg[MAX_SYSPATH];
ref_speeds_t	r_stats;	// r_speeds counters

/*
===============
R_SpeedsMessage
===============
*/
qboolean R_SpeedsMessage( char *out, size_t size )
{
	if( gEngfuncs_gl.drawFuncs->R_SpeedsMessage != NULL )
	{
		if( gEngfuncs_gl.drawFuncs->R_SpeedsMessage( out, size ))
			return true;
		// otherwise pass to default handler
	}

	if( r_speeds->value <= 0 ) return false;
	if( !out || !size ) return false;

	Q_strncpy( out, r_speeds_msg, size );

	return true;
}

/*
==============
R_Speeds_Printf

helper to print into r_speeds message
==============
*/
static void R_Speeds_Printf( const char *msg, ... )
{
	va_list	argptr;
	char	text[2048];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	Q_strncat( r_speeds_msg, text, sizeof( r_speeds_msg ));
}

/*
==============
GL_BackendStartFrame
==============
*/
void GL_BackendStartFrame( void )
{
	r_speeds_msg[0] = '\0';
}

/*
==============
GL_BackendEndFrame
==============
*/
void GL_BackendEndFrame( void )
{
	mleaf_t	*curleaf;

	if( r_speeds->value <= 0 || !RI.drawWorld )
		return;

	if( !RI.viewleaf )
		curleaf = WORLDMODEL->leafs;
	else curleaf = RI.viewleaf;

	R_Speeds_Printf( "Renderer: ^1Engine^7\n\n" );

	switch( (int)r_speeds->value )
	{
	case 1:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i wpoly, %3i apoly\n%3i epoly, %3i spoly",
			r_stats.c_world_polys, r_stats.c_alias_polys, r_stats.c_studio_polys, r_stats.c_sprite_polys );
		break;
	case 2:
		R_Speeds_Printf( "visible leafs:\n%3i leafs\ncurrent leaf %3i\n", r_stats.c_world_leafs, curleaf - WORLDMODEL->leafs );
		R_Speeds_Printf( "ReciusiveWorldNode: %3lf secs\nDrawTextureChains %lf\n", r_stats.t_world_node, r_stats.t_world_draw );
		break;
	case 3:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i alias models drawn\n%3i studio models drawn\n%3i sprites drawn",
			r_stats.c_alias_models_drawn, r_stats.c_studio_models_drawn, r_stats.c_sprite_models_drawn );
		break;
	case 4:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i static entities\n%3i normal entities\n%3i server entities",
			r_numStatics, r_numEntities - r_numStatics, (int)ENGINE_GET_PARM( PARM_NUMENTITIES ));
		break;
	case 5:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i tempents\n%3i viewbeams\n%3i particles",
			r_stats.c_active_tents_count, r_stats.c_view_beams_count, r_stats.c_particle_count );
		break;
	}

	memset( &r_stats, 0, sizeof( r_stats ));
}

/*
=================
GL_LoadTexMatrixExt
=================
*/
void GL_LoadTexMatrixExt( const float *glmatrix )
{
	Assert( glmatrix != NULL );
	glMatrixMode( GL_TEXTURE );
	glLoadMatrixf( glmatrix );
	glState.texIdentityMatrix[glState.activeTMU] = false;
}

/*
=================
GL_LoadMatrix
=================
*/
void GL_LoadMatrix( const matrix4x4 source )
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	glLoadMatrixf( dest );
}

/*
=================
GL_LoadIdentityTexMatrix
=================
*/
void GL_LoadIdentityTexMatrix( void )
{
	if( glState.texIdentityMatrix[glState.activeTMU] )
		return;

	glMatrixMode( GL_TEXTURE );
	glLoadIdentity();
	glState.texIdentityMatrix[glState.activeTMU] = true;
}

/*
=================
GL_SelectTexture
=================
*/
void GL_SelectTexture( GLint tmu )
{
	if( !GL_Support( GL_ARB_MULTITEXTURE ))
		return;

	// don't allow negative texture units
	if( tmu < 0 ) return;

	if( tmu >= GL_MaxTextureUnits( ))
	{
		gEngfuncs_gl.Con_Reportf( S_ERROR "%s: bad tmu state %i\n", __func__, tmu );
		return;
	}

	if( glState.activeTMU == tmu )
		return;

	glState.activeTMU = tmu;
#if !XASH_DREAMCAST
	if( glActiveTextureARB )
	{
		glActiveTextureARB( tmu + GL_TEXTURE0_ARB );

		if( tmu < glConfig.max_texture_coords )
			glClientActiveTextureARB( tmu + GL_TEXTURE0_ARB );
	}
#endif
}

/*
==============
GL_DisableAllTexGens
==============
*/
void GL_DisableAllTexGens( void )
{
	// not implemented
}

/*
==============
GL_CleanUpTextureUnits
==============
*/
void GL_CleanUpTextureUnits( int last )
{
	int	i;

	for( i = glState.activeTMU; i > (last - 1); i-- )
	{
		// disable upper units
		if( glState.currentTextureTargets[i] != GL_NONE )
		{
			glDisable( glState.currentTextureTargets[i] );
			glState.currentTextureTargets[i] = GL_NONE;
			glState.currentTextures[i] = -1; // unbind texture
			glState.currentTexturesIndex[i] = 0;
		}

		GL_SetTexCoordArrayMode( GL_NONE );
		GL_LoadIdentityTexMatrix();
		GL_DisableAllTexGens();
		GL_SelectTexture( i - 1 );
	}
}

/*
==============
GL_CleanupAllTextureUnits
==============
*/
void GL_CleanupAllTextureUnits( void )
{
#if !XASH_DREAMCAST
	if( !glw_state.initialized ) return;
#endif
	// force to cleanup all the units
	GL_SelectTexture( GL_MaxTextureUnits() - 1 );
	GL_CleanUpTextureUnits( 0 );
}

/*
=================
GL_MultiTexCoord2f
=================
*/
void GL_MultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
	if( !GL_Support( GL_ARB_MULTITEXTURE ))
		return;
#if !XASH_DREAMCAST
#ifndef XASH_GL_STATIC
	if( glMultiTexCoord2f != NULL )
#endif
		glMultiTexCoord2f( texture + GL_TEXTURE0_ARB, s, t );
#endif

}

/*
====================
GL_EnableTextureUnit
====================
*/
void GL_EnableTextureUnit( int tmu, qboolean enable )
{
	// only enable fixed-function pipeline units
	if( tmu < glConfig.max_texture_units )
	{
		if( enable )
		{
			glEnable( glState.currentTextureTargets[tmu] );
		}
		else if( glState.currentTextureTargets[tmu] != GL_NONE )
		{
			glDisable( glState.currentTextureTargets[tmu] );
		}
	}
}

/*
=================
GL_TextureTarget
=================
*/
void GL_TextureTarget( uint target )
{
	if( glState.activeTMU < 0 || glState.activeTMU >= GL_MaxTextureUnits( ))
	{
		gEngfuncs_gl.Con_Reportf( S_ERROR "%s: bad tmu state %i\n", __func__, glState.activeTMU );
		return;
	}

	if( glState.currentTextureTargets[glState.activeTMU] != target )
	{
		GL_EnableTextureUnit( glState.activeTMU, false );
		glState.currentTextureTargets[glState.activeTMU] = target;
		if( target != GL_NONE )
			GL_EnableTextureUnit( glState.activeTMU, true );
	}
}

/*
=================
GL_TexGen
=================
*/
void GL_TexGen( GLenum coord, GLenum mode )
{
#if !XASH_DREAMCAST
	int	tmu = Q_min( glConfig.max_texture_coords, glState.activeTMU );
	int	bit, gen;

	switch( coord )
	{
	case GL_S:
		bit = 1;
		gen = GL_TEXTURE_GEN_S;
		break;
	case GL_T:
		bit = 2;
		gen = GL_TEXTURE_GEN_T;
		break;
	case GL_R:
		bit = 4;
		gen = GL_TEXTURE_GEN_R;
		break;
	case GL_Q:
		bit = 8;
		gen = GL_TEXTURE_GEN_Q;
		break;
	default: return;
	}

	if( mode )
	{
		if( !( glState.genSTEnabled[tmu] & bit ))
		{
			glEnable( gen );
			glState.genSTEnabled[tmu] |= bit;
		}
		glTexGeni( coord, GL_TEXTURE_GEN_MODE, mode );
	}
	else
	{
		if( glState.genSTEnabled[tmu] & bit )
		{
			glDisable( gen );
			glState.genSTEnabled[tmu] &= ~bit;
		}
	}
#endif
}

/*
=================
GL_SetTexCoordArrayMode
=================
*/
void GL_SetTexCoordArrayMode( GLenum mode )
{
	int	tmu = Q_min( glConfig.max_texture_coords, glState.activeTMU );
	int	bit, cmode = glState.texCoordArrayMode[tmu];

	if( mode == GL_TEXTURE_COORD_ARRAY )
		bit = 1;
	else if( mode == GL_TEXTURE_CUBE_MAP_ARB )
		bit = 2;
	else bit = 0;

	if( cmode != bit )
	{
#if 0
		/*p*/glDisable( GL_TEXTURE_CUBE_MAP_ARB );
		/*p*/glEnable( GL_TEXTURE_CUBE_MAP_ARB );
#else
		if( cmode == 1 ) glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		else if( cmode == 2 ) glDisable( GL_TEXTURE_CUBE_MAP_ARB );

		if( bit == 1 ) glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		else if( bit == 2 ) glEnable( GL_TEXTURE_CUBE_MAP_ARB );
#endif
		glState.texCoordArrayMode[tmu] = bit;
	}
}

/*
=================
GL_Cull
=================
*/
void GL_Cull( GLenum cull )
{
	if( !cull )
	{
		glDisable( GL_CULL_FACE );
		glState.faceCull = 0;
		return;
	}

	glEnable( GL_CULL_FACE );
	glCullFace( cull );
	glState.faceCull = cull;
}

void GL_SetRenderMode( int mode )
{
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	switch( mode )
	{
	case kRenderNormal:
	default:
		glDisable( GL_BLEND );
		glDisable( GL_ALPHA_TEST );
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
		glEnable( GL_BLEND );
		glDisable( GL_ALPHA_TEST );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		break;
	case kRenderTransAlpha:
		glDisable( GL_BLEND );
		glEnable( GL_ALPHA_TEST );
		break;
	case kRenderGlow:
	case kRenderTransAdd:
		glEnable( GL_BLEND );
		glDisable( GL_ALPHA_TEST );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		break;
	case kRenderScreenFadeModulate:
		glEnable( GL_BLEND );
		glDisable( GL_ALPHA_TEST );
		glBlendFunc( GL_ZERO, GL_SRC_COLOR );
	}
}

/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/
// used for 'env' and 'sky' shots
typedef struct envmap_s
{
	vec3_t	angles;
	int	flags;
} envmap_t;

const envmap_t r_skyBoxInfo[6] =
{
{{   0, 270, 180}, IMAGE_FLIP_X },
{{   0,  90, 180}, IMAGE_FLIP_X },
{{ -90,   0, 180}, IMAGE_FLIP_X },
{{  90,   0, 180}, IMAGE_FLIP_X },
{{   0,   0, 180}, IMAGE_FLIP_X },
{{   0, 180, 180}, IMAGE_FLIP_X },
};

const envmap_t r_envMapInfo[6] =
{
{{  0,   0,  90}, 0 },
{{  0, 180, -90}, 0 },
{{  0,  90,   0}, 0 },
{{  0, 270, 180}, 0 },
{{-90, 180, -90}, 0 },
{{ 90,   0,  90}, 0 }
};

qboolean VID_ScreenShot( const char *filename, int shot_type )
{
	rgbdata_t *r_shot;
	uint	flags = IMAGE_FLIP_Y;
	int	width = 0, height = 0;
	qboolean	result;

	r_shot = Mem_Calloc( r_temppool, sizeof( rgbdata_t ));
	r_shot->width = (gpGlobals_gl->width + 3) & ~3;
	r_shot->height = (gpGlobals_gl->height + 3) & ~3;
	r_shot->flags = IMAGE_HAS_COLOR;
	r_shot->type = PF_RGBA_32;
	r_shot->size = r_shot->width * r_shot->height * gEngfuncs_gl.Image_GetPFDesc( r_shot->type )->bpp;
	r_shot->palette = NULL;
	r_shot->buffer = Mem_Malloc( r_temppool, r_shot->size );

	// get screen frame
	glReadPixels( 0, 0, r_shot->width, r_shot->height, GL_RGBA, GL_UNSIGNED_BYTE, r_shot->buffer );

	switch( shot_type )
	{
	case VID_SCREENSHOT:
		break;
	case VID_SNAPSHOT:
		gEngfuncs_gl.fsapi->AllowDirectPaths( true );
		break;
	case VID_LEVELSHOT:
	case VID_MINISHOT:
		flags |= IMAGE_RESAMPLE;
		height = shot_type == VID_MINISHOT ? 200 : 480;
		width = Q_rint( height * ((double)r_shot->width / r_shot->height ));
		break;
	case VID_MAPSHOT:
		flags |= IMAGE_RESAMPLE|IMAGE_QUANTIZE;	// GoldSrc request overviews in 8-bit format
		height = 768;
		width = 1024;
		break;
	}

	gEngfuncs_gl.Image_Process( &r_shot, width, height, flags, 0.0f );

	// write image
	result = gEngfuncs_gl.FS_SaveImage( filename, r_shot );
	gEngfuncs_gl.fsapi->AllowDirectPaths( false );			// always reset after store screenshot
	gEngfuncs_gl.FS_FreeImage( r_shot );

	return result;
}

/*
=================
VID_CubemapShot
=================
*/
qboolean VID_CubemapShot( const char *base, uint size, const float *vieworg, qboolean skyshot )
{
	rgbdata_t		*r_shot, *r_side;
	byte		*temp = NULL;
	byte		*buffer = NULL;
	string		basename;
	int		i = 1, flags, result;

	if( !RI.drawWorld || !WORLDMODEL )
		return false;

	// make sure the specified size is valid
	while( i < size ) i<<=1;

	if( i != size ) return false;
	if( size > gpGlobals_gl->width || size > gpGlobals_gl->height )
		return false;

	// alloc space
	temp = Mem_Malloc( r_temppool, size * size * 3 );
	buffer = Mem_Malloc( r_temppool, size * size * 3 * 6 );
	r_shot = Mem_Calloc( r_temppool, sizeof( rgbdata_t ));
	r_side = Mem_Calloc( r_temppool, sizeof( rgbdata_t ));

	// use client vieworg
	if( !vieworg ) vieworg = RI.vieworg;

	for( i = 0; i < 6; i++ )
	{
		// go into 3d mode
		R_Set2DMode( false );

		if( skyshot )
		{
			R_DrawCubemapView( vieworg, r_skyBoxInfo[i].angles, size );
			flags = r_skyBoxInfo[i].flags;
		}
		else
		{
			R_DrawCubemapView( vieworg, r_envMapInfo[i].angles, size );
			flags = r_envMapInfo[i].flags;
		}

		glReadPixels( 0, 0, size, size, GL_RGB, GL_UNSIGNED_BYTE, temp );
		r_side->flags = IMAGE_HAS_COLOR;
		r_side->width = r_side->height = size;
		r_side->type = PF_RGB_24;
		r_side->size = r_side->width * r_side->height * 3;
		r_side->buffer = temp;

		if( flags ) gEngfuncs_gl.Image_Process( &r_side, 0, 0, flags, 0.0f );
		memcpy( buffer + (size * size * 3 * i), r_side->buffer, size * size * 3 );
	}

	r_shot->flags = IMAGE_HAS_COLOR;
	r_shot->flags |= (skyshot) ? IMAGE_SKYBOX : IMAGE_CUBEMAP;
	r_shot->width = size;
	r_shot->height = size;
	r_shot->type = PF_RGB_24;
	r_shot->size = r_shot->width * r_shot->height * 3 * 6;
	r_shot->palette = NULL;
	r_shot->buffer = buffer;

	// make sure what we have right extension
	Q_strncpy( basename, base, sizeof( basename ));
	COM_ReplaceExtension( basename, ".tga", sizeof( basename ));

	// write image as 6 sides
	result = gEngfuncs_gl.FS_SaveImage( basename, r_shot );
	gEngfuncs_gl.FS_FreeImage( r_shot );
	gEngfuncs_gl.FS_FreeImage( r_side );

	return result;
}

//=======================================================

/*
===============
R_ShowTextures

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void R_ShowTextures( void )
{
#if !XASH_DREAMCAST
	gl_texture_t	*image;
	float		x, y, w, h;
	int		total, start, end;
	int		i, j, k, base_w, base_h;
	rgba_t		color = { 192, 192, 192, 255 };
	int		charHeight, numTries = 0;
	static qboolean	showHelp = true;
	string		shortname;

	if( !r_showtextures->value )
		return;

	if( showHelp )
	{
		gEngfuncs_gl.CL_CenterPrint( "use '<-' and '->' keys to change atlas page, ESC to quit", 0.25f );
		showHelp = false;
	}

	GL_SetRenderMode( kRenderNormal );
	glClear( GL_COLOR_BUFFER_BIT );
	glFinish();

	base_w = 8;	// textures view by horizontal
	base_h = 6;	// textures view by vertical

rebuild_page:
	total = base_w * base_h;
	start = total * (r_showtextures->value - 1);
	end = total * r_showtextures->value;
	if( end > MAX_TEXTURES ) end = MAX_TEXTURES;

	w = gpGlobals_gl->width / base_w;
	h = gpGlobals_gl->height / base_h;

	gEngfuncs_gl.Con_DrawStringLen( NULL, NULL, &charHeight );

	for( i = j = 0; i < MAX_TEXTURES; i++ )
	{
		image = R_GetTexture( i );
		if( j == start ) break; // found start
		if( glIsTexture( image->texnum )) j++;
	}

	if( i == MAX_TEXTURES && r_showtextures->value != 1 )
	{
		// bad case, rewind to one and try again
		gEngfuncs_gl.Cvar_SetValue( "r_showtextures", Q_max( 1, r_showtextures->value - 1 ));
		if( ++numTries < 2 ) goto rebuild_page;	// to prevent infinite loop
	}

	for( k = 0; i < MAX_TEXTURES; i++ )
	{
		if( j == end ) break; // page is full

		image = R_GetTexture( i );
		if( !glIsTexture( image->texnum ))
			continue;

		x = k % base_w * w;
		y = k / base_w * h;

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		GL_Bind( XASH_TEXTURE0, i ); // NOTE: don't use image->texnum here, because skybox has a 'wrong' indexes

		if( FBitSet( image->flags, TF_DEPTHMAP ) && !FBitSet( image->flags, TF_NOCOMPARE ))
			glTexParameteri( image->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE );

		glBegin( GL_QUADS );
		glTexCoord2f( 0, 0 );
		glVertex2f( x, y );
		if( image->target == GL_TEXTURE_RECTANGLE_EXT )
			glTexCoord2f( image->width, 0 );
		else glTexCoord2f( 1, 0 );
		glVertex2f( x + w, y );
		if( image->target == GL_TEXTURE_RECTANGLE_EXT )
			glTexCoord2f( image->width, image->height );
		else glTexCoord2f( 1, 1 );
		glVertex2f( x + w, y + h );
		if( image->target == GL_TEXTURE_RECTANGLE_EXT )
			glTexCoord2f( 0, image->height );
		else glTexCoord2f( 0, 1 );
		glVertex2f( x, y + h );
		glEnd();

		if( FBitSet( image->flags, TF_DEPTHMAP ) && !FBitSet( image->flags, TF_NOCOMPARE ))
			glTexParameteri( image->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );

		COM_FileBase( image->name, shortname, sizeof( shortname ));
		if( Q_strlen( shortname ) > 18 )
		{
			// cutoff too long names, it looks ugly
			shortname[16] = '.';
			shortname[17] = '.';
			shortname[18] = '\0';
		}
		gEngfuncs_gl.Con_DrawString( x + 1, y + h - charHeight, shortname, color );
		j++, k++;
	}

	gEngfuncs_gl.CL_DrawCenterPrint ();
	glFinish();

#endif
}



/*
================
SCR_TimeRefresh_f

timerefresh [noflip]
================
*/
void SCR_TimeRefresh_f( void )
{
	int	i;
	double	start, stop;
	double	time;

	if( ENGINE_GET_PARM( PARM_CONNSTATE ) != ca_active )
		return;

	start = gEngfuncs_gl.pfnTime();

	// run without page flipping like GoldSrc
	if( gEngfuncs_gl.Cmd_Argc() == 1 )
	{
		glDrawBuffer( GL_FRONT );
		for( i = 0; i < 128; i++ )
		{
			gpGlobals_gl->viewangles[1] = i / 128.0f * 360.0f;
			R_RenderScene();
		}
		glFinish();
		R_EndFrame();
	}
	else
	{
		for( i = 0; i < 128; i++ )
		{
			R_BeginFrame( true );
			gpGlobals_gl->viewangles[1] = i / 128.0f * 360.0f;
			R_RenderScene();
			R_EndFrame();
		}
	}

	stop = gEngfuncs_gl.pfnTime ();
	time = (stop - start);
	gEngfuncs_gl.Con_Printf( "%f seconds (%f fps)\n", time, 128 / time );
}
