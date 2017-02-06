#include "Archive.h"
#include <uuid/uuid.h>


void        Archive_init(Archive*                 self,
                         const char*              base_file_path)
{
    size_t capacity = 10;
    self->n_pages = 0;
    self->capacity = capacity;
    self->pages = (ArchivePage*)malloc(sizeof(ArchivePage) * capacity);

    // copy base file path
    size_t base_file_path_size = strlen(base_file_path) + 1;
    self->base_file_path = (char*)malloc(base_file_path_size);
    memcpy(self->base_file_path, base_file_path, base_file_path_size);
}


void        Archive_free(Archive*                 self)
{
    // free all pages
    size_t n_pages = self->n_pages;
    for (size_t i = 0; i < n_pages; i++) {
        ArchivePage_free(&(self->pages[i]));
    }
    free(self->pages);
    
    // free file path string
    free(self->base_file_path);
    
    // set null pointers
    self->base_file_path = NULL;
    self->pages = NULL;
    self->n_pages = 0;
    self->capacity = 0;
}


Errors      Archive_save(const Archive*           self,
                         ArchiveSaveResult*       result)
{
    Errors error;
    ArchivePage* page;
    ArchiveSaveFile* file;
    size_t filename_size;
    bool has_changes;
    size_t n_pages = self->n_pages;
    result->count = 0;
    result->files = NULL;
    
    // if empty, we're done
    if (n_pages == 0) {
        return E_SUCCESS;
    }
    
    // alloc a number of ArchiveSaveFile
    result->files = (ArchiveSaveFile*)malloc(sizeof(ArchiveSaveFile) * n_pages);

    // save all pages
    for (size_t i = 0; i < n_pages; i++) {
        page = self->pages + i;
        file = result->files + i;
        has_changes = page->has_changes;
        
        // save page
        error = ArchivePage_save(page);
        if (error != E_SUCCESS) {
            printf("Failed to save!!! %d\n", error);
            printf("File not saved ERROR: %s\n", strerror(errno));
            ArchiveSaveResult_free(result);
            return error;
        }
        
        // copy the filename
        filename_size = strlen(page->filename) + 1;
        file->filename = (char*)malloc(filename_size);
        memcpy(file->filename, page->filename, filename_size);
        
        // set has changes flag
        file->has_changes = has_changes;
        
        // increment the result count
        result->count++;
    }
    
    return E_SUCCESS;
}


static inline Errors    Archive_add_page(Archive*       self,
                                         const char*    filename,
                                         bool           new_file)
{
    // build full file path
    char* filepath;
    asprintf(&filepath, "%s%s", self->base_file_path, filename);
    
    // make sure we have enough space, or we realloc
    if (self->n_pages >= self->capacity) {
        size_t new_capacity = self->capacity * 2;
        self->pages = (ArchivePage*)realloc(self->pages, sizeof(ArchivePage) * new_capacity);
        self->capacity = new_capacity;
    }

    // create the page struct
    ArchivePage* page = &(self->pages[self->n_pages]);
    Errors error = ArchivePage_init(page, filepath, new_file);
    if (error != E_SUCCESS) {
        free(filepath);
        return error;
    }
    
    // increment number of pages
    self->n_pages += 1;
    
    // free filepath
    free(filepath);
    filepath = NULL;
   
    return E_SUCCESS;
}


Errors      Archive_add_page_by_name(Archive*     self,
                                     const char*  filename)
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


bool                Archive_has_partial(const Archive*      self,
                                        const char*         partial_key,
                                        size_t              partial_key_len,
                                        char*               key)
{
    for (long long i = self->n_pages - 1; i >= 0; i--) {
        if (ArchivePage_has(self->pages + i, partial_key, partial_key_len, key)) {
            return true;
        }
    }
    return false;
}


Errors              Archive_get_partial(const Archive*      self,
                                        const char*         partial_key,
                                        size_t              partial_key_len,
                                        char*               key,
                                        size_t              data_max_size,
                                        char**              _data,
                                        size_t*             _data_size)
{
    Errors error;
    
    for (long long i = self->n_pages - 1; i >= 0; i--) {
        // lookup in an archive
        error = ArchivePage_get(self->pages + i, partial_key, partial_key_len, key, data_max_size, _data, _data_size);
        // if success or an error that isn't "not found" stop
        if (error != E_NOT_FOUND) {
            return error;
        }
    }

    return E_NOT_FOUND;
}


Errors      Archive_set(Archive*                  self,
                        const char*               key,
                        const char*               data,
                        size_t                    size)
{
    Errors error;
    
    // if file is already in the archive, consider it a success
    if (Archive_has(self, key)) {
        return E_SUCCESS;
    }
    
    // write to the last page
    error = ArchivePage_set(&(self->pages[self->n_pages - 1]), key, data, size);
    
    // if page is full, add a new page and try again
    if (error == E_INDEX_MAX_SIZE_EXCEEDED) {
        error = Archive_add_empty_page(self);
        if (error != E_SUCCESS) {
            return error;
        }
        error =  ArchivePage_set(&(self->pages[self->n_pages - 1]), key, data, size);
    }

    return error;
}
