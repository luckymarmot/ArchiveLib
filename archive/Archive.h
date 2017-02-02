#ifndef ARCHIVELIB_ARCHIVE_H
#define ARCHIVELIB_ARCHIVE_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "Errors.h"
#include "ArchivePage.h"
#include "HashIndex.h"


typedef struct ArchiveListItem
{
    ArchivePage*                page;
    struct ArchiveListItem*     next;
} ArchiveListItem;


/**
 * Archive object latest pages on the end of the list
 *
 */
typedef struct Archive
{
    size_t                      n_pages;
    char*                       base_file_path;
    ArchiveListItem*            page_stack;
} Archive;


/**
 Initializes a new archive.

 @param self The archive struct to initialize.
 @param base_file_path The base path for archive files (a null-terminated
                       string).
 */
void        Archive_init(Archive*                 self,
                         char*                    base_file_path);

/**
 Frees the archive's internal structure.

 @param self The archive.
 */
void        Archive_free(Archive*                 self);


/**
 Checks if the archive contains a given key.

 @param self The archive.
 @param key The key to lookup (a 20 bytes binary string).
 @return A boolean representing wheather the given key has been found.
 */
bool        Archive_has(Archive*                  self,
                        char*                     key);


/**
 Retrieve an item from the archive.

 @param self The archive.
 @param key The key to lookup (a 20 bytes binary string).
 @param _data A pointer to the char* that will be returned.
              The returned char* should be free'ed by the caller.
              Data is not null-terminated, may be binary. The caller should
              rely on the `_data_size` value to know how many bytes to read.
 @param _data_size A pointer to the size of the read data.
 @return An error code.
 */
Errors      Archive_get(Archive*                  self,
                        char*                     key,
                        char**                    _data,
                        size_t*                   _data_size);


/**
 Sets a new item to the archive.

 @param self The archive.
 @param key The key to set for the new item (a 20 bytes binary string).
 @param data The data to write to the archive.
 @param size The length of the data to write.
 @return An error code.
 */
Errors      Archive_set(Archive*                  self,
                        char*                     key,
                        char*                     data,
                        size_t                    size);

/**
 Adds a new empty page to the archive.

 @param self The archive.
 @return An error code.
 */
Errors      Archive_add_empty_page(Archive*       self);


/**
 Adds a new page to the archive reading from an existing file.

 @param self The archive.
 @param filename The file path to the existing file.
 @return An error code.
 */
Errors      Archive_add_page_by_name(Archive*     self,
                                     char*        filename);

/**
 Saves all pages of the archive to the file system.

 @param self The archive.
 @param _filenames A pointer in which an array of filenames will be written.
                   It's the caller responsibility to free() the char** array
                   that was set to this pointer.
 @param _n_files The number of archive files in the archive.
 @return An error code.
 */
Errors      Archive_save(Archive*                 self,
                         char***                  _filenames,
                         size_t*                  _n_files);


#endif //ARCHIVELIB_ARCHIVE_H
