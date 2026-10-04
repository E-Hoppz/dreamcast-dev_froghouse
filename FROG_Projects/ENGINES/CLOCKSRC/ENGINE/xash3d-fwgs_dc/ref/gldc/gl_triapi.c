/*
gl_triapi.c - TriAPI draw methods
Copyright (C) 2011 Uncle Mike
Copyright (C) 2019 a1batross

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
#include "const.h"

static struct
{
	int		renderMode;		// override kRenderMode from TriAPI
	vec4_t		triRGBA;
} ds;

/*
===============================================================

  TRIAPI IMPLEMENTATION

===============================================================
*/
/*
=============
TriRenderMode

set rendermode
=============
*/
void _TriRenderMode( int mode )
{
	ds.renderMode = mode;
	switch( mode )
	{
	case kRenderNormal:
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glDisable( GL_BLEND );
		glDepthMask( GL_TRUE );
		break;
	case kRenderTransAlpha:
		glEnable( GL_BLEND );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDepthMask( GL_FALSE );
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		break;
	case kRenderGlow:
	case kRenderTransAdd:
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		glEnable( GL_BLEND );
		glDepthMask( GL_FALSE );
		break;
	}
}

/*
=============
TriBegin

begin triangle sequence
=============
*/
void TriBegin( int mode )
{
	switch( mode )
	{
	case TRI_POINTS:
		mode = GL_POINTS;
		break;
	case TRI_TRIANGLES:
		mode = GL_TRIANGLES;
		break;
	case TRI_TRIANGLE_FAN:
		mode = GL_TRIANGLE_FAN;
		break;
	case TRI_QUADS:
		mode = GL_QUADS;
		break;
	case TRI_LINES:
		mode = GL_LINES;
		break;
	case TRI_TRIANGLE_STRIP:
		mode = GL_TRIANGLE_STRIP;
		break;
	case TRI_QUAD_STRIP:
		mode = GL_QUAD_STRIP;
		break;
	case TRI_POLYGON:
	default:
		mode = GL_POLYGON;
		break;
	}

	glBegin( mode );
}

/*
=============
TriEnd

draw triangle sequence
=============
*/
void TriEnd( void )
{
	glEnd( );
}

/*
=============
_TriColor4f

=============
*/
void _TriColor4f( float r, float g, float b, float a )
{
	glColor4f( r, g, b, a );
}

/*
=============
_TriColor4f

=============
*/
void _TriColor4ub( byte r, byte g, byte b, byte a )
{
	glColor4ub( r, g, b, a );
}


/*
=============
TriColor4ub

=============
*/
void TriColor4ub( byte r, byte g, byte b, byte a )
{
	ds.triRGBA[0] = r * (1.0f / 255.0f);
	ds.triRGBA[1] = g * (1.0f / 255.0f);
	ds.triRGBA[2] = b * (1.0f / 255.0f);
	ds.triRGBA[3] = a * (1.0f / 255.0f);

	_TriColor4f( ds.triRGBA[0], ds.triRGBA[1], ds.triRGBA[2], 1.0f );
}

/*
=================
TriColor4f
=================
*/
void TriColor4f( float r, float g, float b, float a )
{
	if( ds.renderMode == kRenderTransAlpha )
		TriColor4ub( r * 255.9f, g * 255.9f, b * 255.9f, a * 255.0f );
	else _TriColor4f( r * a, g * a, b * a, 1.0 );

	ds.triRGBA[0] = r;
	ds.triRGBA[1] = g;
	ds.triRGBA[2] = b;
	ds.triRGBA[3] = a;
}

/*
=============
TriTexCoord2f

=============
*/
void TriTexCoord2f( float u, float v )
{
	glTexCoord2f( u, v );
}

/*
=============
TriVertex3fv

=============
*/
void TriVertex3fv( const float *v )
{
	glVertex3fv( v );
}

