#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "HashIndex.h"

unsigned short HASH_PAGE_INITIAL_CAPACITY = 10;


#pragma mark - HashItem

/**
 Initializes a HashItem for a given key.

 @param self The HashItem.
 @param key The key (20 bytes).
 @param size Data size of the item in the data.
 @param offset Data offset of the item in the data.
 */
static inline void	HashItem_init_with_key(HashItem*        self,
                    	                   const char*      key,
                                           size_t           offset,
                        	               size_t           size)
{
    memcpy(self->key, key, 20);
    self->data_offset = offset;
    self->data_size = size;
}


#pragma mark - HashPage


/**
 Initializes a new hash page.

 @param self The hash page to be initialized.
 */
static inline void  HashPage_init(HashPage*         self)
{
    self->items = (HashItem*)malloc(sizeof(HashItem) *
                                    HASH_PAGE_INITIAL_CAPACITY);
    self->capacity = HASH_PAGE_INITIAL_CAPACITY;
    self->n_items = 0;
}


/**
 Frees the hash page. This won't free the page structure itself.

 @param page The hash page to free.
 */
static inline void  HashPage_free(HashPage*         page)
{
    free(page->items);
}


/**
 Retrieves an item from the hash page.

 @param self The hash page.
 @param key The key to retrieve.
 @return The hash item. It's not a copy, the item is still in the page.
 */
static inline const HashItem* HashPage_get(HashPage*    self,
                                           const char*  key)
{
    HashItem *item;
    for (int i = 0; i < self->n_items; ++i) {
        item = &self->items[i];
        if (memcmp(item->key, key, 20) == 0) {
            return item;
        }
    }
    return NULL;
}


/**
 Checks whether an entry is in the page for the given key.

 @param self The hash page.
 @param key The item key.
 @return A boolean representing whether the key is in the hash page.
 */
static inline bool      HashPage_has(HashPage*      	self,
                                     const char*    	key)
{
    return NULL != HashPage_get(self, key);
}


/**
 Adds a HashItem to the page.

 @param self The hash page.
 @param key The key to insert (a 20 bytes binary string).
 @param size Data size in the file.
 @param offset Data offset in the file.
 */
static inline void        HashPage_set(HashPage*        self,
                                       const char*      key,
                                       size_t           offset,
                                       size_t           size)
{
    // if needed, increase the capacity
    if (self->n_items >= self->capacity) {
        unsigned short new_capacity = self->capacity * 2;
        self->items = realloc(self->items, (sizeof(HashItem) * new_capacity));
        self->capacity = new_capacity;
    }

    // set the hash item
    HashItem_init_with_key(self->items + self->n_items, key, offset, size);

    // increase the number of items
    self->n_items += 1;
}


#pragma mark - HashIndex (Private)


/**
 Gets a page from the index given a key.

 @param self The hash index.
 @param key The key to retrieve.
 @return The page found.
 */
static inline HashPage*     HashIndex_get_page(HashIndex*       self,
                                               const char*      key)
{
    char lookup_chr = key[0];
    return self->pages[lookup_chr];
}


#pragma mark - HashIndex


void            HashIndex_init(HashIndex*               self)
{
    self->n_items = 0;
    memset(self->pages, 0, sizeof(HashPage*[256]));
}


void            HashIndex_free(HashIndex*               self)
{
    HashPage* page;
    for (int j = 0; j < 255; ++j) {
        page = self->pages[j];
        if (page != NULL) {
            HashPage_free(page);
            free(page);
        }
    }
}


HashPage*       HashIndex_get_or_create_page(HashIndex* 	self,
                                             const char*	key)
{
    char lookup_chr = key[0];
    HashPage* page = self->pages[lookup_chr];
    if (page == NULL) {
        page = (HashPage*)malloc(sizeof(HashPage));
        HashPage_init(page);
        self->pages[lookup_chr] = page;
    }
    return page;
}


bool            HashIndex_has(HashIndex*                	self,
                              const char*               	key)
{
    HashPage* page = HashIndex_get_page(self, key);
    if (page == NULL) {
        return false;
    }
    return HashPage_has(page, key);
}


Errors          HashIndex_set(HashIndex*                	self,
                              const char*                   key,
                              size_t                        offset,
                              size_t                        size)
{
    if (self->n_items >= MAX_ITEMS_PER_INDEX) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    HashPage* page = HashIndex_get_or_create_page(self, key);
    HashPage_set(page, key, offset, size);
    self->n_items += 1;
    return E_SUCCESS;
}


const HashItem* HashIndex_get(HashIndex*                	self,
                              const char*               	key)
{
    HashPage* page = HashIndex_get_page(self, key);
    if (page == NULL) {
        return NULL;
    }
    return HashPage_get(page, key);
}
