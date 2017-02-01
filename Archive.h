//
// Created by Matthaus Woolard on 31/01/2017.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "ArchivePage.h"
#include "HashIndex.h"

#ifndef ARCHIVELIB_ARCHIVE_H
#define ARCHIVELIB_ARCHIVE_H


typedef struct ArchiveListItem {
    ArchivePage* page;
    struct ArchiveListItem* next;
} ArchiveListItem;


typedef struct ArchiveFiles {
    size_t n_files;
    char** page_filenames;
} ArchiveFiles;

/**
 * Archive object latest pages on the end of the list
 *
 */
typedef struct Archive {
    size_t n_pages;
    char* base_file_path;
    ArchiveListItem* page_stack;
} Archive;

bool Archive_has(Archive* self, char* key);
Errors Archive_get(Archive* self, char* key, char** data);
Errors Archive_set(Archive* self, char* key, char* data, size_t size);
Errors Archive_new_page(Archive* self);
Errors Archive_add_page_by_name(Archive* self, char* filename);
void Archive_add_page(Archive* self, ArchivePage* page);
Errors Archive_save(Archive* self, ArchiveFiles* files);
void Archive_free(Archive* self);
void Archive__init__(Archive* self, char* base_file_path);

#endif //ARCHIVELIB_ARCHIVE_H
