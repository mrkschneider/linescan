#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LINESCAN_DEBUG
#define LINESCAN_CHECK(expression,return_value)				\
  if(!(expression)){							\
    fprintf(stderr,"linescan check failed: '%s' in %s at %s:%d\n",	\
	    #expression,__func__,__FILE__,__LINE__);			\
    if(errno) perror("Error: ");					\
    return return_value;						\
  }
#define LINESCAN_DBG(expression) expression
#else
#define LINESCAN_CHECK(expression,return_value) 
#define LINESCAN_DBG(expression) 
#endif
  
  /* Container for linescan results */
  typedef struct linescan {
    // Input search buffer
    const char* buf;
    // Number of characters between buf and end of search
    size_t size;
    /* Offsets to search start, cmask matches (see find/rfind), and search stop.
       Pointers can be obtained via (buf + offset) */
    size_t* offsets;
    // Number of offsets found (including start and end locations)
    size_t offsets_n;
    // Maximum number of offsets to store (not enforced)
    size_t offsets_size;

    #ifdef LINESCAN_DEBUG
    size_t debug_steps_1;
    size_t debug_steps_2;
    size_t debug_steps_3;
    #endif
    
  } linescan;

  linescan* linescan_create(size_t offsets_size);
  void linescan_free(linescan* r);
  void linescan_reset(linescan* r);

  /* Create mask to speed up character searches.
     @param[c] Character to create mask for.
     @returns Word containing c in every byte.
  */
  uint64_t linescan_create_mask(char c);

  /* Search buffer left-to-right for occurences of character (described by cmask) for
     max n characters or until newline \n is encountered.
     @param[buf] Buffer to search
     @param[cmask] Character mask to match (see linescan_create_mask).
     @param[n] Maximum number of characters to search; must be >= 0
     @param[result] Struct to which results are written.
     @returns 1 if newline is encountered; 0 if no newline was found after n steps. -1 indicates an error.
  */
  int linescan_find(const char* buf, uint64_t cmask, size_t n, linescan* result);
  /* Search buffer right-to-left for occurences of character (described by cmask) for
     max n characters or until newline \n is encountered. Search starts at (buf + (n - 1))
     and traverses towards buf.
     @param[buf] Buffer to search
     @param[cmask] Character mask to match (see linescan_create_mask).
     @param[n] Maximum number of characters to search; must be >= 0
     @param[result] Struct to which results are written.
     @returns 1 if newline is encountered; 0 if no newline was found after n steps. -1 indicates an error.
  */
  int linescan_rfind(const char* buf, uint64_t cmask, size_t n, linescan* result);

#ifdef __cplusplus
}
#endif
