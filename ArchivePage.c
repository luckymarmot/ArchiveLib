#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "ArchivePage.h"


static Errors       write_to_file(file_descriptor   fd,
                                  const void*       buffer,
                                  size_t            size,
                                  off_t             offset)
{
    off_t writen = 0;
    ssize_t r;
    while (writen < size) {
        r = pwrite(fd, (char*)buffer + writen, size - writen, offset + writen);
        if (r < 0) {
            return E_SYSTEM_ERROR_ERRNO;
        }
        writen += r;
    }
    return E_SUCCESS;
}


static Errors       read_from_file(file_descriptor  fd,
                                   void*            buffer,
                                   size_t           size,
                                   off_t            offset)
{
    off_t read = 0;
    ssize_t r;
    while (read < size) {
        r = pread(fd, (char*)buffer + read, size - read, offset + read);
        if (r < 0) {
            return E_SYSTEM_ERROR_ERRNO;
        }
        read += r;
    }
    return E_SUCCESS;
}


/**
 * Archive File Header
 *
 * Header saved at the top of each file.
 * This helps with unpacking the file.
 */
typedef struct __attribute__((__packed__)){
    size_t capacity;
    size_t saved_items;
    PackedHashItem items[];
} PackedIndex;

/**
 *
 * Build Packed Index
 * Do not copy in any data yet
 *
 */
PackedIndex* PackedIndex_init(size_t capacity) {
    // allocate the memory needed
    PackedIndex* p_index = (PackedIndex*) (
            malloc(sizeof(PackedIndex) + sizeof(PackedHashItem) * capacity)
    );
    p_index->saved_items = 0;
    p_index->capacity = capacity;
    return p_index;
}


static inline void HashItem_pack(HashItem* self, PackedHashItem* target) {
    memcpy(target->key, self->key, sizeof(char[20]));
    target->data_offset = (__uint32_t) self->data_offset;
    target->data_size = (__uint32_t) self->data_size;
}


/**
 *
 * Dump a HashIndex to  packed index
 *
 * @param self the hash index to dump
 * @param packed_index the target
 * @return Error? if it returns E_INDEX_OUT_OF_BOUNDS then the packed index is the wrong dimention
 */
Errors HashIndex_pack(HashIndex* self, PackedIndex* packed_index) {
    if (self->items > packed_index->capacity) {
        return E_INDEX_OUT_OF_BOUNDS;
    }
    size_t items_saved = 0;
    int i = 0;
    int j = 0;
    HashPage* page;
    for( i = 0; i < 256; i = i + 1 ){
        page = self->pages[i];
        if (page != NULL) {
            if (items_saved + page->length > packed_index->capacity) {
                // Update this number before error
                packed_index->saved_items = items_saved;
                return E_INDEX_OUT_OF_BOUNDS;
            }
            for (int k = 0; k < page->length; ++k) {
                HashItem_pack(&page->items[k], &packed_index->items[items_saved + k]);
            }
            items_saved += page->length;
        }
    }
    if (items_saved > packed_index->capacity) {
        return E_INDEX_OUT_OF_BOUNDS;
    }
    packed_index->saved_items = items_saved;
    return E_SUCCESS;
}


/**
 * Unpack a Packed Index into a hash index.
 *
 * @param self
 * @param index
 */
void PackedIndex_unpack(PackedIndex* self, HashIndex* index) {
    // Packed indexes are saved in a sequence so a while loop
    // is best here to reduce lookups;
    size_t current_item_index = 0;
    PackedHashItem* current_item;
    HashPage* current_page;
    char current_page_char;
    if (self->saved_items > 0) {
        current_item = &self->items[current_item_index];
        current_page_char = current_item->key[0];
        current_page = HashIndex_get_or_create_page(index, current_item->key);
    } else {
        return;
    }
    while (current_item_index < self->saved_items) {
        // this is part of a packed struct so memory is managed outside
        // Could do a spead up hear loop until there is a change of page and
        // then mass copy all at once to the old page.
        current_item = &self->items[current_item_index];
        if (current_page_char != current_item->key[0]) {
            current_page = HashIndex_get_or_create_page(index, current_item->key);
            current_page_char = current_item->key[0];
        }
        HashPage_set_packed(current_page, current_item);
        current_item_index += 1;
    }
    index->items = current_item_index;
}


