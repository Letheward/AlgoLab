#include <stdio.h>
#include <stdlib.h>
#include <time.h>




/* ---- Types ---- */

typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef float                   f32;
typedef double                  f64;




/* ---- Timer ---- */

typedef struct {
    struct timespec start;
    struct timespec end;
    u8 which;
} PerformanceTimer;

PerformanceTimer timer;

void time_it() {
    if (timer.which == 0) {
        clock_gettime(CLOCK_MONOTONIC, &timer.start);
        timer.which = 1;
    } else {
        clock_gettime(CLOCK_MONOTONIC, &timer.end);
        printf("%fs\n", (timer.end.tv_sec - timer.start.tv_sec) + 1e-9 * (timer.end.tv_nsec - timer.start.tv_nsec));
        timer.which = 0;
    }
}




/* ==== Types ==== */

typedef struct {
    s32* data;
    u64  count;
} Array;

typedef struct List {
    s32          value;
    struct List* prev;
    struct List* next;
} List;

typedef struct {
    List* data;
    u64   pos;
    u64   size;
} ListPool;




/* ==== Pool ==== */

ListPool list_pool;

void list_pool_init(ListPool* pool, u64 size) {
    pool->data = malloc(sizeof(List) * size);
    pool->size = size;
    pool->pos  = 0;
}

void list_pool_reset(ListPool* pool) {
    for (u64 i = 0; i < pool->pos; i++) {
        pool->data[i] = (List) {0};
    }
    pool->pos = 0;
}




/* ==== Alloc ==== */

Array get_random_array(u64 count, void* (*alloc)(u64)) {
    Array out = {alloc(sizeof(s32) * count), count}; 
    for (u64 i = 0; i < out.count; i++) {
        out.data[i] = rand() % 128;
    }
    return out;
}

List* new_list_node_heap(s32 value) {
    List* l = calloc(1, sizeof(List));
    l->value = value;
    return l;
}

List* new_list_node_pool(s32 value) {
    
    ListPool* p = &list_pool;
    u64 pos = p->pos;
    if (pos >= p->size) return NULL;
    
    p->data[pos].value = value;    
    p->pos++;

    return &p->data[pos];
}

// todo: validate that this free the tail. leak?
void free_heap_nodes(List* l) {
    List* it = l; 
    while (it->next) {
        List* to_free = it;
        it = it->next;
        free(to_free);
    }
}


List* get_random_list(u64 count, List* (*new)(s32)) {
    
    List* head = new(rand() % 128);
    
    List* it = head;
    for (u64 i = 0; i < count - 1; i++) {
        List* next = new(rand() % 128);
        it->next = next;
        it = next;
    }
    
    return head;
}

List* get_random_list_in_bulk(u64 count) {
    
    ListPool* p = &list_pool;

    u64 pos    = p->pos;
    u64 wanted = pos + count;
    if (wanted >= p->size) return NULL;
    
    for (u64 i = 0; i < count - 1; i++) {
        p->data[pos + i].value = rand() % 128;
        p->data[pos + i].next = &p->data[pos + i + 1];
    }

    p->pos = wanted;
    
    return &p->data[pos];
}


void print_list(List* l) {
    for (List* it = l; it->next; it = it->next) {
        printf("%d ", it->value);        
    }
    printf("\n");
}



int main() {
    
    u64 pool_count = 1024 * 1024 * 64 + 16;
    list_pool_init(&list_pool, pool_count);
    
    for (u64 item_count = 16; item_count < pool_count; item_count *= 2) {
        
        printf("\n\nItem count: %llu\n", item_count);

        printf("\nArray: Heap\n");
        {
            time_it();
            Array a = get_random_array(item_count, malloc);
            time_it();
            
            time_it();
            for (u64 i = 0; i < a.count; i++) {
                a.data[i] += 1;
            }
            time_it();
           
            time_it();
            free(a.data);
            time_it();
        }
        
        printf("\nList: Heap\n");
        {
            time_it();
            List* l = get_random_list(item_count, new_list_node_heap);
            time_it();
            
            time_it();
            for (List* it = l; it->next; it = it->next) {
                it->value += 1;    
            }
            time_it();
            
            time_it();
            free_heap_nodes(l);
            time_it();
        }
        
        printf("\nList: Pool\n");
        {
            time_it();
            List* l = get_random_list(item_count, new_list_node_pool);
            time_it();
            
            time_it();
            for (List* it = l; it->next; it = it->next) {
                it->value += 1;    
            }
            time_it();
            
            time_it();
            list_pool_reset(&list_pool);
            time_it();
        }

        printf("\nList: Pool Bulk\n");
        {
            time_it();
            List* l = get_random_list_in_bulk(item_count);
            time_it();
            
            time_it();
            for (List* it = l; it->next; it = it->next) {
                it->value += 1;    
            }
            time_it();
            
            time_it();
            list_pool_reset(&list_pool);
            time_it();
        }
    }
    
}

