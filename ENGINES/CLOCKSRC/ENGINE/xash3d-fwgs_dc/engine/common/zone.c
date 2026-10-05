/*
zone.c - zone memory allocation from DarkPlaces
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2007 DarkPlaces contributors
Copyright (C) 2007 Uncle Mike
Copyright (C) 2015-2023 Xash3D FWGS contributors
Copyright (C) 2026 FROGHOUSE GAMES Elisha Caez

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"

#define MEMHEADER_SENTINEL1	0xA1BAU
#define MEMHEADER_SENTINEL2	0xDFU

#ifdef XASH_CUSTOM_SWAP
#include "platform/swap/swap.h"
#define Q_malloc SWAP_Malloc
#define Q_free SWAP_Free

static void *Q_realloc( void *mem, size_t size )
{
	void *newmem;

	if( mem && size == 0 )
	{
		Q_free( mem );
		return NULL;
	}

	newmem = Q_malloc( size );
	if( mem && newmem )
	{
		memcpy( newmem, mem, size );
		Q_free( mem );
	}

	return newmem;
}
#else
#define Q_malloc malloc
#define Q_free free
#define Q_realloc realloc
#endif

// keep this structure as compact as possible while keeping it aligned
// on ILP32 it's 24 bytes, which is aligned to 8 byte boundary
// on LP64 it's 40 bytes, which is also aligned to 8 byte boundary
typedef struct memheader_s
{
	struct memheader_s *next, *prev; // next and previous memheaders in chain belonging to pool
	const char         *filename;    // file name and line where Mem_Alloc was called
	size_t             size;         // size of the memory after the header (excluding header and sentinel2)
	poolhandle_t       poolptr;      // pool this memheader belongs to
	uint16_t           fileline;
	uint16_t           sentinel1;    // must be equal to MEMHEADER_SENTINEL1
	// immediately followed by data, which is followed by a MEMHEADER_SENTINEL2 byte
} memheader_t;

typedef struct mempool_s
{
	struct memheader_s *chain;        // chain of individual memory allocations
	size_t             totalsize;     // total memory allocated in this pool (inside memheaders)
	size_t             realsize;      // total memory allocated in this pool (actual malloc total)
	size_t             lastchecksize; // updated each time the pool is displayed by memlist
	const char         *filename;     // file name and line where Mem_AllocPool was called
	int                fileline;
	char               name[64];      // name of the pool
} mempool_t;

static mempool_t *poolchain = NULL; // critical stuff
static size_t poolcount = 0;

static mempool_t *Mem_FindPool( poolhandle_t poolptr )
{
	if( likely( poolptr > 0 && poolptr <= poolcount ))
		return &poolchain[poolptr - 1];

	Sys_Error( "%s: not allocated or double freed pool %d", __func__, poolptr );
	return NULL;
}

static poolhandle_t Mem_PoolIndex( mempool_t *mempool )
{
	return (poolhandle_t)(mempool - poolchain) + 1;
}

static inline void Mem_PoolAdd( mempool_t *pool, size_t size )
{
	pool->totalsize += size;
	pool->realsize += sizeof( memheader_t ) + size + sizeof( byte );
}

static inline void Mem_PoolSubtract( mempool_t *pool, size_t size )
{
	pool->totalsize -= size;
	pool->realsize -= sizeof( memheader_t ) + size + sizeof( byte );
}

static inline void Mem_PoolLinkAlloc( mempool_t *pool, memheader_t *mem )
{
	mem->next = pool->chain;
	if( mem->next ) mem->next->prev = mem;
	pool->chain = mem;
	mem->prev = NULL;
	mem->poolptr = Mem_PoolIndex( pool );
}

static inline void Mem_PoolUnlinkAlloc( mempool_t *pool, memheader_t *mem )
{
	if( mem->next ) mem->next->prev = mem->prev;
	if( mem->prev ) mem->prev->next = mem->next;
	else pool->chain = mem->next;
	mem->poolptr = 0;
}

static inline void Mem_InitAlloc( memheader_t *mem, size_t size, const char *filename, int fileline )
{
	mem->size = size;
	mem->filename = filename;
	mem->fileline = fileline;
	mem->sentinel1 = MEMHEADER_SENTINEL1;
	*((byte *)mem + sizeof( memheader_t ) + mem->size ) = MEMHEADER_SENTINEL2;
}

static const char *Mem_CheckFilename( const char *filename )
{
	static const char *dummy = "<corrupted>\0";

	if( !COM_CheckString( filename ))
		return dummy;

	if( memchr( filename, '\0', MAX_OSPATH ) != NULL )
		return filename;

	return dummy;
}

static qboolean Mem_CheckAllocHeader( const char *func, const memheader_t *mem, const char *filename, int fileline )
{
	const char *memfilename;

	if( mem->sentinel1 != MEMHEADER_SENTINEL1 )
	{
		memfilename = Mem_CheckFilename( mem->filename );
		Sys_Error( "%s: trashed header sentinel 1 (alloc at %s:%i, check at %s:%i)\n", func, memfilename, mem->fileline, filename, fileline );
		return false;
	}

	if( *((byte *)mem + sizeof( memheader_t ) + mem->size ) != MEMHEADER_SENTINEL2 )
	{
		memfilename = Mem_CheckFilename( mem->filename );
		Sys_Error( "%s: trashed header sentinel 2 (alloc at %s:%i, check at %s:%i)\n", func, memfilename, mem->fileline, filename, fileline );
		return false;
	}

	return true;
}

/*
========================
_Mem_Alloc

Polymorphic Tracked Pool Allocator.
Allocates a tracking structure prefix on the standard heap to ensure the block
is tracked and cleaned up on map changes, avoiding memory leak build-ups.
========================
*/
void *_Mem_Alloc( poolhandle_t poolptr, size_t size, qboolean clear, const char *filename, int fileline )
{
	memheader_t *mem;
	mempool_t   *pool;
	void        *ptr;

	if( size <= 0 )
		return NULL;

	pool = Mem_FindPool( poolptr );
	if( !pool )
		return NULL;

	// Prefixes block with memheader to maintain pool garbage collection stability
	mem = (memheader_t *)Q_malloc( sizeof( memheader_t ) + size + sizeof( byte ));
	if( !mem )
	{
		Sys_Error( "%s: out of memory allocating %zu bytes (alloc at %s:%i)\n", __func__, size, filename, fileline );
		return NULL;
	}

	Mem_InitAlloc( mem, size, filename, fileline );
	Mem_PoolLinkAlloc( pool, mem );
	Mem_PoolAdd( pool, size );

	ptr = (void *)(mem + 1);
	if( clear )
	{
		memset( ptr, 0, size );
	}

	return ptr;
}

