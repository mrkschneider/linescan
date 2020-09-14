/* This file contains derived functions of the GNU C Library, specifically 
   string/memchr.c and string/memrchr.c, which are published under the terms of the 
   GNU Lesser General Public License version 2.1. The corresponding copyright notices 
   are reproduced below.

   linescan - fast character and newline search in buffers
   Copyright (C) 2020 Markus Schneider

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

   Copyright notice string/memchr.c:

   Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Based on strlen implementation by Torbjorn Granlund (tege@sics.se),
   with help from Dan Sahlin (dan@sics.se) and
   commentary by Jim Blandy (jimb@ai.mit.edu);
   adaptation to memchr suggested by Dick Karpinski (dick@cca.ucsf.edu),
   and implemented by Roland McGrath (roland@ai.mit.edu).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  

   Copyright notice string/memrchr.c:

   memrchr -- find the last occurrence of a byte in a memory block
   Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Based on strlen implementation by Torbjorn Granlund (tege@sics.se),
   with help from Dan Sahlin (dan@sics.se) and
   commentary by Jim Blandy (jimb@ai.mit.edu);
   adaptation to memchr suggested by Dick Karpinski (dick@cca.ucsf.edu),
   and implemented by Roland McGrath (roland@ai.mit.edu).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>
*/

#include <linescan.h>

// Mask containing 00000001 in every byte
static const uint64_t ONES_MASK = (uint64_t)0x01010101 | ((uint64_t)0x01010101) << 32;
// Mask containing 10000000 in every byte
static const uint64_t ONES_MASK_7 = ONES_MASK << 7;

static const unsigned char NL = '\n';
// Mask containing NL in every byte
static const uint64_t NL_MASK = (uint64_t)NL
  | (((uint64_t)NL) << 8)
  | (((uint64_t)NL) << 16)
  | (((uint64_t)NL) << 24)
  | (((uint64_t)NL) << 32)
  | (((uint64_t)NL) << 40)
  | (((uint64_t)NL) << 48)
  | (((uint64_t)NL) << 56);

// Magic mask used by rfind
static const uint64_t RFIND_MAGIC_MASK = ((uint64_t)-1) / 0xff * 0xfe << 1 >> 1 | 1; 

// private helper method for setting some values
static inline void linescan_update(linescan* r,
				   const char* buf,
				   size_t size,
				   size_t offsets_n){
  r->buf = buf;
  r->offsets_n = offsets_n;
  r->size = size;
}

void linescan_reset(linescan* r){
  r->buf = NULL;
  r->size = 0;
  r->offsets_n = 0;
}

#ifdef LINESCAN_DEBUG
void linescan_reset_debug(linescan* r){
  r->debug_steps_1 = 0;
  r->debug_steps_2 = 0;
  r->debug_steps_3 = 0;
}
#endif

linescan* linescan_create(size_t offsets_size){
  linescan* r = malloc(sizeof(linescan));      
  size_t* offsets = calloc(offsets_size, sizeof(size_t));
  r->offsets = offsets;
  r->offsets_size = offsets_size;
  linescan_reset(r);
  return r;
}

void linescan_free(linescan* r){
  free(r->offsets);
  free(r);
}

__attribute__ ((pure)) uint64_t linescan_create_mask(char c) {
    uint64_t result = c;
    result |= result << 8;
    result |= result << 16;
    result |= result << 32;
    return result;
}

/* Adapted from glibc string/memchr.c
   HERE BE DRAGONS.
   This code famously relies on undefined behavior w/ strict pointer aliasing.
   Accessing char* with a longword pointer is actually not allowed according to C11.
   The code usually still works because (a) we do not write to buf in this function and
   (b) separate compilation prevents the compiler from problematic optimizations due
   to the type of buf.
 */
