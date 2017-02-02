#ifndef HASHINDEXPACK_H
#define HASHINDEXPACK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#import "HashIndex.h"
#include "Errors.h"


typedef struct __attribute__((__packed__)) PackedHashItem
{
    char                    key[20];
    __uint32_t              data_offset;
    __uint32_t              data_size;
} PackedHashItem;


/**
 Pack the HashIndex into an array of PackedHashItem

 @param self The HashIndex.
 @param items An array of PackedHashItem to populate.
 @param capacity The maximum number of PackedHashItem in items.
 @return An error code.
 */
Errors    HashIndex_pack(HashIndex*             self,
                         PackedHashItem*        items,
                         size_t                 capacity,
                         size_t*                _n_items);


/**
 Loads an array of PackedHashItem into the index.

 @param self The HashIndex.
 @param items An array of PackedHashItem to read from.
 @param n_items The number of items in the array.
 */
void      HashIndex_unpack(HashIndex*           self,
                           PackedHashItem*      items,
                           size_t               n_items);

#endif /* HASHINDEXPACK_H */
