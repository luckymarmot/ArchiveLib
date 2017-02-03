#include <stdio.h>
#include <stdbool.h>

#include "Archive.h"

static inline void rand_key(char* key)
{
    u_int32_t a = arc4random();
    u_int32_t b = arc4random();
    u_int32_t c = arc4random();
    u_int32_t d = arc4random();
    u_int32_t e = arc4random();
    key[0] = a & 0xff;
    key[1] = (a & 0xff00) >> 8;
    key[2] = (a & 0xff0000) >> 16;
    key[3] = (a & 0xff000000) >> 24;
    key[4 + 0] = b & 0xff;
    key[4 + 1] = (b & 0xff00) >> 8;
    key[4 + 2] = (b & 0xff0000) >> 16;
    key[4 + 3] = (b & 0xff000000) >> 24;
    key[8 + 0] = c & 0xff;
    key[8 + 1] = (c & 0xff00) >> 8;
    key[8 + 2] = (c & 0xff0000) >> 16;
    key[8 + 3] = (c & 0xff000000) >> 24;
    key[12 + 0] = d & 0xff;
    key[12 + 1] = (d & 0xff00) >> 8;
    key[12 + 2] = (d & 0xff0000) >> 16;
    key[12 + 3] = (d & 0xff000000) >> 24;
    key[16 + 0] = e & 0xff;
    key[16 + 1] = (e & 0xff00) >> 8;
    key[16 + 2] = (e & 0xff0000) >> 16;
    key[16 + 3] = (e & 0xff000000) >> 24;
}

Errors _set_data(Archive* archive, char* _key, size_t i)
{
    char value[256];
    rand_key(_key);
    sprintf(value, "[My data, i = %ld]", i);
    size_t size = strlen(value);
    return Archive_set(archive, _key, value, size);
}

Errors _set_data_n(Archive* archive, size_t n_items, char** _keys)
{
    Errors error;
    char* keys = (char*)malloc(sizeof(char) * 20 * n_items);
    for (int i = 0; i < n_items; i++) {
        error = _set_data(archive, keys + (20 * i), i);
        if (error != E_SUCCESS) {
            free(keys);
            return error;
        }
    }
    *_keys = keys;
    return E_SUCCESS;
}

