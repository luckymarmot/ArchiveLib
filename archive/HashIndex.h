#ifndef ARCHIVELIB_HASHINDEX_H
#define ARCHIVELIB_HASHINDEX_H

#include <stdint.h>
#include <stdbool.h>
#include "Errors.h"

#define _MAX_ITEMS_PER_INDEX 2000
static const size_t MAX_ITEMS_PER_INDEX = _MAX_ITEMS_PER_INDEX;

#pragma mark - Lookup Key

#define HashIndexPageCount 256

static inline size_t _HashIndex_key(const char* key)
{
    return (unsigned char)key[0];
}

#pragma mark - Structs

/**
 * A HashItem that maps a key (20 bytes)
 * to a pointer in the data part of the file system archive.
 */
typedef struct HashItem
{
    char                    key[20];
    size_t                  data_offset;
    size_t                  data_size;
} HashItem;


/**
 * Page of the Hash Map
 * there is 1 page for each of the first chars.
 */
typedef struct HashPage
{
    size_t                  capacity;
    size_t                  n_items;
    HashItem*               items;
} HashPage;


/**
 * Hash Index keeps a collection of pages mapped by the first char of the key.
 */
typedef struct HashIndex
{
    size_t                  n_items;
    HashPage                pages[HashIndexPageCount];
} HashIndex;


#pragma mark - HashIndex (Public API)


/**
 Initializes a new empty index.

 @param self The index.
 */
void      HashIndex_init(HashIndex*                   self);


/**
 Frees the index.

 @param self The index to free.
 */
void      HashIndex_free(HashIndex*                   self);


/**
 Gets an existing page for the key, or create a new one.

 @param self The index.
 @param key The key for the page.
 @return The page.
 */
HashPage* HashIndex_get_or_create_page(HashIndex*     self,
                                       const char*    key);


/**
 Retrieves an hash item from the index by its key.

 @param self The index.
 @param partial_key The partial key to lookup (between 3 and 20 bytes).
 @param partial_key_len The length of the partial key.
 @return The hash item.
 */
const HashItem* HashIndex_get(HashIndex*              self,
                              const char*             partial_key,
                              size_t                  partial_key_len);


/**
 Sets an item in the index.

 @param self The index.
 @param key The key to insert (a 20 bytes binary string).
 @param size Data size in the file.
 @param offset Data offset in the file.
 @return An error code.
 */
Errors    HashIndex_set(HashIndex*                    self,
                        const char*                   key,
                        size_t                        offset,
                        size_t                        size);

#endif //ARCHIVELIB_HASHINDEX_H
