#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "HashIndex.h"

static size_t HASH_PAGE_INITIAL_CAPACITY = 10;

#pragma mark - HashItem


/**
 Initializes a HashItem for a given key.

 @param self The HashItem.
 @param key The key (20 bytes).
 @param size Data size of the item in the data.
 @param offset Data offset of the item in the data.
 */
static inline void  _HashItem_init_with_key(HashItem*       self,
                                            const char*     key,
                                            size_t          offset,
                                            size_t          size)
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
static inline void  _HashPage_init(HashPage*                self)
{
    self->items = (HashItem*)malloc(
        sizeof(HashItem) * HASH_PAGE_INITIAL_CAPACITY);
    self->capacity = HASH_PAGE_INITIAL_CAPACITY;
    self->n_items = 0;
}


/**
 Frees the hash page. This won't free the page structure itself.

 @param self The hash page to free.
 */
static inline void  _HashPage_free(HashPage*                self)
{
    free(self->items);
}


/**
 Retrieves an item from the hash page.

 @param self The hash page.
 @param partial_key The partial key to lookup (between 3 and 20 bytes).
 @param partial_key_len The length of the partial key.
 @return The hash item. It's not a copy, the item is still in the page.
 */
static inline const HashItem* _HashPage_get(HashPage*       self,
                                            const char*     partial_key,
                                            size_t          partial_key_len)
{
    size_t n_items = self->n_items;
    HashItem *item = self->items;
    for (int i = 0; i < n_items; ++i) {
        char* item_key = item->key;
        // the first byte doesn't need to be compared as it's in the hash key
        // the two first bytes are compared inline here to reduce the use of
        // memcmp (which is more expensive)
        if (item_key[1] == partial_key[1] &&
            item_key[2] == partial_key[2] &&
            memcmp(item_key + 3, partial_key + 3, partial_key_len - 3) == 0) {
            return item;
        }
        item++;
    }
    return NULL;
}


/**
 Adds a HashItem to the page.

 @param self The hash page.
 @param key The key to insert (a 20 bytes binary string).
 @param size Data size in the file.
 @param offset Data offset in the file.
 */
static inline void        _HashPage_set(HashPage*           self,
                                        const char*         key,
                                        size_t              offset,
                                        size_t              size)
{
    // if needed, increase the capacity
    if (self->n_items >= self->capacity) {
        size_t new_capacity = self->capacity * 2;
        if (new_capacity <= HASH_PAGE_INITIAL_CAPACITY) {
            new_capacity = HASH_PAGE_INITIAL_CAPACITY;
        }
        self->items = (HashItem*)realloc(self->items,
                                         sizeof(HashItem) * new_capacity);
        self->capacity = new_capacity;
    }

    // set the hash item
    _HashItem_init_with_key(self->items + self->n_items, key, offset, size);

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
static inline HashPage*     _HashIndex_get_page(HashIndex*  self,
                                                const char* key)
{
    return &(self->pages[_HashIndex_key(key)]);
}


#pragma mark - HashIndex (Public API)


void      HashIndex_init(HashIndex*                   self)
{
    self->n_items = 0;
    memset(self->pages, 0, sizeof(HashPage[HashIndexPageCount]));
}


void      HashIndex_free(HashIndex*                   self)
{
    HashPage* page;
    for (int i = 0; i < HashIndexPageCount; ++i) {
        page = &(self->pages[i]);
        if (page->items != NULL) {
            _HashPage_free(page);
        }
    }
}


HashPage* HashIndex_get_or_create_page(HashIndex*     self,
                                       const char*    key)
{
    HashPage* page = &(self->pages[_HashIndex_key(key)]);
    if (page->items == NULL) {
        _HashPage_init(page);
    }
    return page;
}


const HashItem* HashIndex_get(HashIndex*              self,
                              const char*             partial_key,
                              size_t                  partial_key_len)
{
    HashPage* page = _HashIndex_get_page(self, partial_key);
    return _HashPage_get(page, partial_key, partial_key_len);
}


Errors    HashIndex_set(HashIndex*                    self,
                        const char*                   key,
                        size_t                        offset,
                        size_t                        size)
{
    if (self->n_items >= MAX_ITEMS_PER_INDEX) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    HashPage* page = HashIndex_get_or_create_page(self, key);
    _HashPage_set(page, key, offset, size);
    self->n_items += 1;
    return E_SUCCESS;
}
