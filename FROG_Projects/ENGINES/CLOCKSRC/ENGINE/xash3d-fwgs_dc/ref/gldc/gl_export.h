/*
gl_export.h - opengl definition
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef GL_EXPORT_H
#define GL_EXPORT_H
#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef APIENTRY_LINKAGE
#define APIENTRY_LINKAGE extern
#endif

#if defined XASH_NANOGL || defined XASH_WES || defined XASH_REGAL
#define XASH_GLES
#define XASH_GL_STATIC
#define REF_GL_KEEP_MANGLED_FUNCTIONS
#elif defined XASH_GLES3COMPAT
#ifdef SOFTFP_LINK
#undef APIENTRY
#define APIENTRY __attribute__((pcs("aapcs")))
#endif
#define XASH_GLES
#endif

#if XASH_DREAMCAST
#include <GL/gl.h>
#include <GL/glkos.h>
#include <GL/glext.h>
#include <GL/glu.h>
#endif

#if XASH_DREAMCAST
//missing GLDC definitions
#define GL_ADD				0x0104
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#if defined( XASH_GL_STATIC ) && !defined( REF_GL_KEEP_MANGLED_FUNCTIONS )
#define GL_FUNCTION( name ) name
#elif defined( XASH_GL_STATIC ) && defined( REF_GL_KEEP_MANGLED_FUNCTIONS )
#define GL_FUNCTION( name ) APIENTRY p##name
#else
#define GL_FUNCTION( name ) (APIENTRY *p##name)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#endif//GL_EXPORT_H