Errors PackedIndex_write_to_disk(PackedIndex* self, ArchivePage* archive) {
    ArchiveFileHeader* header = archive->file_header;
    size_t size = (self->capacity * sizeof(PackedHashItem)) + sizeof(PackedIndex);
    if ( size > header->header_size) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    return write_to_file(archive->fd, self, size, (off_t) header->header_start);
}


Errors PackedIndex_read_size(PackedIndex* self, ArchivePage* archive) {
    ArchiveFileHeader* header = archive->file_header;
    // Read in 2 parts just read the first git of the data first
    return read_from_file(
            archive->fd,
            self,
            sizeof(PackedIndex),
            (off_t) header->header_start
    );
}


Errors PackedIndex_read_from_disk(PackedIndex* self, ArchivePage* archive) {
    ArchiveFileHeader* header = archive->file_header;
    size_t size = (self->capacity * sizeof(PackedHashItem)) + sizeof(PackedIndex);
    if ( size > header->header_size) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    return read_from_file(
            archive->fd,
            self,
            size,
            (off_t) header->header_start
    );
}


/**
 *
 * Open a file descriptor for an archive.
 *
 * @param self The archive page to open.
 * @param filename Filename of the archive file.
 * @return An error code.
 */
static Errors ArchivePage_open_file(ArchivePage* self, char* filename) {
    // open a (new) file for read and write with use ownership.
    file_descriptor fd = open(filename , O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return E_SYSTEM_ERROR_ERRNO;
    }
    self->fd = fd;
    return E_SUCCESS;
}


/**
 *
 * Write File Header
 *
 */
Errors ArchivePage_write_file_header(
        ArchivePage* self,
        ArchiveFileHeader* file_header) {
    return write_to_file(self->fd, file_header, sizeof(ArchiveFileHeader), 0);
}



/**
 *
 * Read File Header
 *
 */
Errors ArchivePage_read_file_header(
        ArchivePage* self,
        ArchiveFileHeader* file_header) {
    return read_from_file(self->fd, file_header, sizeof(ArchiveFileHeader), 0);
}


Errors ArchivePage_init_new_file_header(
        ArchivePage* self,
        ArchiveFileHeader* file_header) {
    file_header->data_size = 0;
    file_header->header_size = sizeof(PackedHashItem) * MAX_ITEMS_PER_INDEX + sizeof(PackedIndex);
    file_header->header_start = sizeof(ArchiveFileHeader);
    file_header->data_start = file_header->header_size + file_header->header_start;
    return ArchivePage_write_file_header(self, file_header);
}


Errors      ArchivePage_init(ArchivePage*           self,
                             HashIndex*             index,
                             char*                  filename,
                             bool                   new_file)
{
    self->index = index;
    self->filename = filename;
    printf("Init File:%s\n", self->filename);
    Errors error = ArchivePage_open_file(self, filename);
    if (error != E_SUCCESS) {
        printf("File not opened ERROR: %s\n", strerror(errno));
        return error;
    }
    self->file_header = (ArchiveFileHeader*) (malloc(sizeof(ArchiveFileHeader)));

    if (!new_file) {
        error = ArchivePage_read_file_header(self, self->file_header);
    } else {
        error = ArchivePage_init_new_file_header(self, self->file_header);
    }
    if (error != E_SUCCESS ) {
        return error;
    }

    if (new_file) {
        HashIndex_init(index);
        self->index = index;
    } else {
        // Read packed index from disk
        // unpack to an index
        PackedIndex* p_index = (PackedIndex*) (malloc(sizeof(PackedIndex)));
        error = PackedIndex_read_size(p_index, self);
        if (error != E_SUCCESS) {
            free(p_index);
            return error;
        }
        size_t capacity = p_index->capacity;
        free(p_index);
        p_index = (PackedIndex*) (malloc(sizeof(PackedIndex) + sizeof(PackedHashItem) * capacity));
        p_index->capacity = capacity;
        error = PackedIndex_read_from_disk(p_index, self);
        if (error != E_SUCCESS) {
            return error;
        }
        HashIndex_init(index);
        PackedIndex_unpack(p_index, index);
        free(p_index);
        self->index = index;
    }
    return E_SUCCESS;
    // Read the header...
}


