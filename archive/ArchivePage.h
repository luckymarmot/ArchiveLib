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

 @param self The archive page structure.
 @param filename The filename of the archive.
 @param new_file Whether the archive is a new file.
 @return An error code.
 */
Errors      ArchivePage_init(ArchivePage*           self,
                             char*                  filename,
                             bool                   new_file);

void        ArchivePage_free(ArchivePage*           self);

Errors      ArchivePage_save(ArchivePage*           self);

bool        ArchivePage_has(ArchivePage*            page,
                            char*                   key);

Errors      ArchivePage_get(ArchivePage*            self,
                            char*                   key,
                            char**                  _data,
                            size_t*                 _data_size);

Errors      ArchivePage_set(ArchivePage*            self,
                            char*                   key,
                            char*                   data,
                            size_t                  size);

#endif //ARCHIVELIB_ARCHIVELAYER_H
