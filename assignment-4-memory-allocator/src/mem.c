#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

void debug_block(struct block_header* b, const char* fmt, ... );
void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

static bool            block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }
static size_t          pages_count   ( size_t mem )                      { return mem / getpagesize() + ((mem % getpagesize()) > 0); }
static size_t          round_pages   ( size_t mem )                      { return getpagesize() * pages_count( mem ) ; }

static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) {
  *((struct block_header*)addr) = (struct block_header) {
    .next = next,
    .capacity = capacity_from_size(block_sz),
    .is_free = true
  };
}

static size_t region_actual_size( size_t query ) { return size_max( round_pages( query ), REGION_MIN_SIZE ); }

extern inline bool region_is_invalid( const struct region* r );



static void* map_pages(void const* addr, size_t length, int additional_flags) {
  return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , -1, 0 );
}

/*  аллоцировать регион памяти и инициализировать его блоком */
static struct region alloc_region  ( void const * addr, size_t query ) {
  if (!addr) return REGION_INVALID;
    size_t size_of_block = size_from_capacity((block_capacity) {.bytes = query}).bytes;
    size_t region_size = region_actual_size(size_of_block);
    void *region = map_pages(addr,region_size,MAP_FIXED_NOREPLACE);
    if (region == MAP_FAILED) {
        region = map_pages(addr, region_size, 0);
    }
    if (region != MAP_FAILED) {
        block_init(region,(block_size){.bytes = region_size},NULL);
        return (struct region) {.addr=region, .size=region_size, .extends=region==addr};
    }
    return REGION_INVALID;
}


static void* block_after( struct block_header const* block )         ;

void* heap_init( size_t initial ) {
  const struct region region = alloc_region( HEAP_START, initial );
  if ( region_is_invalid(&region) ) return NULL;

  return region.addr;
}

#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */

static bool block_splittable( struct block_header* restrict block, size_t query) {
  return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

static bool split_if_too_big( struct block_header* block, size_t query ) {
  if (block != NULL){
    if (!block_splittable(block, query)) {
      return false;
    }

    size_t actual_query_size = size_max(query, BLOCK_MIN_CAPACITY);
    
    void* new_addr = block->contents + actual_query_size;
    block_size size = (block_size) {block->capacity.bytes - actual_query_size};
    void* next = block->next;
    block_init(new_addr, size, next);

    block->next = new_addr;
    block->capacity = (block_capacity) {actual_query_size};

    return true;
  }
  return false;
}


/*  --- Слияние соседних свободных блоков --- */

static void* block_after( struct block_header const* block )              {
  return  (void*) (block->contents + block->capacity.bytes);
}
static bool blocks_continuous (
                               struct block_header const* fst,
                               struct block_header const* snd ) {
  return (void*)snd == block_after(fst);
}

static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) {
  return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

static bool try_merge_with_next( struct block_header* block ) {
  struct block_header *next = block->next;
	if (next != NULL && mergeable(block, next)){
		block->capacity.bytes = size_from_capacity(next->capacity).bytes + block->capacity.bytes;
		block->next = next->next;
		return true;
	}
	return false;

}


/*  --- ... ecли размера кучи хватает --- */

struct block_search_result {
  enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
  struct block_header* block;
};


static struct block_search_result find_good_or_last  ( struct block_header* restrict block, size_t sz )    {
  if (block && sz>0) {
        while (block) {
            if (block->is_free) {
                while (try_merge_with_next(block));
                if (block_is_big_enough(sz, block)) {
                    return (struct block_search_result) {.block=block, .type=BSR_FOUND_GOOD_BLOCK};
                }
            }
            if (!block->next) return (struct block_search_result) {.type=BSR_REACHED_END_NOT_FOUND, .block=block};
            block = block->next;
        }
    }
    return (struct block_search_result){.type=BSR_CORRUPTED};

}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
static struct block_search_result try_memalloc_existing ( size_t query, struct block_header* block ) {
    struct block_search_result search_result = find_good_or_last (block, query);

    if (search_result.type == BSR_FOUND_GOOD_BLOCK) {
          split_if_too_big(search_result.block, query);
          search_result.block->is_free = false;
    }

    return search_result;
}



static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) {
  if(!last) return NULL;
  struct region region = alloc_region(block_after(last), query + offsetof(struct block_header, contents));
  if(region_is_invalid(&region)) return NULL;
  last->next = region.addr;
  if(try_merge_with_next(last)) return last;
  return last->next;

}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
static struct block_header* memalloc( size_t query, struct block_header* heap_start) {
  if (heap_start) {
        query= size_max(BLOCK_MIN_CAPACITY,query);
        struct block_search_result blockSearchResult = try_memalloc_existing(query, heap_start);
        if (blockSearchResult.type == BSR_REACHED_END_NOT_FOUND) {
            struct block_search_result blockSearchResultLast = blockSearchResult;
            struct block_header* blockHeader = grow_heap(blockSearchResultLast.block, query);
            blockSearchResult = try_memalloc_existing(query, blockHeader);
        }
        if (blockSearchResult.type == BSR_CORRUPTED) return NULL;
        return blockSearchResult.block;
    }
    return NULL;

}

void* _malloc( size_t query ) {
  struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
  if (addr) return addr->contents;
  else return NULL;
}

static struct block_header* block_get_header(void* contents) {
  return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void _free( void* mem ) {
  if (!mem) return ;
  struct block_header* header = block_get_header( mem );
  header->is_free = true;
  while (try_merge_with_next(header));
}
