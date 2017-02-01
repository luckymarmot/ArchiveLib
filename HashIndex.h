//
// Created by Matthaus Woolard on 27/01/2017.
//




#ifndef ARCHIVELIB_HASHINDEX_H
#define ARCHIVELIB_HASHINDEX_H
static const size_t MAX_ITEMS_PER_INDEX = 2000;


typedef enum Errors
{
    E_FOUND=1,
    E_SUCCESS = 0,
    E_SYSTEM_ERROR_ERRNO = -1,
    E_INDEX_MAX_SIZE_EXCEEDED = -2,
    E_INDEX_OUT_OF_BOUNDS = -3,
    E_FILE_READ_ERROR = -4
} Errors;



/**
 *
 * A HashItem that maps a key (20 bytes)
 * to a pointer in the data part of your archvie
 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error "Compile with Little Endian"
#endif
typedef struct {
    char key[20];
    size_t data_offset;
    size_t data_size;
} HashItem;

typedef struct __attribute__((__packed__)){
    char key[20];
    __uint32_t data_offset;
    __uint32_t data_size;
} PackedHashItem;







/**
 * Data Blob
 *
 * Data object used for reading and writing
 * to the archive.
 */
typedef struct {
    size_t data_size;
    char* key;
    char* data;
} DataBlob;


/**
 * Page of the Hash Map
 * there is 1 page for each of the first chars.
 */
typedef struct HashPage {
    unsigned short allocated;
    unsigned short length;
    HashItem* items;
} HashPage;


/**
 *
 * Hash Index keeps a collection of pages mapped by the first char of the key.
 *
 */
typedef struct {
    size_t items;
    HashPage* pages[256];
} HashIndex;


HashPage* HashIndex_get_or_create_page(HashIndex* self, char key[20]);
void HashPage_set(HashPage* self, HashItem* item);
void HashIndex__init__(HashIndex* self);
void HashIndex_free(HashIndex* self);
bool HashIndex_has(HashIndex* self, char* key);
HashItem* HashIndex_get(HashIndex* self, char* key);
void HashItem__init_with_key__(HashItem* self, char key[20], size_t data_offset, size_t data_size);
Errors HashIndex_set(HashIndex* self, HashItem* item);
void HashPage_set_packed(HashPage* self, PackedHashItem* item);

#endif //ARCHIVELIB_HASHINDEX_H
