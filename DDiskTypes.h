#ifndef DLIB_DDISKTYPES_H
#define DLIB_DDISKTYPES_H


#include <stdint.h>


// UINT8, UINT16, UINT32 and UINT64
// are guaranteed to be those bitsizes
#ifndef ASYS_WINDOWS
#ifndef uint8_t_ALREADY_DEFINED
typedef unsigned char uint8_t;
#endif
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
#ifndef ALREADY_HAVE_UINT64
typedef unsigned long UINT64;
#endif // ALREADY_HAVE_UINT64
#endif // ASYS_WINDOWS
typedef signed char Suint8_t;
typedef signed char SINT8;
typedef signed short SINT16;
typedef signed int SINT32;

// ULONG will be widest possible native int
// (which means it might NOT be an unsigned long)
// UINT will be the standard native int
#ifndef ASYS_WINDOWS
#ifndef UINT_ALREADY_DEFINED
typedef unsigned int UINT;
#endif // UINT_ALREADY_DEFINED
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif // ASYS_WINDOWS


#endif

