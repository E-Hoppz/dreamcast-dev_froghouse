/*
gl_draw.c - orthogonal drawing stuff
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

/*
=============
R_GetImageParms
=============
*/
void _R_GetTextureParms( int *w, int *h, int texnum )
{
	gl_texture_t	*glt;

	glt = R_GetTexture( texnum );
	if( w ) *w = glt->srcWidth;
	if( h ) *h = glt->srcHeight;
}

/*
=============
R_GetSpriteParms

same as GetImageParms but used
for sprite models
=============
*/
void R_GetSpriteParms( int *frameWidth, int *frameHeight, int *numFrames, int currentFrame, const model_t *pSprite )
{
	mspriteframe_t	*pFrame;

	if( !pSprite || pSprite->type != mod_sprite ) return; // bad model ?
	pFrame = R_GetSpriteFrame( pSprite, currentFrame, 0.0f );

	if( frameWidth ) *frameWidth = pFrame->width;
	if( frameHeight ) *frameHeight = pFrame->height;
	if( numFrames ) *numFrames = pSprite->numframes;
}

int R_GetSpriteTexture( const model_t *m_pSpriteModel, int frame )
{
	if( !m_pSpriteModel || m_pSpriteModel->type != mod_sprite || !m_pSpriteModel->cache.data )
		return 0;

	return R_GetSpriteFrame( m_pSpriteModel, frame, 0.0f )->gl_texturenum;
}

/*
=============
R_DrawStretchPic
=============
*/
void R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int texnum )
{
	GL_Bind( XASH_TEXTURE0, texnum );

	glBegin( GL_QUADS );
		glTexCoord2f( s1, t1 );
		glVertex2f( x, y );

		glTexCoord2f( s2, t1 );
		glVertex2f( x + w, y );

		glTexCoord2f( s2, t2 );
		glVertex2f( x + w, y + h );

		glTexCoord2f( s1, t2 );
		glVertex2f( x, y + h );
	glEnd();
}

/*
=============
R_DrawStretchRaw
=============
*/
void R_DrawStretchRaw( float x, float y, float w, float h, int cols, int rows, const byte *data, qboolean dirty )
{

	byte		*raw = NULL;
	gl_texture_t	*tex;

	
		raw = (byte *)data;

	if( cols > glConfig.max_2d_texture_size )
		gEngfuncs_gl.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, cols );
	if( rows > glConfig.max_2d_texture_size )
		gEngfuncs_gl.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, rows );

	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	tex = R_GetTexture( tr.cinTexture );
	GL_Bind( XASH_TEXTURE0, tr.cinTexture );

	if( cols == tex->width && rows == tex->height )
	{
		if( dirty )
		{	
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_BGRA, GL_UNSIGNED_BYTE, raw );
		}
	}
	else
	{
		tex->size = cols * rows * 4;
		tex->width = cols;
		tex->height = rows;
		if( dirty )
		{	
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, cols, rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, raw );
		}
	}

	glBegin( GL_QUADS );
	glTexCoord2f( 0, 0 );
	glVertex2f( x, y );
	glTexCoord2f( 1, 0 );
	glVertex2f( x + w, y );
	glTexCoord2f( 1, 1 );
	glVertex2f( x + w, y + h );
	glTexCoord2f( 0, 1 );
	glVertex2f( x, y + h );
	glEnd();

}

/*
=============
R_UploadStretchRaw
=============
*/
void R_UploadStretchRaw( int texture, int cols, int rows, int width, int height, const byte *data )
{

	byte		*raw = NULL;
	gl_texture_t	*tex;

	

		raw = (byte *)data;
	
	if( cols > glConfig.max_2d_texture_size )
		gEngfuncs_gl.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, cols );
	if( rows > glConfig.max_2d_texture_size )
		gEngfuncs_gl.Host_Error( "%s: size %i exceeds hardware limits\n", __func__, rows );

	tex = R_GetTexture( texture );
	GL_Bind( GL_KEEP_UNIT, texture );
	tex->width = cols;
	tex->height = rows;
	glTexImage2D( GL_TEXTURE_2D, 0, tex->format, cols, rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, raw );
	GL_ApplyTextureParams( tex );

}

/*
===============
R_Set2DMode
===============
*/
void R_Set2DMode( qboolean enable )
{
	if( enable )
	{
		if( glState.in2DMode )
			return;

		// set 2D virtual screen size
		glViewport( 0, 0, gpGlobals_gl->width, gpGlobals_gl->height );
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0, gpGlobals_gl->width, gpGlobals_gl->height, 0, -99999, 99999 );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		GL_Cull( GL_NONE );

#if XASH_DREAMCAST
  		glDisable(GL_NEARZ_CLIPPING_KOS);
#endif

		glDepthMask( GL_FALSE );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_ALPHA_TEST );
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

		glState.in2DMode = true;
		RI.currententity = NULL;
		RI.currentmodel = NULL;
	}
	else
	{
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
		glState.in2DMode = false;

		glMatrixMode( GL_PROJECTION );
		GL_LoadMatrix( RI.projectionMatrix );

		glMatrixMode( GL_MODELVIEW );
		GL_LoadMatrix( RI.worldviewMatrix );

		GL_Cull( GL_FRONT );

#if XASH_DREAMCAST
  		glDisable(GL_NEARZ_CLIPPING_KOS);
#endif
	}
}
