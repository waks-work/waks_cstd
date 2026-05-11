/// Format: X(ENUM_NAME, STRING_DESCRIPTION) 
X(WAKS_OK,                  "Operation successful")
X(WAKS_ERR_GENERIC,         "An unknown error occurred")
X(WAKS_ERR_NOMEM,           "Arena allocation failed: Out of memory")
X(WAKS_ERR_BOUNDS,          "Pointer out of bounds during iteration")
X(WAKS_ERR_ALIGN,           "Memory alignment violation")
X(WAKS_ERR_NULL,            "Null pointer dereference prevented")
X(WAKS_ERR_OVERFLOW,        "Integer or buffer overflow detected")

X(WAKS_ERR_STALE_HANDLE,    "Handle version mismatch: stale reference")
X(WAKS_ERR_BORROW_CONFLICT, "Borrow violation: resource busy")
X(WAKS_ERR_OS_REJECTED,     "OS-level memory request failed")
X(WAKS_ERR_NOT_FOUND,       "Requested item does not exist")
X(WAKS_ERR_INVALID_STATE,   "Logic error: system in wrong state for operation")

/* Handle safety errors */
X(WAKS_ERR_HANDLE_CORRUPT,  "Invalid handle magic or offset")
X(WAKS_ERR_UNAUTHORIZED,    "Owner ID mismatch: unauthorized memory access")

/* OS and environment errors */
X(WAKS_ERR_PAGE_FAULT,      "Hardware/Software page protection violation")
X(WAKS_ERR_UNSUPPORTED,     "Feature not available in this environment")

/* Collections and Data Structures errors */
X(WAKS_ERR_EMPTY,           "Operation failed: Collection is empty")
X(WAKS_ERR_FULL,            "Operation failed: Fixed-size buffer is full")

/* Concurency errors */
X(WAKS_ERR_TIMEOUT,         "Synchronization primitive timed out")
X(WAKS_ERR_DEADLOCK,        "Potential deadlock detected")
