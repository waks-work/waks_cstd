
#ifndef WAKS_CONTAINER_H
#define WAKS_CONTAINER_H

#include "allocator.h"

#ifdef __cplusplus 
extern "C" { 
#endif 

// This macro allows us to safely get values from an array_list 
// safely that gets defered and cleaned once out of scope or 
// when waks_arena_reset is called.
#define WAKS_array_list_sget(arena, vector, index) \
    (((index) < (vector).length) ? (waks_handle_defer((arena), (vector).data), \
    (Any *)waks_handle_borrow((arena), (vector).data, (vector).user_id) + (index)) : WAKS_NOVALUE)

// Allows us to push elements to our arraylist one by one indipendent
// of the provided type. It makes it more generic supporting any type 
// of array list
#define WAKS_array_list_push_t(arena, vec, value)        \
    do {                                                 \
        WAKS_AUTO _tmp = (value);                        \
        waks_array_list_push_raw((arena), (vec), &_tmp); \
    } while (0)

// This macro allows for a more generic get operation for any type.
#define WAKS_array_list_get_t(arena, vec, TYPE, index) (*(TYPE *)waks_array_list_get_raw(arena, vec, index))

// This macro allows us to create a container wrapper for any type 
// of data structure.
#define WAKS_container_of(ptr, type, member) ((type *)((char *)(ptr) - (waks_uintptr)&((type *)0)->member))


/// OPTION API

// This an option type  that allows us to either return a 
// value or else we return None. Allows for much safer operations 
// preventing panics and allows bugs to be caught at compile time.
typedef struct waks_option waks_option;
struct waks_option
{
    waks_handle value;     // encapsulated handle which is either value or none
    waks_bool   has_value; // returns true if there is value 
};


// This function when called construct a None option.
waks_option waks_option_none(void);

// This creates some option wrapping around a valid option.
waks_option waks_option_some(waks_handle handle);

// This function returns a some option if valid or
// panics if it is a none option.
waks_handle waks_option_unwrap(waks_option *opt);

// This function returns a some option if valid and a 
// fallback if the option is none.
waks_handle waks_option_unwrap_or(waks_option *opt, waks_handle fallback);

///
// LIST API

// This is an intrunsive linked list node header.
typedef struct waks_list_node waks_list_node;
struct waks_list_node
{
    waks_list_node *next;
    waks_list_node *prev;
};

/// Linked list API

// This function initialises our nodelinkd to NULL.
void 	  waks_list_init(waks_list_node *node);

// This function appends a new node to the end of a doubly linked list.
void 	  waks_list_push_back(waks_list_node **head, waks_list_node *new_node);

// This unlinks a node from the parent doubly linked list when called.
void 	  waks_list_remove(waks_list_node **head, waks_list_node *node);

// This function when called checks if the list is empty or 
// not by checking if the head is NULL/WAKS_NOVALUE.
waks_bool waks_list_is_empty(waks_list_node *head);

/// WAKS_ARRAY_LIST API

typedef struct waks_array_list waks_array_list;
struct waks_array_list
{
    waks_handle  data;      // memory box handle storing underlying buffer.
    waks_usize   capacity;  // total number of the allocated element.
    waks_usize   length;    // current active element count.
    waks_usize   item_size; // byte size of each individual elements.
    waks_u32     user_id;   // owner id used to handle permission verification.
};

#define WAKS_SLICE_CAST(type, string) ((type *)(string).ptr)

// This initialises the dynamic array list allocated inside the arena.
waks_array_list waks_array_list_init(waks_arena *arena, waks_usize initial_cap, waks_usize item_size, waks_u32 user_id);

// This retrieves a direct pointer to element at specified index. 
Any   *waks_array_list_get(waks_arena *arena, waks_array_list *vector,     waks_usize index);

// This copies the vale of the element at the specified index.
Any    waks_array_list_get_copy(waks_arena *arena, waks_array_list *vector, waks_usize index);

// This removes the top/last  element at the top of the array list.
Any    waks_array_list_pop(waks_arena *arena, waks_array_list *vector);

// This appends element at the top/end of the array list.
void   waks_array_list_push(waks_arena *arena, waks_array_list *vector, Any value);

// This inserts an element at the specified index.
void   waks_array_list_insert(waks_arena *arena, waks_array_list *vector, waks_usize index, Any value);

// This removes an element from the array list.
void   waks_array_list_remove(waks_arena *arena, waks_array_list *vector, waks_usize index);

// This resets the array list length to  zero without deallocating buffer memory.
void   waks_array_list_clear(waks_array_list *vector);

// This ensures the array list has room for at least the minimum capacity.
void   waks_array_list_ensure_capacity(waks_arena *arena, waks_array_list *vector, waks_usize min_cap);

// This allows for raw byte-copy push operation for generic types
void   waks_array_list_push_raw(waks_arena *arena, waks_array_list *vector, void *item);

// This allows for raw pointer access to element byte offset at index.
void  *waks_array_list_get_raw(waks_arena *arena, waks_array_list *vector, waks_usize index);

// This allows for the array list length to be decremented by one without 
// returning the element.
void   waks_array_list_pop_raw(waks_arena *arena, waks_array_list *vector);


/// 
// HASHMAP API 
// 

typedef struct waks_hm_bucket waks_hm_bucket;
typedef struct waks_hash_map waks_hash_map;

struct waks_hm_bucket 
{
	waks_char      *key;
	void           *value;
	waks_hm_bucket *next;
};

struct waks_hash_map 
{
    waks_hm_bucket **buckets;
	waks_ssize       capacity;
    waks_ssize       size;
	waks_arena      *arena;
	waks_bool        use_heap;
};

#define WAKS_hm_loadfactor(m, h) (h) % (m)->capacity
#define WAKS_TO_PTR(val)   ((void*)(waks_uintptr)(val))
#define WAKS_FROM_PTR(T, p) ((T)(waks_uintptr)(p))

// convenience wrapper for storing primitive integers:
#define WAKS_HM_SET_INT(m, key, i) waks_hm_insert((m), (key), WAKS_TO_PTR(i))
#define WAKS_HM_GET_INT(T, m, key) WAKS_FROM_PTR(T, waks_hm_get((m), (key)))

// this is the hash function implementation from the k33 algorithm 
// from dan bernstein(djb2).
waks_u64   waks_hstr_function(const waks_char *str);
void       waks_hm_init_malloc(waks_hash_map *m, waks_ssize capacity);
void       waks_hm_init_arena(waks_arena *arena, waks_hash_map *m, waks_ssize capacity);
void       waks_hm_insert(waks_hash_map *m, const waks_char *key, void *value);
void       waks_hm_resize(waks_hash_map *m, waks_ssize capacity);
void      *waks_hm_get(waks_hash_map *m, const waks_char *key);
waks_bool  waks_hm_contains(waks_hash_map *m, const waks_char *key);
void       waks_hm_remove(waks_hash_map *m, const waks_char *key);
void       waks_hm_free(waks_hash_map *m);

#ifdef __cplusplus 
}
#endif

