#include <assert.h>
#include <my_malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <vector>
#include <iostream>

using namespace std;

// A vector array that stores pointers to the heads (starts) of the free lists for each allocated 
// page (i.e., vector[0] contains the free lists for the first allocated page). This allows for 
// reallocating additional 4KB pages when there is insufficient memory.
// The number of free lists (node_t objects) for each allocated page is 5. 
vector<node_t**> heap;

// The alloc_page function allocates a page (4096 bytes) from memory and returns
// a pointer to the start of the allocated page. This function does not take any paramters.
void* alloc_page() {
    // This allocates 4096 bytes from the heap and returns a void pointer to it.
    cout << "4KB page allocated" << endl;
    return (void*) mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                          MAP_ANON | MAP_PRIVATE, -1, 0);
}

// This function is used to initialize the free lists in a page. Each page will consist of 5 separate free lists,
// whose size increases by a multiple of 2. The first free list support a maximum of 2^7 bytes, the second 2^8, the 
// third 2^9, the fourth 2^10, the fifth 2^11 bytes.
// PARAMETERS: 
// page - the page in which to initialize the free lists
void init_free_lists(void* page) {
  node_t** free_lists = (node_t**) malloc(sizeof(node_t*) * 5);
  node_t* next_list = (node_t*) page;
  unsigned short int exp = 7;
  int list_pos = 0;
  for (int i = 0; i < 5; ++i) {
    // initialize current list and add list to free_lists
    next_list -> size = 1 << exp;
    next_list -> next = NULL;
    *(free_lists + list_pos) = next_list;

    // update position of next_list
    next_list = (node_t*)(((char*) next_list) + (next_list -> size))  + sizeof(node_t);

    // increment exponent
    exp++;

    // increment position in free_lists
    list_pos++;
  }
  cout << "5 free lists of size 2^7, 2^8, ... 2^11 bytes initialized in heap" << endl;
  // add free_lists to heap
  heap.push_back(free_lists);
}

// allocates 4KB heap and initializes heap with free_lists
void create_heap() {
  void* page = alloc_page();
  init_free_lists(page);
}

// clears all 4KB pages and allocates a new page
void reset_heap() {
  heap.clear();
  create_heap();
}

// Returns a reference to the vector array that stores the free lists for each allocated page
vector<node_t**>& get_heap() { return heap; }


// Calculates the total amount of free memory available across all 4KB pages in the heap
size_t available_memory() {
  size_t num_bytes = 0;
  for (auto it = heap.begin(); it != heap.end(); it++) {
    for (int offset = 0; offset < 5; offset++) {
      node_t *p = *((*it) + offset);
      while (p != NULL) {
        num_bytes += p -> size;
        p = p->next;
      }
    }
  }
  return num_bytes;
}

// This function counts the number of free nodes available in all list with 2^exp bytes.
// PARAMETERS:
// exp - an enum of type exponent that specifies the desired free lists to search through that provide
// a maximum of 2^exp bytes of availale memory.
// exp can be any of the following: Seven, Eight, Nine, Ten, Eleven, Twelve;
// RETURNS:
// The number of free nodes for a specific list with a maximum of 2^exp bytes of available memory across all 4KB pages in the heap

int number_of_free_nodes(exponent exp) {
  int count = 0;
  int offset = exp;

  for (auto it = heap.begin(); it != heap.end(); it++) {
    node_t *p = *((*it) + offset);
    while (p != NULL) {
      count++;
      p = p->next;
    }
  }

  return count;
}

// Prints all five free lists for each 4KB allocated page in the heap
void print_free_list() {
  int page_num = 1;
  for (auto it = heap.begin(); it != heap.end(); it++) {
    for (int offset = 0; offset < 5; offset++) { 
      cout << "printing list with " << (1 << (offset + 7)) << " max available bytes on page " << page_num << endl;
      node_t *p = *((*it) + offset);
      while (p != NULL) {
        printf("Free(%zd)", p->size);
        p = p->next;
        if (p != NULL) {
          printf("->");
        }
      } 
      cout << endl;
    }
    page_num++;
  }
}

// Finds the best fit list for the requested size block 
// iterates over list sizes and finds list with enough memory
// PARAMETERS
// size - size of requested block in bytes
// RETURNS
// int - the offset into the lists to begin search for a block with enough memory
int find_best_fit(size_t size) {
  int block_size = 1 << 7;
  for (int i = 0; i < 5; i++) {
    if (block_size + sizeof(node_t) >= (size + sizeof(header_t))) {
      // list contains enough memory to accomdate size and header_t
      return i;
    }
    block_size = block_size << 1;
  }
  // not enough space, need to allocate addition 4KB page from memory!
  return -1;
}

