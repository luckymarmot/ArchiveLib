//
//  HashIndexPack.c
//  ArchiveLib
//
//  Created by Micha Mazaheri on 2/2/17.
//  Copyright © 2017 Paw. All rights reserved.
//

#include "HashIndexPack.h"
#include "Endian.h"

#pragma mark HashItem Pack

static inline void      HashItem_pack(const HashItem*               self,
                                      PackedHashItem*               packed)
{
    memcpy(packed->key, self->key, 20);
    packed->data_offset = htobe32((__uint32_t)self->data_offset);
    packed->data_size   = htobe32((__uint32_t)self->data_size);
}


static inline void      HashItem_unpack(HashItem*                   self,
                                        const PackedHashItem*       packed)
{
    memcpy(self->key, packed->key, 20);
    self->data_offset   = be32toh(packed->data_offset);
    self->data_size     = be32toh(packed->data_size);
}


static inline void      HashPage_set_packed(HashPage*               self,
                                            const PackedHashItem*   item)
{
    // if needed, increase the capacity
    if (self->n_items >= self->capacity) {
        unsigned short new_capacity = self->capacity * 2;
        self->items = realloc(self->items, (sizeof(HashItem) * new_capacity));
        self->capacity = new_capacity;
    }

    HashItem_unpack(&(self->items[self->n_items]), item);
    self->n_items += 1;
}


Errors    HashIndex_pack(HashIndex*             self,
                         PackedHashItem*        items,
                         size_t                 capacity,
                         size_t*                _n_items)
{
    if (self->n_items > capacity) {
        return E_INDEX_OUT_OF_BOUNDS;
    }
    size_t n_items = 0;
    int i = 0, j = 0;
    HashPage* page;
    for(i = 0; i < 256; i = i + 1 ){
        page = self->pages[i];
        if (page != NULL) {
            if (n_items + page->n_items > capacity) {
                return E_INDEX_OUT_OF_BOUNDS;
            }
            for (j = 0; j < page->n_items; ++j) {
                HashItem_pack(page->items + j, items + n_items + j);
            }
            n_items += page->n_items;
        }
    }
    *_n_items = n_items;
    return E_SUCCESS;
}


void      HashIndex_unpack(HashIndex*           self,
                           PackedHashItem*      items,
                           size_t               n_items)
{
    // Packed indexes are saved in a sequence so a while loop
    // is best here to reduce lookups;
    size_t current_item_index = 0;
    PackedHashItem* current_item;
    HashPage* current_page;
    char current_page_char;
    if (n_items > 0) {
        current_item = &items[current_item_index];
        current_page_char = current_item->key[0];
        current_page = HashIndex_get_or_create_page(self, current_item->key);
    } else {
        return;
    }
    while (current_item_index < n_items) {
        // this is part of a packed struct so memory is managed outside
        // Could do a spead up hear loop until there is a change of page and
        // then mass copy all at once to the old page.
        current_item = &items[current_item_index];
        if (current_page_char != current_item->key[0]) {
            current_page = HashIndex_get_or_create_page(self, current_item->key);
            current_page_char = current_item->key[0];
        }
        HashPage_set_packed(current_page, current_item);
        current_item_index += 1;
    }
    self->n_items = current_item_index;
}