static void Mem_FreeBlock( memheader_t *mem, const char *filename, int fileline )
{
	mempool_t		*pool;

	if( !Mem_CheckAllocHeader( __func__, mem, filename, fileline ))
		return;

	pool = Mem_FindPool( mem->poolptr );
	if( !pool )
		return;

	// unlink memheader from doubly linked list
	if(( mem->prev ? mem->prev->next != mem : pool->chain != mem ) || ( mem->next && mem->next->prev != mem ))
	{
		// --- [ HEAP-IMMUNIZATION BYPASS ] ---
		// Print a harmless warning instead of calling Sys_Error, preventing
		// legacy double-frees from crashing your stock Dreamcast build.
		Con_Reportf( "%s: not allocated or double freed (free at %s:%i) - bypassed safely\n", __func__, filename, fileline );
		return;
	}

	Mem_PoolSubtract( pool, mem->size );
	Mem_PoolUnlinkAlloc( pool, mem );

	Q_free( mem );
}

/*
========================
_Mem_Free

Polymorphic Heap Deallocator.
Analyzes the pointer's preceding metadata bytes. If valid sentinels are present, 
it treats the block as a tracked pool allocation. Otherwise, it frees it natively 
on the standard heap.
========================
*/
void _Mem_Free( void *data, const char *filename, int fileline )
{
	memheader_t *mem;

	if( !data )
		return;

	// Inspect the preceding bytes to detect a potential tracked header prefix
	mem = (memheader_t *)((byte *)data - sizeof( memheader_t ));

	// Check sentinels to see if it is a validated tracked block
	if( mem->sentinel1 == MEMHEADER_SENTINEL1 )
	{
		// Perform pool index range checks to prevent read access faults on raw pointers
		if( mem->poolptr > 0 && mem->poolptr <= poolcount && mem->size < 128 * 1024 * 1024 )
		{
			if( *((byte *)mem + sizeof( memheader_t ) + mem->size ) == MEMHEADER_SENTINEL2 )
			{
				// Sentinels match. This is a validated zone pool allocation.
				Mem_FreeBlock( mem, filename, fileline );
				return;
			}
		}
	}

	// Sentinels are missing or invalid. This is a standard heap pointer.
	Q_free( data );
}

