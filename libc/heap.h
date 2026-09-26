#ifndef _HEAP_H
#define _HEAP_H
#include <stddef.h>
#include <stdint.h>

/* Single-core allocator. Never call malloc/free from interrupt handlers. */
struct heap_stats {
    size_t total, used, free, largest, overhead;
    size_t peak_used, peak_occupied, used_blocks, free_blocks;
    unsigned malloc_calls, calloc_calls, realloc_calls, free_calls;
    unsigned realloc_inplace, realloc_moved, failures;
    size_t last_request, last_old_size, last_count, last_element;
    uintptr_t last_caller;
    const char *last_op, *last_reason;
};
/* used counts rounded payloads; overhead includes ALL block headers.
 * total = used + free + overhead. largest is the largest free payload. */
int heap_get_stats(struct heap_stats *);
int heap_check(void);
void heap_reset_peak(void);
void heap_dump(const char *);
void heap_dump_failure(const char *, unsigned);
void heap_failure_site(void *);
#endif
