#ifndef TYPES_H
#define TYPES_H

typedef __UINT8_TYPE__ u8; // char
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

typedef __INT8_TYPE__ i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
typedef __INT64_TYPE__ i64;

typedef float f32;
typedef double f64;

typedef __SIZE_TYPE__ usize;
typedef __PTRDIFF_TYPE__ ssize;
typedef _Bool b8;

#ifndef __cplusplus
#define true 1
#define false 0
#endif // !__cplusplus

typedef struct String String;
struct String
{
    u8 *data;
    usize length;
};

#endif // !TYPES_H