#endif // WAKS_CONTAINER_H

#ifdef WAKS_CONTAINER_IMPLEMENTATION

/// OPTION API IMPLEMENTATION

waks_option waks_option_none(void)
{
    return (waks_option){.value = {0, 0, 0, 0}, .has_value = false};
}

waks_option waks_option_some(waks_handle handle)
{
    return (waks_option){.value = handle, .has_value = true};
}

// Think about passing by reference so that we know we are passing
// by reference and not by value 
waks_handle waks_option_unwrap(waks_option *opt)
{
    if (!opt) return (waks_handle){0, 0, 0, 0};
	 // TODO(waks-work):implement the panic issue and check the logic as it is
	 // needed and used in alot of places not just here and it may lead to issue
	 // it is the part returning segmentation fault */
    if (!opt->has_value) WAKS_PANIC_MSG("Attempted to unwrap an waks_option(None)");

    return opt->value;
}

// Think about passing by reference so that we know we are passing
// by reference and not by value 
waks_handle waks_option_unwrap_or(waks_option *opt, waks_handle fallback)
{
    return opt->has_value ? opt->value : fallback;
}

/// LINKED LIST IMPLEMENTATION

// list_init(&line->node); 
void waks_list_init(waks_list_node *node)
{
    node->next = WAKS_NOVALUE;
    node->prev = WAKS_NOVALUE;
}

