#include "Archive.h"
#include <uuid/uuid.h>

void        Archive_init(Archive*                 self,
                         char*                    base_file_path)
{
    self->n_pages = 0;
    self->page_stack = NULL;
    
    // copy base file path
    size_t base_file_path_len = strlen(base_file_path);
    self->base_file_path = malloc(base_file_path_len);
    memcpy(self->base_file_path, base_file_path, base_file_path_len);
}

void        Archive_free(Archive*                 self)
{
    ArchiveListItem* next = self->page_stack;
    ArchiveListItem* last = NULL;

    while (next != NULL) {
        ArchivePage_free(next->page);
        free(next->page);
        last = next;
        next = next->next;
        free(last);
    }
    
    free(self->base_file_path);
    
    self->n_pages = 0;
    self->base_file_path = NULL;
    self->page_stack = NULL;
}

Errors      Archive_save(Archive*                 self,
                         char***                  _filenames,
                         size_t*                  _n_files)
{
    Errors error;

    char** filenames = (char **)malloc(sizeof(char*) * self->n_pages);
    size_t n_files = self->n_pages;

    ArchiveListItem* next = self->page_stack;
    int i = 0;
    while (next != NULL) {
        printf("In loop %d\n %p %s\n", i, next->page, next->page->filename);
        error = ArchivePage_save(next->page);
        if (error != E_SUCCESS) {
            printf("Failed to save!!! %d\n", error);
            printf("File not saved ERROR: %s\n", strerror(errno));
            return error;
        }
        filenames[i] = next->page->filename;
        i += 1;
        next = next->next;
    }
    
    // set return pointers
    *_filenames = filenames;
    *_n_files = n_files;
    
    return E_SUCCESS;
}


static inline Errors    Archive_add_page(Archive*       self,
                                         char*          filename,
                                         bool           new_file)
{
    // build full file path
    char* filepath;
    asprintf(&filepath, "%s%s", self->base_file_path, filename);
    
    // log
    printf("Add archive page, new? = %d, filename = %s\n", new_file, filename);

    // create the page struct
    ArchivePage* page = (ArchivePage*)malloc(sizeof(ArchivePage));
    Errors error = ArchivePage_init(page, filepath, new_file);
    if (error != E_SUCCESS) {
        free(filepath);
        free(page);
        return error;
    }
    
    // free filepath
    free(filepath);
    filepath = NULL;

    // add page to the list
    ArchiveListItem* list_item = (ArchiveListItem*)malloc(sizeof(ArchiveListItem));
    list_item->next = self->page_stack;
    list_item->page = page;
    self->page_stack = list_item;
    self->n_pages += 1;
    
    return E_SUCCESS;
}


Errors      Archive_add_page_by_name(Archive*     self,
                                     char*        filename)
{
    return Archive_add_page(self, filename, false);
}


Errors      Archive_add_empty_page(Archive*       self)
{
    // generate filename uuid
    uuid_t uuid;
    uuid_generate_random(uuid);
    char filename[37]; // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
    uuid_unparse_lower(uuid, filename);
    
    return Archive_add_page(self, filename, true);
}


bool        Archive_has(Archive*                  self,
                        char*                     key)
{
    ArchiveListItem* item = self->page_stack;
    while (item != NULL) {
        if (ArchivePage_has(item->page, key)) {
            return true;
        }
        item = item->next;
    }
    return false;
}


Errors      Archive_get(Archive*                  self,
                        char*                     key,
                        char**                    _data,
                        size_t*                   _data_size)
{
    Errors error;
    ArchiveListItem* item = self->page_stack;
    while (item != NULL) {
        error = ArchivePage_get(item->page, key, _data, _data_size);
        if (error == E_SUCCESS) {
            return E_SUCCESS;
        }
        if (error < 0) {
            return error;
        }
        item = item->next;
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
        error = Archive_add_empty_page(self);
        if (error != E_SUCCESS) {
            return error;
        }
        error =  ArchivePage_set(self->page_stack->page, key, data, size);
    }

    return error;
}
