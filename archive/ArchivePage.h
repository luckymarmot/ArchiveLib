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
    char*                   base_file_path;
    bool                    has_changes;
} ArchivePage;


/**
 Initializes a new archive page.

 @param self The archive page.
 @param filename The filename of the archive.
 @param new_file Whether the archive is a new file.
 @return An error code.
 */
Errors      ArchivePage_init(ArchivePage*           self,
                             const char*            filename,
                             const char*            base_file_name,
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
 Checks if a given partial key is inside the archive page.

 @param page The archive page.
 @param partial_key The partial key to lookup (between 3 and 20 bytes).
 @param partial_key_len The length of the partial key.
 @param key A pointer in which the full key (20 bytes) will be written.
            Or NULL, if the user doesn't need to know the full key.
 @return A boolean representing wheather the given key has been found.
 */
bool        ArchivePage_has(const ArchivePage*      page,
                            const char*             partial_key,
                            size_t                  partial_key_len,
                            char*                   key);


/**
 Retrieve an item from the archive page.

 @param self The archive page.
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
Errors      ArchivePage_get(const ArchivePage*      self,
                            const char*             partial_key,
                            size_t                  partial_key_len,
                            char*                   key,
                            size_t                  data_max_size,
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
                            const char*             key,
                            const char*             data,
                            size_t                  size);


#endif //ARCHIVELIB_ARCHIVELAYER_H
