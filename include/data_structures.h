#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include "arena.h"
#include "types.h"
// - Array list
// - Span
// - Hashmap

/// ie zig/rust slice: replaces raw pointers
typedef struct Span Span;
struct Span
{
    u8 *ptr;
    usize len;
};

#define SLICE_CAST(type, span) ((type *)(span).ptr)

/// Metadata stays inside the struct you want to link: no need for extra boxalloc
typedef struct ListNode ListNode;
struct ListNode
{
    ListNode *next;
    ListNode *prev
};

/// Rust Option C style
typedef struct Option Option;
struct Option
{
    Handle value;
    b8 has_value
};

#endif