// list_push_back(&line_list, &line->node); 
void waks_list_push_back(waks_list_node **head, waks_list_node *new_node)
{
    if (!new_node) return;

    if (*head == WAKS_NOVALUE) {
        *head          = new_node;
        new_node->next = WAKS_NOVALUE;
        new_node->prev = WAKS_NOVALUE;
        return;
    }

    waks_list_node *curr = *head;
    while (curr->next) curr = curr->next;

    curr->next     = new_node;
    new_node->prev = curr;
    new_node->next = WAKS_NOVALUE;
}

// list_remove(&line_list, &line->node); 
void waks_list_remove(waks_list_node **head, waks_list_node *node)
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

waks_bool waks_list_is_empty(waks_list_node *head)
{
    return head == WAKS_NOVALUE;
}

/// VECTORS IMPLEMENTATION

waks_array_list waks_array_list_init(waks_arena *arena, waks_usize initial_cap, waks_usize item_size, waks_u32 user_id)
{
    waks_array_list list = {0};
    list.user_id = user_id;
    list.capacity = initial_cap;
    list.length = 0;
    list.item_size = (waks_usize)item_size;
    list.data = waks_box_alloc(arena, initial_cap * (waks_usize)item_size, user_id);
    return list;
}

// @TODO(waks-work): 
//Every time get() or get_raw() is called, waks_handle_defer is invoked.
//If a caller calls get() 1,000 times inside a loop, it pushes 1,000 
//entries into the arena defer list, filling up your arena memory header fast!
//
//Fix: Let waks_handle_borrow handle shared borrows directly, or 
//document that callers using waks_array_list_get_raw should borrow/release 
//explicitly, or keep deferring tied strictly to waks_array_list_sget.
Any *waks_array_list_get(waks_arena *arena, waks_array_list *vector, waks_usize index)
{
    if (index >= vector->length) return WAKS_NOVALUE;

    waks_handle_defer(arena, vector->data);
    Any *ptr = (Any *)waks_handle_borrow(arena, vector->data, vector->user_id);
    return ptr ? &ptr[index] : WAKS_NOVALUE;
}

void *waks_array_list_get_raw(waks_arena *arena, waks_array_list *vector, waks_usize index)
{
    if (index >= vector->length) return WAKS_NOVALUE;

    waks_handle_defer(arena, vector->data);
    waks_uchar *ptr = (waks_uchar *)waks_handle_borrow(arena, vector->data, vector->user_id);
    return ptr ? (ptr + (index * vector->item_size)) : WAKS_NOVALUE;
}

Any waks_array_list_get_copy(waks_arena *arena, waks_array_list *vector, waks_usize index)
{
    if (index >= vector->length) return AnyNone();
    Any result = AnyNone();
    WAKS_TEMP_ARENA (arena) {
        WAKS_scoped_borrow(Any, data_ptr, vector->data, vector->user_id);
        if (data_ptr) result = data_ptr[index];
    }
    return result;
}

