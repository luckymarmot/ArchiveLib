#ifndef ARCHIVESAVERESULT_H
#define ARCHIVESAVERESULT_H
#include <stdbool.h>


typedef struct ArchiveSaveFile {
    char*						filename;
    bool                        has_changes;
} ArchiveSaveFile;


typedef struct ArchiveSaveResult {
    ArchiveSaveFile*            files;
    size_t                      count;
} ArchiveSaveResult;


static inline void ArchiveSaveResult_free(ArchiveSaveResult*	result)
{
    ArchiveSaveFile* file;
    size_t count = result->count;
    size_t i;
    for (i = 0; i < count; i++) {
        file = &(result->files[i]);
        free(file->filename);
    }
    free(result->files);
    result->files = NULL;
    result->count = 0;
}


static inline void ArchiveSaveResult_print(ArchiveSaveResult*	result)
{
    ArchiveSaveFile* file;
    size_t count = result->count;
    int i;
    for (i = 0; i < count; i++) {
        file = &(result->files[i]);
        printf("Archive file [%s] = %s\n", (file->has_changes ? " changed " : "no change"), file->filename);
    }
}

#endif /* ARCHIVESAVERESULT_H */
