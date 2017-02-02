//
// Created by Matthaus Woolard on 26/01/2017.
//

#include <stdio.h>
#include <stdbool.h>
#include "HashIndex.h"
#include "Archive.h"


int main() {
    printf("Create 1 data\n");

    char key[20] = {21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    char key2[20] = {124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124};

    Archive archive;
    Archive_init(&archive, "./");
    Archive_new_page(&archive);
    Archive_set(&archive, key, "the data", 8);
    Archive_set(&archive, key2, "the other data", 14);
    size_t n_files;
    char** filenames;
    Archive_save(&archive, &filenames, &n_files);
    char* filename = NULL;
    for (int i = 0; i < n_files; ++i) {
        filename = filenames[i] + 2;
        printf("file %s\n", filenames[i]);
    }
    free(filenames);
    Archive_free(&archive);

    printf("Reading data\n");
    Archive_init(&archive, "./");
    printf("Init archive\n");
    Archive_add_page_by_name(&archive, filename);
    printf("Added page %s\n", filename);
    bool found = Archive_has(&archive, key);
    printf("Found = %d\n", found);
    char* data;
    size_t data_size;
    Errors error = Archive_get(&archive, key2, &data, &data_size);
    printf("Return Code = %d\n", error);
    printf("Error = %s\n", strerror(errno));
    printf("Length = %lu, Data = %.*s\n", data_size, (int)data_size, data);
    free(data);
    Archive_free(&archive);

    return 0;
}
