#ifndef ARCHIVELIB_HASHINDEX_H
#define ARCHIVELIB_HASHINDEX_H

#include "Errors.h"

#define _MAX_ITEMS_PER_INDEX 2000
static const size_t MAX_ITEMS_PER_INDEX = _MAX_ITEMS_PER_INDEX;


/**
 *
 * A HashItem that maps a key (20 bytes)
 * to a pointer in the data part of your archvie
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
    unsigned short          allocated;
    unsigned short          length;
    HashItem*               items;
} HashPage;


/**
 *
 * Hash Index keeps a collection of pages mapped by the first char of the key.
 *
 */
typedef struct HashIndex
{
    size_t                  n_items;
    HashPage*               pages[256];
} HashIndex;


#pragma mark HashIndex

/**
 Initializes a new empty index.

 @param self The index to initialize.
 */
void            HashIndex_init(HashIndex*               self);

void            HashIndex_free(HashIndex*               self);

HashPage*       HashIndex_get_or_create_page(HashIndex* self,
                                             char       key[20]);

bool            HashIndex_has(HashIndex*                self,
                              char*                     key);

HashItem*       HashIndex_get(HashIndex*                self,
                              char*                     key);

Errors          HashIndex_set(HashIndex*                self,
                              HashItem*                 item);

#pragma mark HashItem

void            HashItem_init_with_key(HashItem*        self,
                                       char*            key,
                                       size_t           data_offset,
                                       size_t           data_size);

#endif //ARCHIVELIB_HASHINDEX_H