int linescan_find(const char* buf, uint64_t cmask, size_t n, linescan* result){
  LINESCAN_CHECK(buf != NULL, -1)
  LINESCAN_CHECK(result != NULL, -1)
  LINESCAN_DBG(linescan_reset_debug(result);)
  
  const unsigned char* b;
  unsigned char c_ref = (unsigned char)cmask;
  size_t* offsets = result->offsets;
  size_t n0 = n;

  offsets[0] = 0;
  size_t offsets_n = 1;

  /* Step 1: Search every byte until pointer is aligned to uint64_t */
  for(b = (const unsigned char*)buf;
      n > 0 && (size_t)b % 8 !=0;
      n--, b++){
    unsigned char c = *b;
    LINESCAN_DBG(result->debug_steps_1++;)
    if(c == c_ref){
      offsets[offsets_n] = n0 - n;
      offsets_n++;
    } else if(c == NL){
      size_t offset = n0 - n;
      offsets[offsets_n] = offset;
      offsets_n++;
      linescan_update(result, buf, offset+1, offsets_n);
      return 1;
    }
  }

  const uint64_t* lb = (const uint64_t*)b;

  /* Step 2: Search multiple bytes using uint64_t masks (see memchr for details) */
  while(n >= 8){
    LINESCAN_DBG(result->debug_steps_2++;)
    uint64_t w_nl = *lb ^ NL_MASK;
    
    if ((((w_nl - ONES_MASK) & ~w_nl) & ONES_MASK_7) != 0){
      break;
    }

    uint64_t w_t = *lb ^ cmask;
    if ((((w_t - ONES_MASK) & ~w_t) & ONES_MASK_7) != 0){
      for(size_t i=0;i<8;i++){
	unsigned char c = ((const unsigned char*)lb)[i];
	size_t offset_base = n0 - n;
	if(c == c_ref) {
	  offsets[offsets_n] = offset_base + i;
	  offsets_n++;
	}
      }
    }
    lb++;
    n -= 8;
  }

  b = (const unsigned char*)lb;

  /* Step 3: There is less than 8 bytes left to search (either by n shrinking 
     or a detected newline in the last step. */
  for(; n > 0; n--, b++){
    LINESCAN_DBG(result->debug_steps_3++;)
    unsigned char c = *b;
    if(c == c_ref){
      offsets[offsets_n] = n0 - n;
      offsets_n++;
    } else if(c == NL){
      size_t offset = n0 - n;
      offsets[offsets_n] = offset;
      offsets_n++;
      linescan_update(result, buf, offset + 1, offsets_n);
      return 1;
    }
  }

  linescan_update(result, buf, n0, offsets_n);
  return 0;
}

/* Adapted from glibc string/memrchr.c
   HERE BE DRAGONS.
   See linescan_find for details about undefined behavior.
 */
int linescan_rfind(const char* buf, uint64_t cmask, size_t n, linescan* result){
  LINESCAN_CHECK(buf != NULL, -1)
  LINESCAN_CHECK(result != NULL, -1)
  LINESCAN_DBG(linescan_reset_debug(result);)
  const unsigned char* b;
  unsigned char c_ref = (unsigned char)cmask;
  size_t* offsets = result->offsets;
  size_t n0 = n;

  offsets[0] = n-1;
  size_t offsets_n = 1;

  /* Step 1: Search every byte until pointer is aligned to uint64_t */
  for(b = (const unsigned char*)buf + n;
      n > 0 && ((uint64_t)b & (uint64_t)7) !=0;
      n--){
    LINESCAN_DBG(result->debug_steps_1++;)
    unsigned char c = *--b;
    if(c == c_ref){
      offsets[offsets_n] = n - 1;
      offsets_n++;
    } else if(c == NL){
      offsets[offsets_n] = n - 1;
      offsets_n++;
      linescan_update(result, buf, n0 - n + 1, offsets_n);
      return 1;
    }
  }

  const uint64_t* lb = (const uint64_t*)b;

  /* Step 2: Search multiple bytes using uint64_t masks (see memrchr for details) */
  while(n >= 8){
    LINESCAN_DBG(result->debug_steps_2++;)
    n -= 8;
    uint64_t w_t = *--lb ^ cmask;
    uint64_t w_nl = *lb ^ NL_MASK;

    if ((((w_t + RFIND_MAGIC_MASK) ^ ~w_t) & ~RFIND_MAGIC_MASK) != 0
	||
	(((w_nl + RFIND_MAGIC_MASK) ^ ~w_nl) & ~RFIND_MAGIC_MASK) != 0
	){
      for(int i=7;i>=0;i--){
	unsigned char c = ((const unsigned char*)lb)[i];
	size_t offset_base = n;
	if(c == c_ref) {
	  offsets[offsets_n] = offset_base + i;
	  offsets_n++;
	} else if(c == NL) {
	  size_t offset = offset_base + i;
	  offsets[offsets_n] = offset;
	  offsets_n++;
	  linescan_update(result, buf, n0 - offset, offsets_n);
	  return 1;
	}
      }
    }

  }

  b = (const unsigned char*)lb;

  /* Step 3: There is less than 8 bytes left to search */
  while(n-- > 0){
    LINESCAN_DBG(result->debug_steps_3++;)
    unsigned char c = *--b;
    if(c == c_ref){
      offsets[offsets_n] = n;
      offsets_n++;
    } else if(c == NL){
      offsets[offsets_n] = n;
      offsets_n++;
      linescan_update(result, buf, n0 - n, offsets_n);
      return 1;
    }
  }

  linescan_update(result, buf, n0, offsets_n);
  return 0;
}


  

  

      
