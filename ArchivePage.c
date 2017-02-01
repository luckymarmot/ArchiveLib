//
// Created by Matthaus Woolard on 27/01/2017.
//

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "ArchivePage.h"

Errors write_to_file(
        file_descriptor fd,
        const void* buffer,
        size_t size,
        off_t offset) {
    off_t writen = 0;
    ssize_t r;
    while (writen < size) {
        r = pwrite(fd, buffer + writen, size - writen, offset + writen);
        if (r < 0) {
            return E_SYSTEM_ERROR_ERRNO;
        }
        writen += r;
    }
    return E_SUCCESS;
}



Errors read_from_file(
        file_descriptor fd,
        void* buffer,
        size_t size,
        off_t offset) {
    off_t read = 0;
    ssize_t r;
    while (read < size) {
        r = pread(fd, buffer + read, size - read, offset + read);
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
typedef struct {
    size_t capacity;
    size_t saved_items;
    HashItem items[];
} PackedIndex;

/**
 *
 * Build Packed Index
 * Do not copy in any data yet
 *
 */
PackedIndex* PackedIndex__init__(size_t items) {
    // allocate the memory needed
    PackedIndex* p_index = (PackedIndex*) (
            malloc(sizeof(PackedIndex) + sizeof(HashItem)*items)
    );
    p_index->saved_items = 0;
    p_index->capacity = items;
    return p_index;
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
            memcpy(&packed_index->items[items_saved], &page->items[0], sizeof(HashItem)*page->length);
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
 * @return
 */
void PackedIndex_unpack(PackedIndex* self, HashIndex* index) {
    // Packed indexes are saved in a sequence so a while loop
    // is best here to reduce lookups;
    size_t current_item_index = 0;
    HashItem* current_item;
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
        HashPage_set(current_page, current_item);
        current_item_index += 1;
    }
    index->items = current_item_index;
}


Errors PackedIndex_write_to_disk(PackedIndex* self, ArchivePage* archive) {
    ArchiveFileHeader* header = archive->file_header;
    size_t size = (self->capacity * sizeof(HashItem)) + sizeof(PackedIndex);
    if ( size > header->header_size) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    return write_to_file(archive->data_file, self, size, (off_t) header->header_start);
}


Errors PackedIndex_read_size(PackedIndex* self, ArchivePage* archive) {
    ArchiveFileHeader* header = archive->file_header;
    // Read in 2 parts just read the first git of the data first
    return read_from_file(
            archive->data_file,
            self,
            sizeof(PackedIndex),
            (off_t) header->header_start
    );
}


Errors PackedIndex_read_from_disk(PackedIndex* self, ArchivePage* archive) {
    ArchiveFileHeader* header = archive->file_header;
    size_t size = (self->capacity * sizeof(HashItem)) + sizeof(PackedIndex);
    if ( size > header->header_size) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    return read_from_file(
            archive->data_file,
            self,
            size,
            (off_t) header->header_start
    );
}




/**
 *
 *
 *
 * Open a file descriptor for an archive. Load the index and read out the size.
 *
 * @param self
 * @param filename
 * @return
 */
Errors ArchivePage_open_file(ArchivePage* self, char filename[]) {
    // Open a (new) file for read and write with use ownership.
    self->data_file = open ( filename , O_CREAT | O_RDWR, S_IRUSR | S_IWUSR );
    if (self->data_file < 0) {
        return E_SYSTEM_ERROR_ERRNO;
    }
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
    return write_to_file(self->data_file, file_header, sizeof(ArchiveFileHeader), 0);
}



/**
 *
 * Read File Header
 *
 */
Errors ArchivePage_read_file_header(
        ArchivePage* self,
        ArchiveFileHeader* file_header) {
    return read_from_file(self->data_file, file_header, sizeof(ArchiveFileHeader), 0);
}


Errors ArchivePage_init_new_file_header(
        ArchivePage* self,
        ArchiveFileHeader* file_header) {
    file_header->data_size = 0;
    file_header->header_size = sizeof(HashItem) * MAX_ITEMS_PER_INDEX + sizeof(PackedIndex);
    file_header->header_start = sizeof(ArchiveFileHeader);
    file_header->data_start = file_header->header_size + file_header->header_start;
    return ArchivePage_write_file_header(self, file_header);
}


Errors ArchivePage__init__(
        ArchivePage* self,
        HashIndex* index,
        char* filename, bool new_file) {
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
        HashIndex__init__(index);
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
        p_index = (PackedIndex*) (malloc(sizeof(PackedIndex)));
        p_index->capacity = capacity;
        error = PackedIndex_read_from_disk(p_index, self);
        if (error != E_SUCCESS) {
            return error;
        }
        HashIndex__init__(index);
        PackedIndex_unpack(p_index, index);
        free(p_index);
        self->index = index;
    }
    return E_SUCCESS;
    // Read the header...
}


Errors ArchivePage_save_to_disk(ArchivePage* self) {
    Errors error = ArchivePage_write_file_header(
            self, self->file_header
    );
    if (error != E_SUCCESS) {
        printf("ArchivePage_write_file_header %d\n", error);
        return error;
    }
    PackedIndex* p_index = PackedIndex__init__(self->index->items);
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

void ArchivePage_free(ArchivePage* self) {
    HashIndex_free(self->index);
    close(self->data_file);
    free(self);
}


bool ArchivePage_has(ArchivePage* page, char* key) {
    return HashIndex_has(page->index, key);
}

Errors ArchivePage_read(ArchivePage* self, HashItem* item, char** data) {
    *data = (char*) (malloc((sizeof(char)*item->data_size)));
    Errors error = read_from_file(
            self->data_file,
            data,
            item->data_size,
            (off_t) (self->file_header->data_start + item->data_offset)
    );
    if (error != E_SUCCESS) {
        free(*data);
        return error;
    }
    return E_FOUND;
}


Errors ArchivePage_write(
        ArchivePage* self,
        HashItem* item,
        char* data,
        size_t size) {

    // The item is positioned at the end of the files data section
    item->data_offset = self->file_header->data_size;
    item->data_size = size;

    Errors error = write_to_file(
            self->data_file,
            data,
            size,
            (off_t) (item->data_offset + self->file_header->data_start)
    );

    if (error != E_SUCCESS) {
        return error;
    }

    // Update the data size
    self->file_header->data_size += size;

    return E_SUCCESS;
}

Errors ArchivePage_get(ArchivePage* self, char* key, char** data) {
    HashItem* item = HashIndex_get(self->index, key);
    if (item == NULL) {
        return E_SUCCESS;
    }
    return ArchivePage_read(self, item, data);
}

Errors ArchivePage_set(ArchivePage* self, char* key, char* data, size_t size) {
    if (self->index->items >= MAX_ITEMS_PER_INDEX) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    HashItem* item = (HashItem*) (malloc(sizeof(HashItem)));
    item->data_size = size;
    HashItem__init_with_key__(item, key, 0, size);
    Errors error = ArchivePage_write(self, item, data, size);
    if (error != E_SUCCESS) {
        free(item);
        return error;
    }
    error = HashIndex_set(self->index, item);
    free(item);
    return error;
}