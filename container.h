
#ifndef WAKS_CONTAINER_H
#define WAKS_CONTAINER_H

#include "allocator.h"


/// Any val;
/// WAKS_TEMP_ARENA(my_arena) {
///    Any *p = vector_safe_get(my_arena, my_vec, 5);
///    if (p) val = *p; // Copy out immediately
/// }
// HandleDefer cleans up the borrow here
#define vector_safe_get(arena, vector, index)                                                      \
    ((index < vector.length) ? (HandleDefer(arena, vector.data),                                   \
                                (Any *)HandleBorrow(arena, vector.data, vector.user_id) + index)   \
                             : WAKS_NOVALUE)

/*
 * EXAMPLE: Vector<T>
 * Vector nums = vector_init(arena, 8, sizeof(u32), uid);
 * vector_push_t(arena, &nums, (u32)10);
 * vector_push_t(arena, &nums, (u32)20);
 */
#define vector_push_t(arena, vec, value)                                                           \
    do {                                                                                           \
        WAKS_AUTO _tmp = (value);                                                                       \
        vector_push_raw(arena, vec, &_tmp);                                                        \
    } while (0)

/* u32 x = vector_get_t(arena, &nums, u32, 1); */
#define vector_get_t(arena, vec, T, index) (*(T *)vector_get_raw(arena, vec, index))

//  for (ListNode *curr = line_list; curr != WAKS_NOVALUE; curr = curr->next) {
//      EditorLine *line_data = container_of(curr, EditorLine, node);
//      (Do something with line_data->content...)
//  }
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - (waks_uintptr)&((type *)0)->member))


/// OPTION API
typedef struct Option Option;
struct Option
{
    Handle    value;
    waks_bool has_value;
};

static inline Option option_none(void);
static inline Option option_some(Handle handle);
static inline Handle option_unwrap(Option *opt);
static inline Handle option_unwrap_or(Option *opt, Handle fallback);

///
// LIST API

/// Metadata stays inside the struct you want to link: no need for extra
/// boxalloc
typedef struct ListNode ListNode;
struct ListNode
{
    ListNode *next;
    ListNode *prev;
};

/// Linked list API
static inline void 		list_init(ListNode *node);
static inline void 		list_push_back(ListNode **head, ListNode *new_node);
static inline void 		list_remove(ListNode **head, ListNode *node);
static inline waks_bool list_is_empty(ListNode *head);

/// VECTORS API

typedef struct Vector Vector;
struct Vector
{
    Handle     data;     // Handle to the underlying array
    waks_usize capacity; // Total slots
    waks_usize length;   // Used slots
    waks_usize item_size;
    waks_u32   user_id;  // Owner of the data
};

#define SLICE_CAST(type, string) ((type *)(string).ptr)

static inline Vector vector_init(Arena *arena, waks_usize initial_cap, waks_usize item_size, waks_u32 user_id);
static inline Any *vector_get(Arena *arena, Vector *vector,     waks_usize index);
static inline Any  vector_get_copy(Arena *arena, Vector *vector, waks_usize index);
static inline Any  vector_pop(Arena *arena, Vector *vector);
static inline void vector_push(Arena *arena, Vector *vector, Any value);
static inline void vector_insert(Arena *arena, Vector *vector, waks_usize index, Any value);
static inline void vector_remove(Arena *arena, Vector *vector, waks_usize index);
static inline void vector_clear(Vector *vector);
static inline void vector_ensure_capacity(Arena *arena, Vector *vector, waks_usize min_cap);

static inline void  vector_push_raw(Arena *arena, Vector *vector, void *item);
static inline void *vector_get_raw(Arena *arena, Vector *vector, waks_usize index);
static inline void  vector_pop_raw(Arena *arena, Vector *vector);

#endif // WAKS_CONTAINER_H

#ifdef WAKS_CONTAINER_IMPLEMENTATION

/// OPTION API IMPLEMENTATION

static inline Option option_none(void)
{
    return (Option){.value = {0, 0}, .has_value = false};
}

