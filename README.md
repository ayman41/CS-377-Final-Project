# Memory Allocator Project

## Feature 1: 5 Fixed-Sized Free-Lists to Track Free Blocks
    The memory allocator initializes five free lists per 4KB page allocated from physical memory.
    The first list supports a maximum of 2^7 bytes, the second 2^8, the third 2^9, the fourth 2^10, and the fifth
    2^11 bytes. Therefore, the allocator supports a maximum of 2^11 bytes. The init_free_lists function is used to initialize all
    lists on a new page, which creates the list pointers and pushes them as a separate element into the heap vector array, 
    setting the size and next pointers appropriately for each list. The create_heap function is used to allocate a 4KB page and
    subsequently initialize the lists on the page, and the reset_heap function is provided for debugging purposes to 
    clear heap (all allocated 4KB pages).

    The available_memory function returns the total amount of memory in bytes available in the heap across all allocated pages. 
    This involves iterating over the heap vector through all five lists in each page and summing up the sizes. 
    The number_of_free_nodes function operates similarly but takes in an enum exponent argument which specifies 
    which list to search through. Enum exponent can take the values Seven, Eight, ... , Eleven, 
    which indicate the list that supports 2^(exponent) bytes. Print_free_list function iterates 
    over the heap and prints out all free lists for all five lists on a separate line so that they can 
    easily be distinguished.

## Feature 2: Best Fit Approach to Find Free Block
    An additional feature of this allocator is the use of best fit approach instead of first fit in finding a 
    free block from the free list. The best_fit function is used to find the first list number (0-4) that may 
    have enough memory for the requested block. This operation is constant time because there are a constant (5) lists. 
    Find_free function calls the best_fit function in order to determine the starting list to begin search for a free block 
    with sufficient memory. If the current list does not have enough memory, then find_free continues the search in the next list. 
    If no free block has sufficient memory, then find_free calls create_heap to allocate an additional 4KB page and 
    recursively calls itself to find a block in this new page. Find_free is very efficient because in most cases the 
    first few free blocks will likely provide enough memory, and if not, an additional page will be allocated, 
    so subsequent allocation requests will be fast. 

## Feature 3: Allocate additional 4KB Pages when Needed

    Another feature added into this allocator is the ability to allocate new 4KB pages when there is not enough memory in the 
    heap for a particular request. This is accomplished using a vector that contains pointers to the free lists for each 
    allocated page. The type of the vector is vector<node_t**>. For instance, vector[0] represents the free lists for the first
    allocated page, and vector[0][0] is the pointer to the first free list that supports a maximum of 2^7 bytes. 
    Pushing new elements into the vector involves allocating an additional 4KB page from memory by calling create_heap. 


## Link to Presentation 

[Link to presentation and demo of allocator project](https://drive.google.com/file/d/1XMgIs0LOXpN_2-ORC3PfM28NvjjNaxxw/view?usp=sharing)