void waks_array_list_push(waks_arena *arena, waks_array_list *vector, Any value)
{
    waks_array_list_ensure_capacity(arena, vector, vector->length + 1);

    WAKS_TEMP_ARENA (arena) {
        WAKS_scoped_borrow(Any, data_ptr, vector->data, vector->user_id);
        
        // TODO(waks-work): implement proper panic or after failure case handling to
        // ensure is is more secure
        if (!data_ptr) return;
        // PANIC_MSG("waks_array_list Push Failed: Handle Corruption or mismatch.");
        data_ptr[vector->length] = value;
        vector->length += 1;
    }
}

void waks_array_list_push_raw(waks_arena *arena, waks_array_list *vector, void *item)
{
    if (!item) return;

    waks_array_list_ensure_capacity(arena, vector, vector->length + 1);
    WAKS_TEMP_ARENA (arena) {
        WAKS_scoped_borrow(waks_uchar, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) return;

        waks_uchar *dst = data_ptr + (vector->length * vector->item_size);
        __builtin_memcpy(dst, item, vector->item_size);
        vector->length += 1;
    }
}

Any waks_array_list_pop(waks_arena *arena, waks_array_list *vector)
{
    if (vector->length == 0) return AnyNone();
    Any result = AnyNone();

    WAKS_TEMP_ARENA (arena) {
        WAKS_scoped_borrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) WAKS_PANIC_MSG("waks_array_list pop failed: Borrow denied");
        if (data_ptr) {
            vector->length -= 1;
            result = data_ptr[vector->length];
            // data_ptr[vector->length] = AnyNone();
        }
    }
    return result;
}

void waks_array_list_pop_raw(waks_arena *arena, waks_array_list *vector)
{
    (void)arena;
    if (vector->length == 0) return;
    vector->length -= 1;
}

void waks_array_list_insert(waks_arena *arena, waks_array_list *vector, waks_usize index, Any value)
{
    if (index > vector->length) index = vector->length;
    waks_array_list_ensure_capacity(arena, vector, vector->length + 1);

    WAKS_TEMP_ARENA (arena) {
         // TODO(waks-work): implement proper panic or after failure case handling to
         // ensure is is more secure
        WAKS_scoped_borrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) return; /// PANIC_MSG("Couldn't Insert: Failed to Borrow");

        for (waks_usize i = vector->length; i > index; i--)
            data_ptr[i] = data_ptr[i - 1];

        data_ptr[index] = value;
        vector->length += 1;
    }
}

void waks_array_list_remove(waks_arena *arena, waks_array_list *vector, waks_usize index)
{
    if (index >= vector->length) return;

    WAKS_TEMP_ARENA (arena) {
        WAKS_scoped_borrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) WAKS_PANIC_MSG("Couldn't Insert: Failed to Borrow");

        // @TODO(waks-work): check and fix the out of bounds bug/error
        // ie use: waks_uchar *base = (waks_uchar *)data_ptr + (index * vector->item_size);
        //         waks_uchar *next = base + vector->item_size;
        //         waks_usize bytes_to_move = (vector->length - index - 1) * vector->item_size;
        //         __builtin_memmove(base, next, bytes_to_move);
        for (waks_usize i = index; i < vector->length - 1; i++)
            data_ptr[i] = data_ptr[i + 1];
        vector->length -= 1;
    }
}

void waks_array_list_clear(waks_array_list *vector)
{
    vector->length = 0;
}

void waks_array_list_ensure_capacity(waks_arena *arena, waks_array_list *vector, waks_usize min_cap)
{
    if (vector->capacity >= min_cap) return;

    waks_usize new_capacity = min_cap == 0 ? 8 : vector->capacity * 2;
    if (min_cap >= new_capacity) new_capacity = min_cap;

    waks_handle new_handle = waks_box_alloc(arena, new_capacity * vector->item_size, vector->user_id);
    if (vector->length > 0) {
        WAKS_TEMP_ARENA (arena) {
            WAKS_scoped_borrow(Any, old_data, vector->data, vector->user_id);

            // We don't use ScopeBorrow instead we use the raw HandleBorrowMut to
            // prevent new_data from being cleared automatically at the end of the
            // scope so we can clean it manually when we need to clean it up as expected.
            // @TODO(): think of using waks_uchar* instead of Any *
            Any *new_data = waks_handle_borrow_mut(arena, new_handle, vector->user_id);
            if (old_data && new_data) {
                __builtin_memcpy(new_data, old_data, (vector->length * vector->item_size));
            }
            waks_handle_release_mut(arena, new_handle); //vector->data);
        }
    }

    vector->capacity = new_capacity;
    vector->data = new_handle;
}