static inline Option option_some(Handle handle)
{
    return (Option){.value = handle, .has_value = true};
}

// Think about passing by reference so that we know we are passing
// by reference and not by value 
static inline Handle option_unwrap(Option *opt)
{
    if (!opt) return (Handle){0};
	 // TODO(waks-work):implement the panic issue and check the logic as it is
	 // needed and used in alot of places not just here and it may lead to issue
	 // it is the part returning segmentation fault */
    if (!opt->has_value) PANIC_MSG("Attempted to unwrap an Option(None)");

    return opt->value;
}

// Think about passing by reference so that we know we are passing
// by reference and not by value 
static inline Handle option_unwrap_or(Option *opt, Handle fallback)
{
    return opt->has_value ? opt->value : fallback;
}

/// LINKED LIST IMPLEMENTATION

// EXAMPLES:
// typedef struct { u32 id; char *content; ListNode node; } EditorLine;
// 
//	 ListNode *line_list = WAKS_NOVALUE;
//	 EditorLine *line = ArenaPush(arena, sizeof(EditorLine));
//	 line->id = 1;


// typedef struct {
//     char *name;
//     ListNode active_node;  // For the list of open files
//     ListNode history_node; // For the "recently closed" list
// } File;
// list_push_back(&active_files, &file->active_node);
// list_push_back(&recent_history, &file->history_node);
//

// list_init(&line->node); 
static inline void list_init(ListNode *node)
{
    node->next = WAKS_NOVALUE;
    node->prev = WAKS_NOVALUE;
}

// list_push_back(&line_list, &line->node); 
static inline void list_push_back(ListNode **head, ListNode *new_node)
{
    if (!new_node) return;

    if (*head == WAKS_NOVALUE) {
        *head          = new_node;
        new_node->next = WAKS_NOVALUE;
        new_node->prev = WAKS_NOVALUE;
        return;
    }

    ListNode *curr = *head;
    while (curr->next) curr = curr->next;

    curr->next     = new_node;
    new_node->prev = curr;
    new_node->next = WAKS_NOVALUE;
}

// list_remove(&line_list, &line->node); 
static inline void list_remove(ListNode **head, ListNode *node)
{
    if (!head || !*head || !node) return;

    // prev-> points to -> next item
    if (node->prev) node->prev->next = node->next;

    // prev <- the prev item is pointed back by the next item <- next
    if (node->next) node->next->prev = node->prev;

    // move the head pointer to the next element
    if (*head == node) *head = node->next;

    node->prev = WAKS_NOVALUE;
    node->next = WAKS_NOVALUE;
}

static inline waks_bool list_is_empty(ListNode *head)
{
    return head == WAKS_NOVALUE;
}

/// VECTORS IMPLEMENTATION

// EXAMPLE: VECTOR<Any>
// Vector vals = vector_init(arena, 8, sizeof(Any), uid);
// vector_push(arena, &vals, AnyInt(99));
// Any a = vector_get_copy(arena, &vals, 0);

static inline Vector vector_init(Arena *arena, waks_usize initial_cap, waks_usize item_size, waks_u32 user_id)
{
    Vector list = {0};
    list.user_id = user_id;
    list.capacity = initial_cap;
    list.length = 0;
    list.item_size = (waks_usize)item_size;
    list.data = BoxAlloc(arena, initial_cap * sizeof(waks_uchar), user_id);
    return list;
}

static inline Any *vector_get(Arena *arena, Vector *vector, waks_usize index)
{
    if (index >= vector->length) return WAKS_NOVALUE;

    HandleDefer(arena, vector->data);
    Any *ptr = (Any *)HandleBorrow(arena, vector->data, vector->user_id);
    return ptr ? &ptr[index] : WAKS_NOVALUE;
}

static inline void *vector_get_raw(Arena *arena, Vector *vector, waks_usize index)
{
    if (index >= vector->length) return WAKS_NOVALUE;

    HandleDefer(arena, vector->data);
    waks_uchar *ptr = (waks_uchar *)HandleBorrow(arena, vector->data, vector->user_id);
    return ptr ? &ptr[index] : WAKS_NOVALUE;
}

