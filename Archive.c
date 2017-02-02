//
// Created by Matthaus Woolard on 31/01/2017.
//

#include "Archive.h"
#include <uuid/uuid.h>

void Archive_init(Archive* self, char* base_file_path) {
    self->n_pages = 0;
    self->page_stack = NULL;
    self->base_file_path = base_file_path;
}

void Archive_free(Archive* self) {
    ArchiveListItem* next = self->page_stack;
    ArchiveListItem* last = NULL;

    while (next != NULL) {
        ArchivePage_free(next->page);
        last = next;
        next = next->next;
        free(last);
    }
}

Errors Archive_save(Archive* self, ArchiveFiles* files) {
    Errors error;
    files->n_files = self->n_pages;
    files->page_filenames = (char **) (malloc(sizeof(char*) * self->n_pages));
    ArchiveListItem* next = self->page_stack;
    int i = 0;
    while (next != NULL) {
        printf("In loop %d\n %p %s\n", i, next->page, next->page->filename);
        error = ArchivePage_save_to_disk(next->page);
        if (error != E_SUCCESS) {
            printf("Failed to save!!! %d\n", error);
            printf("File not saved ERROR: %s\n", strerror(errno));
            return error;
        }
        files->page_filenames[i] = next->page->filename;
        i += 1;
        next = next->next;
    }
    return E_SUCCESS;
}

void Archive_add_page(Archive* self, ArchivePage* page) {
    ArchiveListItem* list_item = (ArchiveListItem*) (malloc(sizeof(ArchiveListItem)));
    list_item->next = self->page_stack;
    list_item->page = page;
    self->page_stack = list_item;
    self->n_pages += 1;
}

Errors Archive_add_page_by_name(Archive* self, char* filename) {
    char *full_path;
    asprintf(&full_path, "%s%s", self->base_file_path, filename);
    printf("file %s\n", full_path);
    ArchivePage* page = (ArchivePage*) malloc(sizeof(ArchivePage));
    HashIndex* index = (HashIndex*) malloc(sizeof(HashIndex));
    Errors error = ArchivePage_init(page, index, full_path, false);
    if (error != E_SUCCESS) {
        free(page);
        free(index);
        return error;
    }
    Archive_add_page(self, page);
    printf("Top page object %p\n", self->page_stack);
    return E_SUCCESS;
}



Errors Archive_new_page(Archive* self) {
    //
    uuid_t uuid;

    // generate
    uuid_generate_random(uuid);

    // un parse (to string)
    char uuid_str[37];      // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
    uuid_unparse_lower(uuid, uuid_str);
    char *full_path;
    asprintf(&full_path, "%s%s", self->base_file_path, uuid_str);
    ArchivePage* page = (ArchivePage*) malloc(sizeof(ArchivePage));
    HashIndex* index = (HashIndex*) malloc(sizeof(HashIndex));
    Errors error = ArchivePage_init(page, index, full_path, true);
    if (error != E_SUCCESS) {
        free(page);
        free(index);
        return error;
    }
    Archive_add_page(self, page);
    return E_SUCCESS;
}


bool        Archive_has(Archive*                  self,
                        char*                     key)
{
    ArchiveListItem* next = self->page_stack;
    while (next != NULL) {
        if (ArchivePage_has(next->page, key)) {
            return true;
        }
        next = next->next;
    }
    return false;
}


Errors      Archive_get(Archive*                  self,
                        char*                     key,
                        char**                    _data,
                        size_t*                   _data_size)
{
    Errors error;
    ArchiveListItem* next = self->page_stack;
    while (next != NULL) {
        error = ArchivePage_get(next->page, key, _data, _data_size);
        if (error == E_SUCCESS) {
            return E_SUCCESS;
        }
        if (error < 0) {
            return error;
        }
        next = next->next;
    }
    return E_NOT_FOUND;
}


Errors      Archive_set(Archive*                  self,
                        char*                     key,
                        char*                     data,
                        size_t                    size)
{
    Errors error;
    
    // if file is already in the archive, consider it a success
    if (Archive_has(self, key)) {
        return E_SUCCESS;
    }
    
    // write to the last page
    error = ArchivePage_set(self->page_stack->page, key, data, size);
    
    // if page is full, add a new page and try again
    if (error == E_INDEX_MAX_SIZE_EXCEEDED) {
        error = Archive_new_page(self);
        if (error != E_SUCCESS) {
            return error;
        }
        error =  ArchivePage_set(self->page_stack->page, key, data, size);
    }

    return error;
}
