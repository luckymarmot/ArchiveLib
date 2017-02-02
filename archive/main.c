//
// Created by Matthaus Woolard on 26/01/2017.
//

#include <stdio.h>
#include <stdbool.h>
#include "HashIndex.h"
#include "Archive.h"


int main() {
    printf("Create 1 data\n");

    Errors error;
    char key[20] = {21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    char key2[20] = {124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124};

    Archive archive;
    Archive_init(&archive, "./");
    error = Archive_add_empty_page(&archive);
    if (error != E_SUCCESS) {
        printf("Error adding empty page: error = %d\n", error);
        return 1;
    }
    Archive_set(&archive, key, "the data", 8);
    Archive_set(&archive, key2, "the other data", 14);
    size_t n_files;
    char** filenames;
    Archive_save(&archive, &filenames, &n_files);
    char filename[256];
    memcpy(filename, filenames[n_files - 1] + 2, strlen(filenames[n_files - 1]) - 1);
    printf("Archive file = %s (len = %ld)\n", filename, strlen(filename));
    free(filenames);
    Archive_free(&archive);

    printf("Reading data\n");
    Archive_init(&archive, "./");
    printf("Init archive\n");
    error = Archive_add_page_by_name(&archive, filename);
    if (error != E_SUCCESS) {
        printf("Error adding page for file: filename = %s, error = %d\n", filename, error);
        return 1;
    }
    printf("Added page %s\n", filename);
    bool found = Archive_has(&archive, key);
    printf("Found = %d\n", found);
    char* data;
    size_t data_size;
    error = Archive_get(&archive, key2, &data, &data_size);
    printf("Return Code = %d\n", error);
    printf("Error = %s\n", strerror(errno));
    printf("Length = %lu, Data = %.*s\n", data_size, (int)data_size, data);
    free(data);
    Archive_free(&archive);

    return 0;
}
