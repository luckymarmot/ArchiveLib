//
// Created by Matthaus Woolard on 26/01/2017.
//

#include <stdio.h>
#include <stdbool.h>
#include "HashIndex.h"
#include "Archive.h"


int main() {
    printf("Create 1 data\n");

    Archive* archive = (Archive*) (malloc(sizeof(Archive)));
    Archive__init__(archive, "./");
    Archive_new_page(archive);
    char key[20] = {21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    Archive_set(archive, key, "the data", 8);
    char key2[20] = {124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124};
    Archive_set(archive, key2, "the other data", 14);
    ArchiveFiles* files = (ArchiveFiles*) (malloc(sizeof(ArchiveFiles)));
    Archive_save(archive, files);
    char* filename;
    for (int i = 0; i < files->n_files; ++i) {
        filename = files->page_filenames[i] + 2;
        printf("file %s\n", files->page_filenames[i]);
    }
    Archive_free(archive);

    printf("Reading data\n");
    archive = (Archive*) (malloc(sizeof(Archive)));
    printf("Allocated archive\n");
    Archive__init__(archive, "./");
    printf("Init archive\n");
    Archive_add_page_by_name(archive, filename);
    printf("Added page %s\n", filename);
    bool found = Archive_has(archive, key);
    printf("Found?%d\n", found);
    char* data = (char*) (malloc(sizeof(char)));
    Errors error = Archive_get(archive, key2, &data);
    printf("ERROR: %s\n", strerror(errno));
    printf("data? %s\n", &data);

    //Archive_free(archive);

    return 0;
}
