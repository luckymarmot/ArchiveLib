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


typedef struct ArchiveFiles
{
    size_t                      n_files;
    char**                      page_filenames;
} ArchiveFiles;

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


void        Archive_free(Archive*                 self);

void        Archive_init(Archive*                 self,
                         char*                    base_file_path);

bool        Archive_has(Archive*                  self,
                        char*                     key);

Errors      Archive_get(Archive*                  self,
                        char*                     key,
                        char**                    _data,
                        size_t*                   _data_size);

Errors      Archive_set(Archive*                  self,
                        char*                     key,
                        char*                     data,
                        size_t                    size);

Errors      Archive_new_page(Archive*             self);

Errors      Archive_add_page_by_name(Archive*     self,
                                     char*        filename);

void        Archive_add_page(Archive*             self,
                             ArchivePage*         page);

Errors      Archive_save(Archive*                 self,
                         ArchiveFiles*            files);

#endif //ARCHIVELIB_ARCHIVE_H