Errors      ArchivePage_save_to_disk(ArchivePage*   self)
{
    Errors error;
    
    error = ArchivePage_write_file_header(self, self->file_header);
    if (error != E_SUCCESS) {
        printf("ArchivePage_write_file_header %d\n", error);
        return error;
    }
    PackedIndex* p_index = PackedIndex_init(self->index->items);
    error = HashIndex_pack(self->index, p_index);
    if (error != E_SUCCESS) {
        printf("HashIndex_pack %d\n", error);
        return error;
    }
    error = PackedIndex_write_to_disk(p_index, self);
    if (error != E_SUCCESS) {
        printf("PackedIndex_write_to_disk %d\n", error);
        return error;
    }
    return E_SUCCESS;
}


void        ArchivePage_free(ArchivePage*           self)
{
    HashIndex_free(self->index);
    close(self->fd);
    free(self);
}


bool        ArchivePage_has(ArchivePage*            page,
                            char*                   key)
{
    return HashIndex_has(page->index, key);
}


static Errors       ArchivePage_read_item(ArchivePage*  self,
                                          HashItem*     item,
                                          char**        _data,
                                          size_t*       _data_size) {
    size_t data_size = item->data_size;
    char* data = (char*)malloc(sizeof(char) * data_size);

    // read from file
    Errors error = read_from_file(
        self->fd,
        data,
        data_size,
        (off_t)(self->file_header->data_start + item->data_offset)
    );

    // if error, free data and return
    if (error != E_SUCCESS) {
        free(data);
        return error;
    }

    // assign return pointers
    *_data_size = data_size;
    *_data = data;

    return E_SUCCESS;
}


static Errors       ArchivePage_write_item(ArchivePage* self,
                                           HashItem*    item,
                                           char*        data,
                                           size_t       size) {

    // the item is positioned at the end of the files data section
    item->data_offset = self->file_header->data_size;
    item->data_size = size;

    // write to file
    Errors error = write_to_file(
        self->fd,
        data,
        size,
        (off_t)(self->file_header->data_start + item->data_offset)
    );

    if (error != E_SUCCESS) {
        return error;
    }

    // update the data size
    self->file_header->data_size += size;

    return E_SUCCESS;
}


Errors      ArchivePage_get(ArchivePage*            self,
                            char*                   key,
                            char**                  _data,
                            size_t*                 _data_size) {
    HashItem* item = HashIndex_get(self->index, key);
    if (item == NULL) {
        return E_SUCCESS;
    }
    return ArchivePage_read_item(self, item, _data, _data_size);
}


Errors      ArchivePage_set(ArchivePage*            self,
                            char*                   key,
                            char*                   data,
                            size_t                  size)
{
    // if the page is full, return an error
    if (self->index->items >= MAX_ITEMS_PER_INDEX) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    
    HashItem item;
    HashItem_init_with_key(&item, key, 0, size);
    Errors error = ArchivePage_write_item(self, &item, data, size);
    if (error != E_SUCCESS) {
        return error;
    }
    error = HashIndex_set(self->index, &item);
    return error;
}
