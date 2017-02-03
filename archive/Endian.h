#ifndef ENDIAN_H
#define ENDIAN_H


#if defined(__clang__)
#if __has_builtin(__builtin_bswap32)
#define bswap32 __builtin_bswap32
#endif
#endif

#ifndef bswap32
#define bswap32(v)                                                          \
      ((((uint32_t)(v) << 24))                                              \
          | (((uint32_t)(v) << 8) & UINT32_C(0x00FF0000))                   \
          | (((uint32_t)(v) >> 8) & UINT32_C(0x0000FF00))                   \
          | (((uint32_t)(v) >> 24)))
#endif

#if defined(__LITTLE_ENDIAN__)

#define be32toh(v) bswap32(v)
#define htobe32(v) bswap32(v)

#elif defined(__BIG_ENDIAN__)

#define be32toh(v) (v)
#define htobe32(v) (v)

#else
#error Endian not supported
#endif

#endif /* ENDIAN_H */