/// 
// WAKS_HASHMAP IMPLEMENTATION 
// 

waks_u64 waks_hstr_function(const waks_char *str)
{
    waks_u64 hash = 5381;
    waks_u32 c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + (waks_u64)c; // hash * 33 + c

    return hash;
}


void waks_hm_init_malloc(waks_hash_map *m, waks_ssize capacity) 
{
	// guarantees inlining without calling external functions.
    __builtin_memset(m, 0, sizeof * m);	
	m->capacity = capacity;
	m->use_heap = waks_true;
#if defined(WAKS_USE_STD_LIB)
	m->buckets = malloc(sizeof(waks_hm_bucket*) * m->capacity);
    if (m->buckets) __builtin_memset(m->buckets, 0, sizeof(waks_hm_bucket*) * m->capacity);
#else 
	m->buckets = WAKS_NOVALUE;
#endif
	if(!m->buckets) return;
}

void waks_hm_init_arena(waks_arena *arena,waks_hash_map *m, waks_ssize capacity)
{
	__builtin_memset(m, 0, sizeof * m);	
	m->capacity = capacity;
	m->use_heap = waks_false;
	m->arena    = arena;
    m->buckets  = waks_arena_push(arena, sizeof(waks_hm_bucket*) * m->capacity); 
    if (m->buckets) __builtin_memset(m->buckets, 0, sizeof(waks_hm_bucket*) * m->capacity);
	if (!m->buckets) return;
}

void waks_hm_resize(waks_hash_map *m, waks_ssize new_cap) 
{
    waks_hm_bucket **old_buckets  = m->buckets;
    waks_ssize       old_capacity = m->capacity;

    // allocate new_buckets array
    waks_hm_bucket **new_buckets;
    if (m->use_heap) {
#if defined(WAKS_USE_STD_LIB)
        new_buckets = calloc(new_cap, sizeof(waks_hm_bucket*));
#else 
		new_buckets = WAKS_NOVALUE;
#endif
    } else {
        new_buckets = waks_arena_push(m->arena, sizeof(waks_hm_bucket*) * new_cap);
        if (new_buckets) __builtin_memset(new_buckets, 0, sizeof(waks_hm_bucket*) * new_cap);
    }

	// keep old table if new_bucket allocation fails
    if (!new_buckets) return; 

    // rehash: walk every old chain, prepend each node to its new bucket
    for (waks_ssize i = 0; i < old_capacity; i++) {
        waks_hm_bucket *curr = old_buckets[i];
        while (curr) {
            waks_hm_bucket *next = curr->next;

            waks_u64   hash = waks_hstr_function(curr->key);
            waks_ssize index = (waks_ssize)(hash % new_cap);

            curr->next = new_buckets[index]; // Prepend to new chain
            new_buckets[index] = curr;

            curr = next;
        }
    }

    // free old buckets array (heap only; arena reclaims on release)
    if (m->use_heap) {
#if defined(WAKS_USE_STD_LIB)
        free(old_buckets);
#endif
    }

    m->buckets  = new_buckets;
    m->capacity = new_cap;
}