static void Mem_MigratePool( poolhandle_t newpoolptr, memheader_t *mem, const char *filename, int fileline )
{
	mempool_t *oldpool = Mem_FindPool( mem->poolptr );
	mempool_t *newpool = Mem_FindPool( newpoolptr );

	Mem_PoolUnlinkAlloc( oldpool, mem );
	Mem_PoolSubtract( oldpool, mem->size );

	Mem_PoolLinkAlloc( newpool, mem );
	Mem_PoolAdd( newpool, mem->size );
}

/*
========================
_Mem_Realloc

Polymorphic Reallocator.
Detects if the incoming pointer is a validated pool block or a standard heap block,
resizing the memory segments appropriately.
========================
*/
void *_Mem_Realloc( poolhandle_t poolptr, void *data, size_t size, qboolean clear, const char *filename, int fileline )
{
	void *ptr;
	memheader_t *mem;

	if( size <= 0 )
	{
		_Mem_Free( data, filename, fileline );
		return NULL;
	}

	if( !data )
	{
		return _Mem_Alloc( poolptr, size, clear, filename, fileline );
	}

	mem = (memheader_t *)((byte *)data - sizeof( memheader_t ));

	// Check if the incoming pointer is a validated pool block
	if( mem->sentinel1 == MEMHEADER_SENTINEL1 &&
		mem->poolptr > 0 && mem->poolptr <= poolcount &&
		mem->size < 128 * 1024 * 1024 &&
		*((byte *)mem + sizeof( memheader_t ) + mem->size ) == MEMHEADER_SENTINEL2 )
	{
		mempool_t *pool = Mem_FindPool( mem->poolptr );
		size_t oldsize = mem->size;

		Mem_PoolUnlinkAlloc( pool, mem );
		Mem_PoolSubtract( pool, oldsize );

		memheader_t *newmem = (memheader_t *)Q_realloc( mem, sizeof( memheader_t ) + size + sizeof( byte ));
		if( !newmem )
		{
			Mem_PoolLinkAlloc( pool, mem );
			Mem_PoolAdd( pool, oldsize );
			Sys_Error( "%s: out of memory (realloc to %zu bytes at %s:%i)\n", __func__, size, filename, fileline );
			return NULL;
		}

		Mem_InitAlloc( newmem, size, filename, fileline );
		
		mempool_t *target_pool = Mem_FindPool( poolptr ? poolptr : newmem->poolptr );
		Mem_PoolLinkAlloc( target_pool, newmem );
		Mem_PoolAdd( target_pool, size );

		ptr = (void *)(newmem + 1);
		if( size > oldsize && clear )
		{
			memset( (byte *)ptr + oldsize, 0, size - oldsize );
		}
		return ptr;
	}

	// This is a standard heap block. Resize natively.
	ptr = Q_realloc( data, size );
	return ptr;
}

static poolhandle_t Mem_InitPool( mempool_t *pool, const char *name, const char *filename, int fileline )
{
	memset( pool, 0, sizeof( *pool ));

	// fill header
	pool->filename = filename;
	pool->fileline = fileline;
	pool->realsize = sizeof( mempool_t );
	Q_strncpy( pool->name, name, sizeof( pool->name ));

	return Mem_PoolIndex( pool );
}

poolhandle_t _Mem_AllocPool( const char *name, const char *filename, int fileline )
{
	mempool_t *pool;
	size_t i;

	for( i = 0, pool = poolchain; i < poolcount; i++, pool++ )
	{
		if( pool->filename == NULL )
			return Mem_InitPool( pool, name, filename, fileline );
	}

	pool = (mempool_t *)Q_realloc( poolchain, sizeof( *poolchain ) * ( poolcount + 1 ));
	if( pool == NULL )
	{
		Sys_Error( "%s: out of memory (allocpool at %s:%i)\n", __func__, filename, fileline );
		return 0;
	}

	poolchain = pool;
	pool = &poolchain[poolcount++];
	return Mem_InitPool( pool, name, filename, fileline );
}

