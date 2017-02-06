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
#include "ArchiveSaveResult.h"


#pragma mark - Archive


/**
 * Archive object latest pages on the end of the list
 *
 */
typedef struct Archive
{
    char*                       base_file_path;
    ArchivePage*                pages;
    size_t                      n_pages;
    size_t                      capacity;
} Archive;


/**
 Initializes a new archive.

 @param self The archive struct to initialize.
 @param base_file_path The base path for archive files (a null-terminated
                       string).
 */
void            Archive_init(Archive*                   self,
                             const char*                base_file_path);

/**
 Frees the archive's internal structure.

 @param self The archive.
 */
void            Archive_free(Archive*                   self);


/**
 Checks if the archive contains a given partial key, and gets the actual
 key.

 @param self The archive.
 @param partial_key The partial key to lookup (between 3 and 20 bytes).
 @param partial_key_len The length of the partial key.
 @param key A pointer in which the full key (20 bytes) will be written.
            Or NULL, if the user doesn't need to know the full key.
 @return A boolean representing wheather the given key has been found.
 */
bool            Archive_has_partial(const Archive*      self,
                                    const char*         partial_key,
                                    size_t              partial_key_len,
                                    char*               key);

/**
 Checks if the archive contains a given key.
 
 @param self The archive.
 @param key The key to lookup (a 20 bytes binary string).
 @return A boolean representing wheather the given key has been found.
 */
static inline bool  Archive_has(const Archive*          self,
                                const char*             key)
{
    return Archive_has_partial(self, key, 20, NULL);
}


/**
 Retrieve an item from the archive given a partial key.

 @param self The archive.
 @param partial_key The partial key to lookup (between 3 and 20 bytes).
 @param partial_key_len The length of the partial key.
 @param key A pointer in which the full key (20 bytes) will be written.
            Or NULL, if the user doesn't need to know the full key.
 @param data_max_size The maximum number of bytes to read from the file.
                      Pass 0 to read the full file.
 @param _data A pointer to the char* that will be returned.
              The returned char* should be free'ed by the caller.
              Data is not null-terminated, may be binary. The caller should
              rely on the `_data_size` value to know how many bytes to read.
 @param _data_size A pointer to the size of the read data.
 @return An error code.
 */
Errors          Archive_get_partial(const Archive*      self,
                                    const char*         partial_key,
                                    size_t              partial_key_len,
                                    char*               key,
                                    size_t              data_max_size,
                                    char**              _data,
                                    size_t*             _data_size);


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
static inline Errors Archive_get(const Archive*         self,
                                 const char*            key,
                                 char**                 _data,
                                 size_t*                _data_size)
{
    return Archive_get_partial(self, key, 20, NULL, 0, _data, _data_size);
}


/**
 Sets a new item to the archive.

 @param self The archive.
 @param key The key to set for the new item (a 20 bytes binary string).
 @param data The data to write to the archive.
 @param size The length of the data to write.
 @return An error code.
 */
Errors          Archive_set(Archive*                    self,
                            const char*                 key,
                            const char*                 data,
                            size_t                      size);

/**
 Adds a new empty page to the archive.

 @param self The archive.
 @return An error code.
 */
Errors          Archive_add_empty_page(Archive*         self);


/**
 Adds a new page to the archive reading from an existing file.

 @param self The archive.
 @param filename The file path to the existing file.
 @return An error code.
 */
Errors          Archive_add_page_by_name(Archive*       self,
                                         const char*    filename);

/**
 Saves all pages of the archive to the file system.

 @param self The archive.
 @param result A pointer to the result of the save. The caller must
               free the result object using ArchiveSaveResult_free.
 @return An error code.
 */
Errors          Archive_save(const Archive*             self,
                             ArchiveSaveResult*         result);


#endif //ARCHIVELIB_ARCHIVE_H
