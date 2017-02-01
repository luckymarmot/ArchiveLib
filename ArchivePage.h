//
// Created by Matthaus Woolard on 27/01/2017.
//

#include "HashIndex.h"

#ifndef ARCHIVELIB_ARCHIVELAYER_H
#define ARCHIVELIB_ARCHIVELAYER_H

/**
 *
 * file_descriptor type is nicer like this than as an it.
 * We use pread and pwrite so cant use the fopen but rather open.
 *
 * We dont cane about Windows/NT so this is no issue :)
 */
typedef int file_descriptor;


typedef struct ArchiveFileHeader {
    unsigned short version;
    size_t header_start;
    size_t header_size;
    size_t data_start;
    size_t data_size;
} ArchiveFileHeader;

/**
 *
 *  ArchivePage for a given disk file
 *  Store the header, the data offset
 *  from were the data starts in the file,
 *  the size of the file.
 *
 */
typedef struct ArchivePage {
    HashIndex* index;
    ArchiveFileHeader* file_header;
    file_descriptor data_file;
    char* filename;
} ArchivePage;



Errors ArchivePage__init__(
        ArchivePage* self,
        HashIndex* index,
        char* filename,
        bool new_file
);
Errors ArchivePage_save_to_disk(ArchivePage* self);
void ArchivePage_free(ArchivePage* self);
bool ArchivePage_has(ArchivePage* page, char* key);
Errors ArchivePage_get(ArchivePage* self, char* key, char** data);
Errors ArchivePage_set(ArchivePage* self, char* key, char* data, size_t size);

#endif //ARCHIVELIB_ARCHIVELAYER_H
