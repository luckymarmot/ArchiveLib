#ifndef ARCHIVELIB_ARCHIVELAYER_H
#define ARCHIVELIB_ARCHIVELAYER_H


#include "Errors.h"
#include "HashIndex.h"


/**
 *
 * A type for file descriptors. It is nicer like this than as an `int`.
 * We use pread and pwrite so cant use the fopen but rather open.
 *
 * We don't care about Windows NT compatibility.
 */
typedef int file_descriptor;


/**
 *
 *  ArchivePage for a given disk file
 *  Store the header, the data offset
 *  from were the data starts in the file,
 *  the size of the file.
 *
 */
typedef struct ArchivePage
{
    HashIndex*              index;
    size_t                  data_size;
    file_descriptor         fd;
    char*                   filename;
} ArchivePage;


/**
 Initializes a new archive page.

 @param self The archive page.
 @param filename The filename of the archive.
 @param new_file Whether the archive is a new file.
 @return An error code.
 */
Errors      ArchivePage_init(ArchivePage*           self,
                             char*                  filename,
                             bool                   new_file);


/**
 Free the inside structures of the archive page.

 @param self The archive page.
 */
void        ArchivePage_free(ArchivePage*           self);


/**
 Saves the archive page to the file system.

 @param self The archive page.
 @return An error code.
 */
Errors      ArchivePage_save(ArchivePage*           self);


/**
 Checks if a given key is inside the archive page.

 @param page The archive page.
 @param key The key to lookup (a 20 bytes binary string).
 @return An error code.
 */
bool        ArchivePage_has(ArchivePage*            page,
                            char*                   key);


/**
 Retrieve an item from the archive page.

 @param self The archive page.
 @param key The key to lookup (a 20 bytes binary string).
 @param _data A pointer to the char* that will be returned.
              The returned char* should be free'ed by the caller.
              Data is not null-terminated, may be binary. The caller should
              rely on the `_data_size` value to know how many bytes to read.
 @param _data_size A pointer to the size of the read data.
 @return An error code.
 */
Errors      ArchivePage_get(ArchivePage*            self,
                            char*                   key,
                            char**                  _data,
                            size_t*                 _data_size);


/**
 Sets a new item to the archive page.

 @param self The archive.
 @param key The key to set for the new item (a 20 bytes binary string).
 @param data The data to write to the archive.
 @param size The length of the data to write.
 @return An error code.
 */
Errors      ArchivePage_set(ArchivePage*            self,
                            char*                   key,
                            char*                   data,
                            size_t                  size);


#endif //ARCHIVELIB_ARCHIVELAYER_H