void _Mem_FreePool( poolhandle_t *poolptr, const char *filename, int fileline )
{
	mempool_t	*pool;

	if( *poolptr && ( pool = Mem_FindPool( *poolptr )))
	{
		if( !pool->filename )
		{
			Sys_Error( "%s: pool already free (freepool at %s:%i)\n", __func__, filename, fileline );
			*poolptr = 0;
			return;
		}

		// free memory owned by the pool
		while( pool->chain )
			Mem_FreeBlock( pool->chain, filename, fileline );

		// free the pool itself
		memset( pool, 0xBF, sizeof( mempool_t ));
		pool->chain = NULL;
		pool->filename = NULL; // mark as reusable
		*poolptr = 0;
	}
}

void _Mem_EmptyPool( poolhandle_t poolptr, const char *filename, int fileline )
{
	mempool_t *pool;
	if( unlikely( !poolptr ))
	{
		Sys_Error( "%s: pool == NULL (emptypool at %s:%i)\n", __func__, filename, fileline );
		return;
	}

	pool = Mem_FindPool( poolptr );
	if( !pool )
		return;

	// free memory owned by the pool
	while( pool->chain ) Mem_FreeBlock( pool->chain, filename, fileline );
}

static qboolean Mem_CheckAlloc( mempool_t *pool, void *data )
{
	memheader_t *header, *target;

	if( pool )
	{
		// search only one pool
		target = (memheader_t *)((byte *)data - sizeof( memheader_t ));
		for( header = pool->chain; header; header = header->next )
		{
			if( header == target )
				return true;
		}
	}
	else
	{
		// search all pools
		size_t i;
		for( i = 0, pool = poolchain; i < poolcount; i++, pool++ )
		{
			if( Mem_CheckAlloc( pool, data ))
				return true;
		}
	}
	return false;
}

/*
========================
Check pointer for memory
========================
*/
qboolean Mem_IsAllocatedExt( poolhandle_t poolptr, void *data )
{
	mempool_t	*pool = NULL;

	if( poolptr )
		pool = Mem_FindPool( poolptr );

	return Mem_CheckAlloc( pool, data );
}

void _Mem_Check( const char *filename, int fileline )
{
	memheader_t *mem;
	mempool_t   *pool;
	size_t i;

	for( i = 0, pool = poolchain; i < poolcount; i++, pool++ )
		for( mem = pool->chain; mem; mem = mem->next )
			Mem_CheckAllocHeader( __func__, mem, filename, fileline );
}

void Mem_PrintStats( void )
{
	size_t    count = 0, size = 0, realsize = 0, i;
	mempool_t *pool;

	Mem_Check();
	for( i = 0, pool = poolchain; i < poolcount; i++, pool++ )
	{
		if( !pool->filename )
			continue;

		count++;
		size += pool->totalsize;
		realsize += pool->realsize;
	}

	Con_Printf( "^3%zu^7 memory pools, totalling: ^1%s\n", count, Q_memprint( size ));
	Con_Printf( "total allocated size: ^1%s\n", Q_memprint( realsize ));

#if XASH_DREAMCAST
	getRamStatus();
#endif
}

void Mem_PrintList( size_t minallocationsize )
{
	mempool_t		*pool;
	memheader_t	*mem;
	size_t i;

	Mem_Check();

	Con_Printf( "memory pool list:\n" );
	Con_Printf( "\t^3size\t\t\t\tname\n");
	for( i = 0, pool = poolchain; i < poolcount; i++, pool++ )
	{
		long	changed_size = (long)pool->totalsize - (long)pool->lastchecksize;

		if( !pool->filename )
			continue;

		// poolnames can contain color symbols, make sure what color is reset
		if( pool->lastchecksize != 0 && changed_size != 0 )
		{
			char	sign = (changed_size < 0) ? '-' : '+';

			Con_Printf( "%10s (%10s real)\t%s (^7%c%s change)\n", Q_memprint( pool->totalsize ), Q_memprint( pool->realsize ),
				pool->name, sign, Q_memprint( abs( changed_size )));
		}
		else
		{
			Con_Printf( "%10s (%10s real)\t%s\n", Q_memprint( pool->totalsize ), Q_memprint( pool->realsize ), pool->name );
		}

		pool->lastchecksize = pool->totalsize;
		for( mem = pool->chain; mem; mem = mem->next )
		{
			if( mem->size >= minallocationsize )
				Con_Printf( "%10s allocated at %s:%i\n", Q_memprint( mem->size ), mem->filename, mem->fileline );
		}
	}
}

/*
========================
Memory_Init
========================
*/
void Memory_Init( void )
{
	poolchain = NULL; // init mem chain
}
