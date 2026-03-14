#ifndef ITERATORS_H
#define ITERATORS_H

#include "types.h"

/// Foward Iterator (slice)
/// usage: foreach(u32, item, my_slice) {*item = 0;}
#define foreach(type, item, slice)                                                                 \
    for (type *item = (type *)(slice).ptr, *_end = (type *)((slice).ptr + (slice).len);            \
         item < _end; item++)

/// Reverse Iterator(Cleanup Walker): ie defer
#define foreach_rev(type, item, slice)                                                             \
    for (type *_start = (type *)((slice).ptr + (slice).len) - 1, item >= start; item--)

#endif // !ITERATORS_H