static const char s_key1[20] = {21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
static const char s_key2[20] = {124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124};

static inline Errors _read_key(Archive* archive, const char* key)
{
    Errors error;
    char* data;
    size_t data_size;
    
    error = Archive_get(archive, key, &data, &data_size);
    if (error != E_SUCCESS) {
        return error;
    }
    printf("Length = %lu, Data = %.*s\n", data_size, (int)data_size, data);
    free(data);
    
    return E_SUCCESS;
}

Errors _check_archive(Archive* archive, char* keys, size_t n_items)
{
    Errors error;
    char* data;
    size_t data_size;
    
    // has static key
    if (!Archive_has(archive, s_key1)) {
        printf("Failed on has(s_key1) (not found)\n");
        return E_NOT_FOUND;
    }
    if (!Archive_has(archive, s_key2)) {
        printf("Failed on has(s_key2) (not found)\n");
        return E_NOT_FOUND;
    }
    
    // get static key
    if (E_SUCCESS != (error = _read_key(archive, s_key1))) {
        printf("Failed to load `s_key1`\n");
        return error;
    }
    if (E_SUCCESS != (error = _read_key(archive, s_key2))) {
        printf("Failed to load `s_key2`\n");
        return error;
    }
    
    // has dynamic keys
    for (int i = 0; i < n_items; i++) {
        char* key = keys + (20 * i);
        if (!Archive_has(archive, key)) {
            printf("Failed on has() dynamic key (not found), i = %d\n", i);
            return E_NOT_FOUND;
        }
    }
    
    // get dynamic keys
    for (int i = 0; i < n_items; i++) {
        char* key = keys + (20 * i);
        error = Archive_get(archive, key, &data, &data_size);
        if (error != E_SUCCESS) {
            printf("Failed to get dynamic key, i = %d, error = %d\n", i, error);
            return error;
        }
        free(data);
    }
    
    return E_SUCCESS;
}

Errors _build_archive(char** _filenames, size_t* _n_files, char** _keys, size_t n_items)
{
    Errors error;
    Archive archive;
    
    // init archive and add a first page
    Archive_init(&archive, "./");
    error = Archive_add_empty_page(&archive);
    if (error != E_SUCCESS) {
        printf("Error adding empty page: error = %d\n", error);
        return error;
    }
    
    // add data (with static keys)
    error = Archive_set(&archive, s_key1, "the data", 8);
    if (error != E_SUCCESS) {
        printf("Failed to set static keys, error = %d\n", error);
        return error;
    }
    error = Archive_set(&archive, s_key2, "the other data", 14);
    if (error != E_SUCCESS) {
        printf("Failed to set static keys, error = %d\n", error);
        return error;
    }

    // add data (with many dynamic keys)
    error = _set_data_n(&archive, n_items, _keys);
    if (error != E_SUCCESS) {
        printf("Failed to set many keys, error = %d\n", error);
        return error;
    }
    
    // check archive (before save)
    error = _check_archive(&archive, *_keys, n_items);
    if (error != E_SUCCESS) {
        printf("Failed while checking archive after building, error = %d\n", error);
        Archive_free(&archive);
        return error;
    }
    
    // save the archive
    size_t n_files;
    char** filenames;
    Archive_save(&archive, &filenames, &n_files);
    
    // copy archive names
    char* new_filenames = malloc(sizeof(char) * 256 * n_files);
    for (int i = 0; i < n_files; i++) {
        printf("Archive file = %s\n", filenames[i]);
        strcpy(new_filenames + (256 * i), filenames[i]);
    }
    free(filenames);
    filenames = NULL;
    *_filenames = new_filenames;
    *_n_files = n_files;
    
    // check archive (after save)
    error = _check_archive(&archive, *_keys, n_items);
    if (error != E_SUCCESS) {
        printf("Failed while checking archive after saving, error = %d\n", error);
        Archive_free(&archive);
        return error;
    }
    
    // destroy the archive
    Archive_free(&archive);
    
    return E_SUCCESS;
}

Errors _read_archive(char* filenames, size_t n_files, char* keys, size_t n_items)
{
    Errors error;
    Archive archive;

    // init archive
    printf("Init archive\n");
    Archive_init(&archive, "./");
    
    // add pages
    for (int i = 0; i < n_files; i++) {
        char* filename = filenames + (256 * i);
        printf("Add page %s\n", filename);
        error = Archive_add_page_by_name(&archive, filename);
        if (error != E_SUCCESS) {
            printf("Error adding page for file: filename = %s, error = %d\n", filename, error);
            Archive_free(&archive);
            return error;
        }
    }
    
    // check archive
    error = _check_archive(&archive, keys, n_items);
    if (error != E_SUCCESS) {
        printf("Failed while checking archive after loading from file, error = %d\n", error);
        Archive_free(&archive);
        return error;
    }
    
    // free archive
    Archive_free(&archive);
    
    return E_SUCCESS;
}

int main()
{
    Errors error;
    size_t n_items = 100000;

    // == Writing ==
    
    char* filenames;
    char* keys;
    size_t n_files;
    error = _build_archive(&filenames, &n_files, &keys, n_items);
    if (error != E_SUCCESS) {
        printf("Failed building archive\n");
        return 1;
    }

    // == Reading ==
    error = _read_archive(filenames, n_files, keys, n_items);
    if (error != E_SUCCESS) {
        printf("Failed reading archive\n");
        free(filenames);
        free(keys);
        return 2;
    }
    
    free(filenames);
    free(keys);
    
    return 0;
}