/*
=============
TriVertex3f

=============
*/
void TriVertex3f( float x, float y, float z )
{
	glVertex3f( x, y, z );
}

/*
=============
TriWorldToScreen

convert world coordinates (x,y,z) into screen (x, y)
=============
*/
int TriWorldToScreen( const float *world, float *screen )
{
	int	retval;

	retval = R_WorldToScreen( world, screen );

	screen[0] =  0.5f * screen[0] * (float)RI.viewport[2];
	screen[1] = -0.5f * screen[1] * (float)RI.viewport[3];
	screen[0] += 0.5f * (float)RI.viewport[2];
	screen[1] += 0.5f * (float)RI.viewport[3];

	return retval;
}

/*
=============
TriSpriteTexture

bind current texture
=============
*/
int TriSpriteTexture( model_t *pSpriteModel, int frame )
{
	int	gl_texturenum;

	if(( gl_texturenum = R_GetSpriteTexture( pSpriteModel, frame )) == 0 )
		return 0;

	if( gl_texturenum <= 0 || gl_texturenum >= MAX_TEXTURES )
		gl_texturenum = tr.defaultTexture;

	GL_Bind( XASH_TEXTURE0, gl_texturenum );

	return 1;
}

/*
=============
TriFog

enables global fog on the level
=============
*/
void TriFog( float flFogColor[3], float flStart, float flEnd, int bOn )
{
#if !XASH_DREAMCAST
	// overrided by internal fog
	if( RI.fogEnabled ) return;
	RI.fogCustom = bOn;

	// check for invalid parms
	if( flEnd <= flStart )
	{
		glState.isFogEnabled = RI.fogCustom = false;
		glDisable( GL_FOG );
		return;
	}

	if( RI.fogCustom )
		glEnable( GL_FOG );
	else glDisable( GL_FOG );

	// copy fog params
	RI.fogColor[0] = flFogColor[0] / 255.0f;
	RI.fogColor[1] = flFogColor[1] / 255.0f;
	RI.fogColor[2] = flFogColor[2] / 255.0f;
	RI.fogColor[3] = 1.0f;

	RI.fogStart = flStart;
	RI.fogEnd = flEnd;

	if( RI.fogDensity > 0.0f )
	{
		glFogi( GL_FOG_MODE, GL_EXP2 );
		glFogf( GL_FOG_DENSITY, RI.fogDensity );
	}
	else
	{
		glFogi( GL_FOG_MODE, GL_LINEAR );
		RI.fogSkybox = true;
	}

	glFogfv( GL_FOG_COLOR, RI.fogColor );
	glFogf( GL_FOG_START, RI.fogStart );
	glFogf( GL_FOG_END, RI.fogEnd );

	glHint( GL_FOG_HINT, GL_NICEST );
#endif
}

/*
=============
TriGetMatrix

very strange export
=============
*/
void TriGetMatrix( const int pname, float *matrix )
{
	glGetFloatv( pname, matrix );
}

/*
=============
TriForParams

=============
*/
void TriFogParams( float flDensity, int iFogSkybox )
{
	RI.fogDensity = flDensity;
	RI.fogSkybox = iFogSkybox;
}

/*
=============
TriCullFace

=============
*/
void TriCullFace( TRICULLSTYLE mode )
{
	int glMode;

	switch( mode )
	{
	case TRI_FRONT:
		glMode = GL_FRONT;
		break;
	default:
		glMode = GL_NONE;
		break;
	}

	GL_Cull( glMode );
}

/*
=============
TriBrightness
=============
*/
void TriBrightness( float brightness )
{
	float	r, g, b;

	r = ds.triRGBA[0] * ds.triRGBA[3] * brightness;
	g = ds.triRGBA[1] * ds.triRGBA[3] * brightness;
	b = ds.triRGBA[2] * ds.triRGBA[3] * brightness;

	_TriColor4f( r, g, b, 1.0f );
}

