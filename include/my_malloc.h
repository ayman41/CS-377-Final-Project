#ifndef __MY_MALLOC_H
#define __MY_MALLOC_H
#include <stdlib.h>
#include <vector>

// Some important constants.
#define HEAP_SIZE 4096
#define MAGIC 0xDEADBEEF

enum exponent {
  Seven = 0, Eight = 1, Nine = 2, Ten = 3, Eleven = 4
};

// This struct is used as the header of an allocated block.
typedef struct __header_t {
  size_t size;  // the number of bytes of allocated memory
  unsigned int
      magic;  // the magic number used to identify a valid allocated block
  int page_num;
  enum exponent exp;
} header_t;

// This struct is used for the free list.
typedef struct __node_t {
  size_t size;            // the number of bytes available in this free block
  struct __node_t *next;  // a pointer to the next free list node
} node_t;

// This is the primary interface.
void *my_malloc(size_t);
void my_free(void *);

// We expose these functions for testing purposes.
void* alloc_page();
std::vector<node_t**>& get_heap();
void reset_heap();
size_t available_memory();
int number_of_free_nodes(exponent exp); 
void print_free_list();
void find_free(size_t size, node_t **found, node_t **previous, int* pg_num, enum exponent* exp);
void split(size_t size, node_t **previous, node_t **free_block,
           header_t **allocated, int page_num, enum exponent exp);
void coalesce(node_t *free_block);

#endif
