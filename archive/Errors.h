#ifndef ERRORS_H
#define ERRORS_H


typedef enum Errors
{
    E_SUCCESS                       = 0,
    E_SYSTEM_ERROR_ERRNO            = -1,
    E_INDEX_MAX_SIZE_EXCEEDED       = -2,
    E_INDEX_OUT_OF_BOUNDS           = -3,
    E_FILE_READ_ERROR               = -4,
    E_NOT_FOUND                     = -5,
    E_UNKNOWN_ARCHIVE_VERSION       = -6,
} Errors;


#endif /* ERRORS_H */
