#include <stdio.h>
#include <stdbool.h>

#include "Archive.h"


int main() {
    printf("Create 1 data\n");

    Errors error;
    int i;
    char key[20] = {21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    char key2[20] = {124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124};

    // == Writing ==
    
    // init archive and add a first page
    Archive archive;
    Archive_init(&archive, "./");
    error = Archive_add_empty_page(&archive);
    if (error != E_SUCCESS) {
        printf("Error adding empty page: error = %d\n", error);
        return 1;
    }
    
    // add data
    Archive_set(&archive, key, "the data", 8);
    Archive_set(&archive, key2, "the other data", 14);
    for (int i = 0; i < 10000; i++) {
        u_int32_t r = arc4random();
        key2[0] = r & 0xff;
        key2[1] = (r & 0xff00) >> 8;
        key2[2] = (r & 0xff0000) >> 16;
        key2[3] = (r & 0xff000000) >> 24;
        Archive_set(&archive, key2, "[A long data blob]", 18);
    }
    
    // save and get archive names
    size_t n_files;
    char** filenames;
    char new_filenames[256][256];
    Archive_save(&archive, &filenames, &n_files);
    for (i = 0; i < n_files; i++) {
        printf("Archive file = %s\n", filenames[i]);
        strcpy(new_filenames[i], filenames[i]);
    }
    free(filenames);
    filenames = NULL;
    
    // destroy the archive
    Archive_free(&archive);

    // == Reading ==
    
    printf("Reading data\n");
    Archive_init(&archive, "./");
    printf("Init archive\n");
    for (i = 0; i < n_files; i++) {
        error = Archive_add_page_by_name(&archive, new_filenames[i]);
        if (error != E_SUCCESS) {
            printf("Error adding page for file: filename = %s, error = %d\n", new_filenames[i], error);
            return 1;
        }
        printf("Added page %s\n", new_filenames[i]);
    }
    bool found = Archive_has(&archive, key2);
    printf("Found = %d\n", found);
    char* data;
    size_t data_size;
    error = Archive_get(&archive, key2, &data, &data_size);
    printf("Return Code = %d\n", error);
    char* str_err = strerror(errno);
    printf("Error = %s\n", str_err);
    free(str_err);
    printf("Length = %lu, Data = %.*s\n", data_size, (int)data_size, data);
    free(data);
    Archive_free(&archive);

    return 0;
}