static inline Any vector_get_copy(Arena *arena, Vector *vector, waks_usize index)
{
    if (index >= vector->length) return AnyNone();
    Any result = AnyNone();
    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (data_ptr) result = data_ptr[index];
    }
    return result;
}

static inline void vector_push(Arena *arena, Vector *vector, Any value)
{
    vector_ensure_capacity(arena, vector, vector->length + 1);

    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        
        // TODO(waks-work): implement proper panic or after failure case handling to
        // ensure is is more secure
        if (!data_ptr) return;
        // PANIC_MSG("Vector Push Failed: Handle Corruption or mismatch.");
        data_ptr[vector->length] = value;
        vector->length += 1;
    }
}

static inline void vector_push_raw(Arena *arena, Vector *vector, void *item)
{
    if (!item) return;

    vector_ensure_capacity(arena, vector, vector->length + 1);
    WITH_ARENA (arena) {
        ScopedBorrow(waks_uchar, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) return;

        waks_uchar *dst = data_ptr + (vector->length * vector->item_size);
        __builtin_memcpy(dst, item, vector->item_size);
        vector->length += 1;
    }
}

static inline Any vector_pop(Arena *arena, Vector *vector)
{
    if (vector->length == 0) return AnyNone();
    Any result = AnyNone();

    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) PANIC_MSG("Vector pop failed: Borrow denied");
        if (data_ptr) {
            vector->length -= 1;
            result = data_ptr[vector->length];
            // data_ptr[vector->length] = AnyNone();
        }
    }
    return result;
}

static inline void vector_pop_raw(Arena *arena, Vector *vector)
{
    (void)arena;
    if (vector->length == 0) return;
    vector->length -= 1;
}

static inline void vector_insert(Arena *arena, Vector *vector, waks_usize index, Any value)
{
    if (index > vector->length) index = vector->length;
    vector_ensure_capacity(arena, vector, vector->length + 1);

    WITH_ARENA (arena) {
         // TODO(waks-work): implement proper panic or after failure case handling to
         // ensure is is more secure
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) return; /// PANIC_MSG("Couldn't Insert: Failed to Borrow");

        for (waks_usize i = vector->length; i > index; i--)
            data_ptr[i] = data_ptr[i - 1];

        data_ptr[index] = value;
        vector->length += 1;
    }
}

static inline void vector_remove(Arena *arena, Vector *vector, waks_usize index)
{
    if (index >= vector->length) return;

    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) PANIC_MSG("Couldn't Insert: Failed to Borrow");

        for (waks_usize i = index; i < vector->length - 1; i++)
            data_ptr[i] = data_ptr[i + 1];
        vector->length -= 1;
    }
}

static inline void vector_clear(Vector *vector)
{
    vector->length = 0;
}

static inline void vector_ensure_capacity(Arena *arena, Vector *vector, waks_usize min_cap)
{
    if (vector->capacity >= min_cap) return;

    waks_usize new_capacity = min_cap == 0 ? 8 : vector->capacity * 2;
    if (min_cap >= new_capacity) new_capacity = min_cap;

    Handle new_handle = BoxAlloc(arena, new_capacity * sizeof(Any), vector->user_id);
    if (vector->length > 0) {
        WITH_ARENA (arena) {
            ScopedBorrow(Any, old_data, vector->data, vector->user_id);

             // We don't use ScopeBorrow instead we use the raw HandleBorrowMut to
             // prevent new_data from being cleared automatically at the end of the
             // scope so we can clean it manually when we need to clean it up as expected.
            Any *new_data = HandleBorrowMut(arena, new_handle, vector->user_id);
            if (old_data && new_data) {
                for (waks_usize i = 0; i < vector->length; i++) {
                    new_data[i] = old_data[i];
                }
            }

            HandleReleaseMut(arena, vector->data);
        }
    }

    vector->capacity = new_capacity;
    vector->data = new_handle;
}


#endif // WAKS_CONTAINER_IMPLEMENTATION