// Finds a node on the free list that has enough available memory to
// allocate to a calling program. This function uses the "best-fit"
// algorithm to locate a free node.
// Max size supported is 2048 bytes, requesting beyond this causes an error.
// PARAMETERS:
// size - the number of bytes requested to allocate
// 
// RETURNS:
// found - the node found on the free list with enough memory to allocate
// previous - the previous node to the found node
// pg_num - the page number from which the memory was allocated
// exp - specifies the free_list from which memory was allocated
void find_free(size_t size, node_t **found, node_t **previous, int* pg_num, enum exponent* exp) {
  #define MAX_SIZE 2048
  if (size > MAX_SIZE + sizeof(node_t)) {
    cout << "Error, requested block size cannot exceed 2048 bytes" << endl;
    *found = NULL;
    *previous = NULL;
    return;
  }

  // offset into free lists
  int offset = find_best_fit(size);

  int index = 0;

  // iterate over all pages in 4KB until a free block has been found with enough memory
  for (auto it = heap.begin(); it != heap.end(); it++) {
    // iterate over free lists starting at offset in search of a sufficient free block
    int offset_copy = offset;
    for (int idx = offset; idx < 5; idx++) {
      node_t* cur = *((*it) + idx);
      if (cur != NULL) {
        if ((*cur).size + sizeof(node_t) >= size + sizeof(header_t)) {
          *found = cur;
          *previous = NULL;
          (*pg_num) = index;
          (*exp) = (enum exponent) offset_copy;
          return;
        }

        node_t* prev = cur;
        // iterate over current list
        while (prev != NULL && (prev -> next) != NULL) {
          if (((prev -> next) -> size) + sizeof(node_t) >= size + sizeof(header_t)) {
            *found = prev -> next;
            *previous = prev;
            (*pg_num) = index;
            (*exp) = (enum exponent) offset_copy;
            return;
          }
          prev = prev -> next;
        }
      }
      offset_copy++;
    } 
    index++;
  }
  // if this point has been reached, then there is not enough memory, 
  // need to allocate addition 4KB page
  create_heap();
  // recursively call find_free
  find_free(size, found, previous, pg_num, exp);  
}




// Splits a found free node to accommodate an allocation request.
//
// The job of this function is to take a given free_node found from
// `find_free` and split it according to the number of bytes to allocate.
// In doing so, it will adjust the size and next pointer of the `free_block`
// as well as the `previous` node to properly adjust the free list.
//
// PARAMETERS:
// size - the number of bytes requested to allocate
// previous - the previous node to the free block
// free_block - the node on the free list to allocate from
//
// RETURNS:
// allocated - an allocated block to be returned to the calling program
//
void split(size_t size, node_t **previous, node_t **free_block,
           header_t **allocated, int page_num, enum exponent exp) {

  assert(*free_block != NULL);

  if ((sizeof(header_t) + size) >= (*(*free_block)).size) {
    if (*previous != NULL) {
      (*previous) -> next = (*free_block) -> next;
    }
    else {
      // assign head of list to point to next node in list
      heap[page_num][exp] = (*free_block) -> next;
    }
    (*allocated) = (header_t*) (*free_block);
  }
  else {  
    header_t* header = (header_t*) (*free_block);
    size_t actual_size = size + sizeof(header_t);

    *free_block = (node_t *)(((char *)header) + actual_size);


    (**free_block).size = (((node_t*) header) -> size) - (size + sizeof(header_t)); 
    (**free_block).next = ((node_t*) header) -> next;
    if (*previous == NULL) {
      heap[page_num][exp] = *free_block;
    }
    else {
      (*previous) -> next = *free_block;
    }
    (*allocated) = header;
  }
  (*allocated) -> size = size;
  (*allocated) -> magic = MAGIC;
  (*allocated) -> page_num = page_num;
  (*allocated) -> exp = exp;
}

// Returns a pointer to a region of memory having at least the request `size`
// bytes.
//
// PARAMETERS:
// size - the number of bytes requested to allocate
//
// RETURNS:
// A void pointer to the region of allocated memory
//
void *my_malloc(size_t size) {
  // TODO
  node_t* found;
  node_t* previous;
  header_t* allocated;

  int page_num;
  enum exponent exp;

  find_free(size, &found, &previous, &page_num, &exp);
  if (found == NULL) {
    return NULL;
  }
  split(size, &previous, &found, &allocated, page_num, exp);
  return (void*) (allocated + 1);
}

// Merges adjacent nodes on the free list to reduce external fragmentation.
//
// This function will only coalesce nodes starting with `free_block`. It will
// not handle coalescing of previous nodes (we don't have previous pointers!).
//
// PARAMETERS:
// free_block - the starting node on the free list to coalesce
//
void coalesce(node_t *free_block) {
  
  while (free_block != NULL && free_block -> next != NULL) { 
    size_t block_size = free_block->size + sizeof(node_t);
    if (((char *) free_block) + block_size == (char*)(free_block -> next)) {
      free_block -> size += ((free_block -> next) -> size) + sizeof(node_t);
      free_block -> next = (free_block -> next) -> next;
    }
    else {
      break;
    }
  }
}

// Frees a given region of memory back to the free list.
//
// PARAMETERS:
// allocated - a pointer to a region of memory previously allocated by my_malloc
//
void my_free(void *allocated) {

  if (allocated != NULL) {
    header_t* header = (((header_t*) allocated) - 1);
    if (header -> magic == MAGIC) {
      int pg_num = header -> page_num;
      enum exponent exp = header -> exp;

      node_t* new_node = (node_t*) header;
      new_node -> size = (header -> size) + (sizeof(header_t) - sizeof(node_t));
      new_node -> next = heap[pg_num][exp];
      coalesce(new_node);
      heap[pg_num][exp] = new_node;
    }
  }
}
