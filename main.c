#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

//
// Created by Matthaus Woolard on 26/01/2017.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include "HashIndex.h"
#include "ArchivePage.h"
#include <errno.h>
#include "Archive.h"


int main() {
    Archive* archive = (Archive*) (malloc(sizeof(Archive)));
    Archive__init__(archive, "./");
    Errors error = Archive_add_page_by_name(archive, "fe644dd2-185c-40d0-acdf-4e255d44331a");
    //Archive_new_page(archive);
    printf("error? %d\n", error);
    char key[20] = {21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    Archive_set(archive, key, "the data", 8);
    char key2[20] = {124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124};
    Archive_set(archive, key2, "the other data", 14);
    ArchiveFiles* files = (ArchiveFiles*) (malloc(sizeof(ArchiveFiles)));
    Archive_save(archive, files);
    for (int i = 0; i < files->n_files; ++i) {
        printf("file %s\n", files->page_filenames[i]);
    }
    Archive_free(archive);
    return 0;
}
