#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "Endian.h"
#include "ArchivePage.h"
#include "HashIndexPack.h"

/**
 * A private packed structure for writing archive's file header to file.
 */
typedef struct __attribute__((__packed__)) ArchiveFileHeader
{
    __uint32_t              version;
    __uint32_t              capacity;
    __uint32_t              n_items;
    __uint32_t              index_start;
    __uint32_t              data_start;
    __uint32_t              data_size;
} ArchiveFileHeader;


typedef enum ArchiveFileVersion {
    ArchiveFileVersion1 = 1,
} ArchiveFileVersion;


static size_t ArchivePage_index_start = sizeof(ArchiveFileHeader);
static size_t ArchivePage_data_start = sizeof(ArchiveFileHeader) + (_MAX_ITEMS_PER_INDEX * sizeof(PackedHashItem));
static size_t ArchivePage_capacity = _MAX_ITEMS_PER_INDEX;


#pragma mark Read / Write


static inline Errors    write_to_file(file_descriptor   fd,
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


static inline Errors    read_from_file(file_descriptor  fd,
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


#pragma mark ArchivePage Header Deserialization


/**
 Reads the archive file's index data.

 @param self The archive page.
 @param n_items The number of items in the index.
 @return An error code.
 */
static inline Errors    ArchivePage_read_file_index(ArchivePage*        self,
                                                    size_t              n_items)
{
    // read packed hash items
    size_t p_items_size = sizeof(PackedHashItem) * n_items;
    PackedHashItem* p_items = (PackedHashItem*)malloc(p_items_size);
    Errors error = read_from_file(
        self->fd,
        p_items,
        p_items_size,
        ArchivePage_index_start
    );
    if (error != E_SUCCESS) {
        free(p_items);
        return error;
    }
    
    // load packed items to the index
    HashIndex_unpack(self->index, p_items, n_items);
    
    free(p_items);
    
    return E_SUCCESS;
}


/**
 Reads the archive file's header and index data.

 @param self The archive page.
 @return An error code.
 */
static inline Errors    ArchivePage_read_file_header(ArchivePage*       self)
{
    Errors error;
    
    // read ArchiveFileHeader from file
    ArchiveFileHeader file_header;
    error = read_from_file(self->fd, &file_header, sizeof(ArchiveFileHeader), 0);
    if (error != E_SUCCESS) {
        return error;
    }
    
    // check data consistency
    if (be32toh(file_header.version) != ArchiveFileVersion1) {
        return E_UNKNOWN_ARCHIVE_VERSION;
    }

    // enforce the right endianness
    size_t capacity     = be32toh(file_header.capacity);
    size_t n_items      = be32toh(file_header.n_items);
    size_t index_start  = be32toh(file_header.index_start);
    size_t data_start   = be32toh(file_header.data_start);
    size_t data_size    = be32toh(file_header.data_size);

    // check data consistency
    if (index_start != ArchivePage_index_start ||
        data_start != ArchivePage_data_start ||
        capacity != ArchivePage_capacity ||
        n_items > capacity) {
        return E_INVALID_ARCHIVE_HEADER;
    }
    
    // populate the ArchivePage fields
    self->data_size = data_size;
    self->has_changes = false;
    
    // read index
    error = ArchivePage_read_file_index(self, n_items);

    return error;
}


#pragma mark ArchivePage Header Serialization


static inline void      ArchivePage_dump_file_header(const ArchivePage* self,
                                                     void*              buf)
{
    ArchiveFileHeader file_header;
    file_header.version     = htobe32(ArchiveFileVersion1);
    file_header.capacity    = htobe32((__uint32_t)ArchivePage_capacity);
    file_header.n_items     = htobe32((__uint32_t)self->index->n_items);
    file_header.index_start = htobe32((__uint32_t)ArchivePage_index_start);
    file_header.data_start  = htobe32((__uint32_t)ArchivePage_data_start);
    file_header.data_size   = htobe32((__uint32_t)self->data_size);
    memcpy(buf, &file_header, sizeof(ArchiveFileHeader));
}


/**
 Write the file header to the archive's file descriptor.

 @param self The archive.
 @return An error code.
 */
static inline Errors    ArchivePage_write_file_header(const ArchivePage* self)
{
    Errors error;
    
    // allocate a buffer to write the full header + index
    // use calloc to make sure we fill up empty space with 0 in the file
    size_t header_size = sizeof(ArchiveFileHeader) +
                         (sizeof(PackedHashItem) * ArchivePage_capacity);
    void* buf = calloc(header_size, 1);
    
    // write the header
    ArchivePage_dump_file_header(self, buf);
    
    // write the index
    size_t n_items;
    error = HashIndex_pack(self->index, (PackedHashItem*)(buf + ArchivePage_index_start), ArchivePage_capacity, &n_items);
    if (error != E_SUCCESS) {
        free(buf);
        return error;
    }
    
    // write to file
    error = write_to_file(self->fd, buf, header_size, 0);
    if (error != E_SUCCESS) {
        free(buf);
        return error;
    }
    
    free(buf);
    return E_SUCCESS;
}


#pragma mark ArchivePage Private Helpers


/**
 Open the archive's file descriptor.

 @param self The archive.
 @return An error code.
 */
static inline Errors    ArchivePage_open_file(ArchivePage*      self,
                                              bool              new_file)
{
    // open a (new) file for read and write with use ownership
    file_descriptor fd;
    if (new_file) {
        fd = open(self->filename , O_NONBLOCK | O_EXCL | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    } else {
        fd = open(self->filename , O_NONBLOCK | O_RDWR, S_IRUSR | S_IWUSR);
    }
    if (fd < 0) {
        return E_SYSTEM_ERROR_ERRNO;
    }

    // get a lock and don't wait for it
    int lock = flock(fd, LOCK_SH | LOCK_NB);
    if (lock < 0) {
        return E_SYSTEM_ERROR_ERRNO;
    }

    // store the file descriptor
    self->fd = fd;

    return E_SUCCESS;
}


/**
 Close the archive's file descriptor.

 @param self The archive.
 */
static inline void      ArchivePage_close_file(ArchivePage*         self)
{
    flock(self->fd, LOCK_UN);
    close(self->fd);
    self->fd = (-1);
}


static Errors           ArchivePage_read_item(const ArchivePage*    self,
                                              const HashItem*       item,
                                              char**                _data,
                                              size_t*               _data_size)
{
    size_t data_size = item->data_size;
    char* data = (char*)malloc(sizeof(char) * data_size);

    // read from file
    Errors error = read_from_file(
        self->fd,
        data,
        data_size,
        (off_t)(ArchivePage_data_start + item->data_offset)
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


static Errors       ArchivePage_write_item(ArchivePage*     self,
                                           const char*      data,
                                           size_t           size,
                                           size_t*          _data_offset)
{

    // the item is positioned at the end of the files data section
    size_t offset = self->data_size;

    // write to file
    Errors error = write_to_file(
        self->fd,
        data,
        size,
        (off_t)(ArchivePage_data_start + offset)
    );

    if (error != E_SUCCESS) {
        return error;
    }

    // update the data size
    self->data_size += size;
    
    // set return pointer
    *_data_offset = offset;

    return E_SUCCESS;
}


#pragma mark ArchivePage Public Functions


Errors      ArchivePage_init(ArchivePage*           self,
                             const char*            filename,
                             bool                   new_file)
{
    Errors error;

    // copy filename to the struct
    size_t filename_size = strlen(filename) + 1;
    self->filename = (char*)malloc(filename_size);
    memcpy(self->filename, filename, filename_size);

    // open file descriptor
    error = ArchivePage_open_file(self, new_file);
    if (error != E_SUCCESS) {
        printf("File not opened, error = %s\n", strerror(errno));
        free(self->filename);
        self->filename = NULL;
        return error;
    }
    
    // allocates and inits the index
    self->index = (HashIndex*)malloc(sizeof(HashIndex));
    HashIndex_init(self->index);

    // loads file header and index
    if (new_file) {
        self->data_size = 0;
        self->has_changes = true;
    } else {
        error = ArchivePage_read_file_header(self);
        if (error != E_SUCCESS) {
            ArchivePage_close_file(self);
            free(self->index);
            free(self->filename);
            self->index = NULL;
            self->filename = NULL;
            return error;
        }
    }
        
    return E_SUCCESS;
}


void        ArchivePage_free(ArchivePage*           self)
{
    ArchivePage_close_file(self);
    HashIndex_free(self->index);
    free(self->index);
    free(self->filename);
    self->index = NULL;
    self->filename = NULL;
}


Errors      ArchivePage_save(ArchivePage*           self)
{
    Errors error;
    
    // skip write if there are no changes
    if (!self->has_changes) {
        return E_SUCCESS;
    }
    
    // write the file header
    error = ArchivePage_write_file_header(self);
    if (error != E_SUCCESS) {
        printf("An error when saving page, error = %d\n", error);
        return error;
    }
    self->has_changes = false;
    return E_SUCCESS;
}


bool        ArchivePage_has(const ArchivePage*      page,
                            const char*             key)
{
    return HashIndex_has(page->index, key);
}


Errors      ArchivePage_get(const ArchivePage*      self,
                            const char*             key,
                            char**                  _data,
                            size_t*                 _data_size)
{
    const HashItem* item = HashIndex_get(self->index, key);
    if (item == NULL) {
        return E_NOT_FOUND;
    }
    return ArchivePage_read_item(self, item, _data, _data_size);
}


Errors      ArchivePage_set(ArchivePage*            self,
                            const char*             key,
                            const char*             data,
                            size_t                  size)
{
    // if the page is full, return an error
    if (self->index->n_items >= MAX_ITEMS_PER_INDEX) {
        return E_INDEX_MAX_SIZE_EXCEEDED;
    }
    
    size_t offset;
    Errors error = ArchivePage_write_item(self, data, size, &offset);
    if (error != E_SUCCESS) {
        return error;
    }
    error = HashIndex_set(self->index, key, offset, size);
    if (error != E_SUCCESS) {
        return error;
    }
    self->has_changes = true;
    return E_SUCCESS;
}
