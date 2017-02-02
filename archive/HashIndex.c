//
// Created by Matthaus Woolard on 27/01/2017.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "HashIndex.h"


/**
 * Init A HashItem
 *
 * @param self the hash item...
 * @param key the items key (20)bytes of md5
 * @param data_offset the possition in the data archive
 * @param data_size the lenght of the data pointed to
 */
void            HashItem_init_with_key(HashItem*        self,
                                       char*            key,
                                       size_t           data_offset,
                                       size_t           data_size)
{
    memcpy(self->key, key, sizeof(char) * 20);
    self->data_offset = data_offset;
    self->data_size = data_size;
}



void HashPage_free(HashPage* page) {
    free(page->items);
    free(page);
}

void HashIndex_free(HashIndex* self) {
    HashPage* page;
    for (int j = 0; j < 255; ++j) {
        page = self->pages[j];
        if (page != NULL) {
            HashPage_free(page);
        }
    }
    //free(&self->pages);
    free(self);
    // Free
}


/**
 * Init DataBlob (do not set the data)
 *
 * @param key
 * @param data_size
 * @return DataBlob*
 */
void DataBlob_init(DataBlob *self, char key[20], size_t data_size) {
    self->data_size = data_size;
    self->key = key;
    self->data = (char *) (malloc(sizeof(char)*data_size));
}


/**
 * Init a hash item given a data blob and offset.
 *
 * @param self
 * @param blob
 * @param data_offset
 */
void HashItem__init_from_data_blob__(HashItem* self, DataBlob* blob, size_t data_offset) {
    HashItem_init_with_key(self, blob->key, data_offset, blob->data_size);
}


unsigned short HASH_PAGE_ALLOCATION = 10;

/**
 * Init Hash Page
 *
 * @param allocated the number of items to prealocate
 * @return the new hash page.
 */
void HashPage_init(HashPage* self, unsigned short allocated) {
    self->items = (HashItem*) malloc(sizeof(HashItem) * allocated);
    self->allocated = allocated;
    self->length = 0;
}


/**
 * Get a Hash Item from a page
 *
 * @param self The hash page
 * @param key the key to get from
 * @return the new hash item or NULL if not found
 */
HashItem* HashPage_get(HashPage* self, char key[20]) {
    HashItem *item;
    int i;
    for (i = 0; i < self->length; i = i + 1) {
        item = &self->items[i];
        if (memcmp(item->key, key, 20) == 0) {
            return item;
        }
    }
    return NULL;
}


/**
 *
 * Check if there is an entry in the page for this given key.
 *
 * @param self
 * @param key
 * @return True if found False otherwise
 */
bool HashPage_has(HashPage* self, char key[20]) {
    return NULL != HashPage_get(self, key);
}



/**
 * Update the page
 *
 * @param self
 * @param new_item new item to push into the page.
 * @return True if updated False Otherwise
 */
bool HashPage_update(HashPage* self, HashItem* new_item) {
    int i = 0;
    HashItem *item;
    for (i = 0; i < self->length; i = i + 1) {
        item = &self->items[i];
        if (memcmp(item->key, new_item->key, 20) == 0) {
            item->data_size = new_item->data_size;
            item->data_offset = new_item->data_offset;
            return true;
        }
    }
    return false;
}


/**
 * Set a HashItem into this page
 *
 * Will update a record if already pressent
 *
 * This copies in the data from item so you can modifiy item afterwards.
 *
 * @param self
 * @param item new HashItem
 * @return the update page (the pointer might change due to realock)
 */
void HashPage_set(HashPage* self, HashItem* item) {
    if (self->length + 1 >= self->allocated) {
        // We need to expand the object
        self->items = realloc(self->items, (sizeof(HashItem) * self->allocated * 2));
        self->allocated = (unsigned short) (self->allocated * 2);
    }

    memcpy(&(self->items[self->length]), item, sizeof(HashItem));

    self->length += 1;
}


static inline void PackedHashItem_unpack(PackedHashItem* self, HashItem* target) {
    memcpy(target->key, self->key, sizeof(char[20]));
    target->data_offset = self->data_offset;
    target->data_size =  self->data_size;
}


void HashPage_set_packed(HashPage* self, PackedHashItem* item) {
    if (self->length + 1 >= self->allocated) {
        // We need to expand the object
        self->items = realloc(self->items, (sizeof(HashItem) * self->allocated * 2));
        self->allocated = (unsigned short) (self->allocated * 2);
    }

    PackedHashItem_unpack(item, &(self->items[self->length]));
    self->length += 1;
}




/**
 * Create a Hash Index (empty)
 *
 *
 * All pointers to pages are set to NULL!
 * @return the new hash index
 */
void HashIndex_init(HashIndex* self) {
    self->n_items = 0;
    memset(self->pages, 0, sizeof(HashIndex *[256]));
}


/**
 * Get a page from the index given a key
 *
 *
 *
 * @param self
 * @param key
 * @return Return NULL if not page found
 */
HashPage* HashIndex_get_page(HashIndex* self, char key[20]) {
    return self->pages[key[0]];
}


/**
 * Get or Create a Page, like get__page this will return a page if found
 * But it will also create a new page if needed, and save that page to the
 *
 * @param self
 * @param key
 * @return
 */
HashPage* HashIndex_get_or_create_page(HashIndex* self, char key[20]) {
    HashPage* page = self->pages[key[0]];
    if (page) {
        return page;
    }
    page = (HashPage*) (malloc(sizeof(HashPage)));
    HashPage_init(page, HASH_PAGE_ALLOCATION);
    self->pages[key[0]] = page;
    return page;
}


/**
 *
 * Get a hash item
 *
 * @param self
 * @param key
 * @return return NULL if not found
 */
HashItem* HashIndex_get_index_item(HashIndex* self, char key[20]) {
    HashPage* page = HashIndex_get_page(self, key);
    if (page) {
        return HashPage_get(page, key);
    }
    return NULL;
}


/**
 *
 * Check if an object is in this index
 *
 * This method is called a lot when inserting values
 *
 * @param self
 * @param key
 * @return True if the object is here
 */
bool HashIndex_has(HashIndex* self, char* key) {
    HashPage* page = HashIndex_get_page(self, key);
    if (page) {
        return HashPage_has(page, key);
    }
    return false;
}



/**
 *
 * Set an index entry to the index.
 *
 * @param self
 * @param item
 * @return the hash index, the pointer will not be changed.
 */
Errors HashIndex_set(HashIndex* self, HashItem* item) {
    if (self->n_items >= MAX_ITEMS_PER_INDEX) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    HashPage* page = HashIndex_get_or_create_page(self, item->key);
    HashPage_set(page, item);
    self->n_items += 1;
    return E_SUCCESS;
}

HashItem* HashIndex_get(HashIndex* self, char* key) {
    HashPage* page = HashIndex_get_page(self, key);
    if (page == NULL) {
        return NULL;
    }
    return HashPage_get(page, key);
}
