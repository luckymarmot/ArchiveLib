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
    self->base_file_path = malloc(base_file_path_size);
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
                         char***                  _filenames,
                         size_t*                  _n_files)
{
    Errors error;
    char** filenames = (char **)malloc(sizeof(char*) * self->n_pages);
    size_t n_pages = self->n_pages;

    // save all pages
    for (size_t i = 0; i < n_pages; i++) {
        ArchivePage* page = &(self->pages[i]);
        error = ArchivePage_save(page);
        if (error != E_SUCCESS) {
            printf("Failed to save!!! %d\n", error);
            printf("File not saved ERROR: %s\n", strerror(errno));
            free(filenames);
            return error;
        }
        filenames[i] = page->filename;
    }
    
    // set return pointers
    *_filenames = filenames;
    *_n_files = n_pages;
    
    return E_SUCCESS;
}


static inline Errors    Archive_add_page(Archive*       self,
                                         const char*    filename,
                                         bool           new_file)
{
    // build full file path
    char* filepath;
    asprintf(&filepath, "%s%s", self->base_file_path, filename);
    
    // log
    printf("Add archive page, new? = %d, filename = %s\n", new_file, filename);
    
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


bool        Archive_has(const Archive*            self,
                        const char*               key)
{
    for (long long i = self->n_pages - 1; i >= 0; i--) {
        if (ArchivePage_has(self->pages + i, key)) {
            return true;
        }
    }
    return false;
}


Errors      Archive_get(const Archive*            self,
                        const char*               key,
                        char**                    _data,
                        size_t*                   _data_size)
{
    Errors error;
    
    for (long long i = self->n_pages - 1; i >= 0; i--) {
        // lookup in an archive
        error = ArchivePage_get(self->pages + i, key, _data, _data_size);
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