void waks_hm_insert(waks_hash_map *m, const waks_char *key, void *value)
{
    waks_u64 hash        = waks_hstr_function(key);
	waks_ssize index     = WAKS_hm_loadfactor(m, hash);
    waks_hm_bucket *curr = m->buckets[index];

    while (curr) {
        if (waks_str_cstr_eq_cstr(curr->key, key)) {
            curr->value = value;
            return;
        }
        curr = curr->next;
    }
    waks_hm_bucket *b   = WAKS_NOVALUE;
    waks_char *key_copy = WAKS_NOVALUE;
    waks_ssize key_len  = waks_str_cstr_len(key);
	// allocate memory if the current bucket is already filled
	if (m->use_heap) {
#if defined(WAKS_USE_STD_LIB)
        b = (waks_hm_bucket*)malloc(sizeof(waks_hm_bucket));
        key_copy = (waks_char*)malloc(key_len + 1);
        if (key_copy) __builtin_memcpy(key_copy, key, key_len + 1);
#endif
    } else {
        b = (waks_hm_bucket*)waks_arena_push(m->arena, sizeof(waks_hm_bucket));
        key_copy = (waks_char*)waks_arena_push(m->arena, key_len + 1);
        if (key_copy) __builtin_memcpy(key_copy, key, key_len + 1);
    }    

    if (!b || !key_copy) return;

    b->key   = key_copy;
    b->value = value;
    b->next  = m->buckets[index];

    m->buckets[index] = b;
    m->size++;

    // Trigger double-capacity expansion at 75% load factor
    if (m->size > (waks_ssize)(m->capacity * 0.75)) waks_hm_resize(m, m->capacity * 2); 
}

void *waks_hm_get(waks_hash_map *m, const waks_char *key)
{
    waks_u64 hash        = waks_hstr_function(key);
    waks_ssize index     = WAKS_hm_loadfactor(m, hash);
    waks_hm_bucket *curr = m->buckets[index];

    while (curr) {
        if (waks_str_cstr_eq_cstr(curr->key, key)) {
            return curr->value;
        }
        curr = curr->next;
    }
    return WAKS_NOVALUE;
}

waks_bool waks_hm_contains(waks_hash_map *m, const waks_char *key)
{
    waks_u64 hash        = waks_hstr_function(key);
    waks_ssize index     = WAKS_hm_loadfactor(m, hash);
    waks_hm_bucket *curr = m->buckets[index];

    while (curr) {
        if (waks_str_cstr_eq_cstr(curr->key, key)) {
            return waks_true; 
        }
        curr = curr->next;
    }
    
    return waks_false; 
}

void waks_hm_remove(waks_hash_map *m, const waks_char *key)
{
    waks_u64 hash    = waks_hstr_function(key);
    waks_ssize index = WAKS_hm_loadfactor(m,hash); 

    waks_hm_bucket *curr = m->buckets[index];
    waks_hm_bucket *prev = WAKS_NOVALUE;

    while (curr) {
        if (waks_str_cstr_eq_cstr(curr->key, key)) {
            if (prev != WAKS_NOVALUE) prev->next = curr->next;
            else m->buckets[index] = curr->next;

			if (m->use_heap) {
#if defined(WAKS_USE_STD_LIB)
				free(curr->key);
                free(curr);
#endif
			}
            m->size--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void waks_hm_free(waks_hash_map *m)
{
	if (!m || !m->buckets) return;
	if (m->use_heap) {
        for (waks_ssize i = 0; i < m->capacity; i++) {
            waks_hm_bucket *curr = m->buckets[i];
            while (curr) {
                waks_hm_bucket *next = curr->next;
#if defined(WAKS_USE_STD_LIB)
				free(curr->key);
                free(curr);
#endif
                curr = next;
            }
        }
#if defined(WAKS_USE_STD_LIB)
        free(m->buckets);
#endif
	}
    m->buckets  = WAKS_NOVALUE;
    m->capacity = 0;
    m->size     = 0;

    m->arena = WAKS_NOVALUE;
    m->use_heap = waks_false;   
}

#endif // WAKS_CONTAINER_IMPLEMENTATION
