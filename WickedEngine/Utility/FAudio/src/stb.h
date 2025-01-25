/* stb.h - v2.32 - Sean's Tool Box -- public domain -- http://nothings.org/stb.h
          no warranty is offered or implied; use this code at your own risk

   This is a single header file with a bunch of useful utilities
   for getting stuff done in C/C++.

   Documentation: http://nothings.org/stb/stb_h.html
   Unit tests:    http://nothings.org/stb/stb.c


 ============================================================================
   You MUST                                                                  
                                                                             
      #define STB_DEFINE                                                     
                                                                             
   in EXACTLY _one_ C or C++ file that includes this header, BEFORE the
   include, like this:                                                                
                                                                             
      #define STB_DEFINE                                                     
      #include "stb.h"
      
   All other files should just #include "stb.h" without the #define.
 ============================================================================


Version History

   2.32   stb_intcmprev, stb_uidict, fix random numbers on Linux
   2.31   stb_ucharcmp
   2.30   MinGW fix
   2.29   attempt to fix use of swprintf()
   2.28   various new functionality
   2.27   test _WIN32 not WIN32 in STB_THREADS
   2.26   various warning & bugfixes
   2.25   various warning & bugfixes
   2.24   various warning & bugfixes
   2.23   fix 2.22
   2.22   64-bit fixes from '!='; fix stb_sdict_copy() to have preferred name
   2.21   utf-8 decoder rejects "overlong" encodings; attempted 64-bit improvements
   2.20   fix to hash "copy" function--reported by someone with handle "!="
   2.19   ???
   2.18   stb_readdir_subdirs_mask
   2.17   stb_cfg_dir
   2.16   fix stb_bgio_, add stb_bgio_stat(); begin a streaming wrapper
   2.15   upgraded hash table template to allow:
            - aggregate keys (explicit comparison func for EMPTY and DEL keys)
            - "static" implementations (so they can be culled if unused)
   2.14   stb_mprintf
   2.13   reduce identifiable strings in STB_NO_STB_STRINGS
   2.12   fix STB_ONLY -- lots of uint32s, TRUE/FALSE things had crept in
   2.11   fix bug in stb_dirtree_get() which caused "c://path" sorts of stuff
   2.10   STB_F(), STB_I() inline constants (also KI,KU,KF,KD)
   2.09   stb_box_face_vertex_axis_side
   2.08   bugfix stb_trimwhite()
   2.07   colored printing in windows (why are we in 1985?)
   2.06   comparison functions are now functions-that-return-functions and
          accept a struct-offset as a parameter (not thread-safe)
   2.05   compile and pass tests under Linux (but no threads); thread cleanup
   2.04   stb_cubic_bezier_1d, smoothstep, avoid dependency on registry
   2.03   ?
   2.02   remove integrated documentation
   2.01   integrate various fixes; stb_force_uniprocessor
   2.00   revised stb_dupe to use multiple hashes
   1.99   stb_charcmp
   1.98   stb_arr_deleten, stb_arr_insertn
   1.97   fix stb_newell_normal()
   1.96   stb_hash_number()
   1.95   hack stb__rec_max; clean up recursion code to use new functions
   1.94   stb_dirtree; rename stb_extra to stb_ptrmap
   1.93   stb_sem_new() API cleanup (no blockflag-starts blocked; use 'extra')
   1.92   stb_threadqueue--multi reader/writer queue, fixed size or resizeable
   1.91   stb_bgio_* for reading disk asynchronously
   1.90   stb_mutex uses CRITICAL_REGION; new stb_sync primitive for thread
          joining; workqueue supports stb_sync instead of stb_semaphore
   1.89   support ';' in constant-string wildcards; stb_mutex wrapper (can
          implement with EnterCriticalRegion eventually)
   1.88   portable threading API (only for win32 so far); worker thread queue
   1.87   fix wildcard handling in stb_readdir_recursive
   1.86   support ';' in wildcards
   1.85   make stb_regex work with non-constant strings;
               beginnings of stb_introspect()
   1.84   (forgot to make notes)
   1.83   whoops, stb_keep_if_different wasn't deleting the temp file
   1.82   bring back stb_compress from stb_file.h for cmirror
   1.81   various bugfixes, STB_FASTMALLOC_INIT inits FASTMALLOC in release
   1.80   stb_readdir returns utf8; write own utf8-utf16 because lib was wrong
   1.79   stb_write
   1.78   calloc() support for malloc wrapper, STB_FASTMALLOC
   1.77   STB_FASTMALLOC
   1.76   STB_STUA - Lua-like language; (stb_image, stb_csample, stb_bilinear)
   1.75   alloc/free array of blocks; stb_hheap bug; a few stb_ps_ funcs;
          hash*getkey, hash*copy; stb_bitset; stb_strnicmp; bugfix stb_bst
   1.74   stb_replaceinplace; use stdlib C function to convert utf8 to UTF-16
   1.73   fix performance bug & leak in stb_ischar (C++ port lost a 'static')
   1.72   remove stb_block, stb_block_manager, stb_decompress (to stb_file.h)
   1.71   stb_trimwhite, stb_tokens_nested, etc.
   1.70   back out 1.69 because it might problemize mixed builds; stb_filec()
   1.69   (stb_file returns 'char *' in C++)
   1.68   add a special 'tree root' data type for stb_bst; stb_arr_end
   1.67   full C++ port. (stb_block_manager)
   1.66   stb_newell_normal
   1.65   stb_lex_item_wild -- allow wildcard items which MUST match entirely
   1.64   stb_data
   1.63   stb_log_name
   1.62   stb_define_sort; C++ cleanup
   1.61   stb_hash_fast -- Paul Hsieh's hash function (beats Bob Jenkins'?)
   1.60   stb_delete_directory_recursive
   1.59   stb_readdir_recursive
   1.58   stb_bst variant with parent pointer for O(1) iteration, not O(log N)
   1.57   replace LCG random with Mersenne Twister (found a public domain one)
   1.56   stb_perfect_hash, stb_ischar, stb_regex
   1.55   new stb_bst API allows multiple BSTs per node (e.g. secondary keys)
   1.54   bugfix: stb_define_hash, stb_wildmatch, regexp
   1.53   stb_define_hash; recoded stb_extra, stb_sdict use it
   1.52   stb_rand_define, stb_bst, stb_reverse
   1.51   fix 'stb_arr_setlen(NULL, 0)'
   1.50   stb_wordwrap
   1.49   minor improvements to enable the scripting language
   1.48   better approach for stb_arr using stb_malloc; more invasive, clearer
   1.47   stb_lex (lexes stb.h at 1.5ML/s on 3Ghz P4; 60/70% of optimal/flex)
   1.46   stb_wrapper_*, STB_MALLOC_WRAPPER
   1.45   lightly tested DFA acceleration of regexp searching
   1.44   wildcard matching & searching; regexp matching & searching
   1.43   stb_temp
   1.42   allow stb_arr to use stb_malloc/realloc; note this is global
   1.41   make it compile in C++; (disable stb_arr in C++)
   1.40   stb_dupe tweak; stb_swap; stb_substr
   1.39   stb_dupe; improve stb_file_max to be less stupid
   1.38   stb_sha1_file: generate sha1 for file, even > 4GB
   1.37   stb_file_max; partial support for utf8 filenames in Windows
   1.36   remove STB__NO_PREFIX - poor interaction with IDE, not worth it
          streamline stb_arr to make it separately publishable
   1.35   bugfixes for stb_sdict, stb_malloc(0), stristr
   1.34   (streaming interfaces for stb_compress)
   1.33   stb_alloc; bug in stb_getopt; remove stb_overflow
   1.32   (stb_compress returns, smaller&faster; encode window & 64-bit len)
   1.31   stb_prefix_count
   1.30   (STB__NO_PREFIX - remove stb_ prefixes for personal projects)
   1.29   stb_fput_varlen64, etc.
   1.28   stb_sha1
   1.27   ?
   1.26   stb_extra
   1.25   ?
   1.24   stb_copyfile
   1.23   stb_readdir
   1.22   ?
   1.21   ?
   1.20   ?
   1.19   ?
   1.18   ?
   1.17   ?
   1.16   ?
   1.15   stb_fixpath, stb_splitpath, stb_strchr2
   1.14   stb_arr
   1.13   ?stb, stb_log, stb_fatal
   1.12   ?stb_hash2
   1.11   miniML
   1.10   stb_crc32, stb_adler32
   1.09   stb_sdict
   1.08   stb_bitreverse, stb_ispow2, stb_big32
          stb_fopen, stb_fput_varlen, stb_fput_ranged
          stb_fcmp, stb_feq
   1.07   (stb_encompress)
   1.06   stb_compress
   1.05   stb_tokens, (stb_hheap)
   1.04   stb_rand
   1.03   ?(s-strings)
   1.02   ?stb_filelen, stb_tokens
   1.01   stb_tolower
   1.00   stb_hash, stb_intcmp
          stb_file, stb_stringfile, stb_fgets
          stb_prefix, stb_strlower, stb_strtok
          stb_image
          (stb_array), (stb_arena)

Parenthesized items have since been removed.

LICENSE

 See end of file for license information.

CREDITS

 Written by Sean Barrett.

 Fixes:
  Philipp Wiesemann
  Robert Nix
  r-lyeh
  blackpawn
  github:Mojofreem
  Ryan Whitworth
  Vincent Isambart
  Mike Sartain
  Eugene Opalev
  Tim Sjostrand
  github:infatum
  Dave Butler (Croepha)
*/

#ifndef STB__INCLUDE_STB_H
#define STB__INCLUDE_STB_H

#define STB_VERSION  1

/* In addition to trimming out all the stuff FAudio does not use, we are also
 * binding various stdlib functions stb.h uses to FAudio's stdlib.
 * -flibit
 */
#ifdef STB_MEMCPY_OVERRIDE
#ifdef memcpy /* Thanks Apple! */
#undef memcpy
#endif
#define memcpy FAudio_memcpy
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         Miscellany
//

STB_EXTERN void stb_swap(void *p, void *q, size_t sz);

#ifdef STB_DEFINE
typedef struct { char d[4]; } stb__4;
typedef struct { char d[8]; } stb__8;

// optimize the small cases, though you shouldn't be calling this for those!
void stb_swap(void *p, void *q, size_t sz)
{
   char buffer[256];
   if (p == q) return;
   if (sz == 4) {
      stb__4 temp    = * ( stb__4 *) p;
      * (stb__4 *) p = * ( stb__4 *) q;
      * (stb__4 *) q = temp;
      return;
   } else if (sz == 8) {
      stb__8 temp    = * ( stb__8 *) p;
      * (stb__8 *) p = * ( stb__8 *) q;
      * (stb__8 *) q = temp;
      return;
   }

   while (sz > sizeof(buffer)) {
      stb_swap(p, q, sizeof(buffer));
      p = (char *) p + sizeof(buffer);
      q = (char *) q + sizeof(buffer);
      sz -= sizeof(buffer);
   }

   memcpy(buffer, p     , sz);
   memcpy(p     , q     , sz);
   memcpy(q     , buffer, sz);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//               Random Numbers via Meresenne Twister or LCG
//

STB_EXTERN unsigned int  stb_srandLCG(unsigned int seed);
STB_EXTERN unsigned int  stb_randLCG(void);

STB_EXTERN void          stb_srand(unsigned int seed);
STB_EXTERN unsigned int  stb_rand(void);
STB_EXTERN double        stb_frand(void);

#define stb_rand_define(x,y)                                         \
                                                                     \
   unsigned int  x(void)                                             \
   {                                                                 \
      static unsigned int  stb__rand = y;                            \
      stb__rand = stb__rand * 2147001325 + 715136305; /* BCPL */     \
      return 0x31415926 ^ ((stb__rand >> 16) + (stb__rand << 16));   \
   }

#ifdef STB_DEFINE
static unsigned int  stb__rand_seed=0;

unsigned int  stb_srandLCG(unsigned int seed)
{
   unsigned int  previous = stb__rand_seed;
   stb__rand_seed = seed;
   return previous;
}

unsigned int  stb_randLCG(void)
{
   stb__rand_seed = stb__rand_seed * 2147001325 + 715136305; // BCPL generator
   // shuffle non-random bits to the middle, and xor to decorrelate with seed
   return 0x31415926 ^ ((stb__rand_seed >> 16) + (stb__rand_seed << 16));
}

// public domain Mersenne Twister by Michael Brundage
#define STB__MT_LEN       624

int stb__mt_index = STB__MT_LEN*sizeof(int)+1;
unsigned int stb__mt_buffer[STB__MT_LEN];

void stb_srand(unsigned int seed)
{
   int i;
   unsigned int old = stb_srandLCG(seed);
   for (i = 0; i < STB__MT_LEN; i++)
      stb__mt_buffer[i] = stb_randLCG();
   stb_srandLCG(old);
   stb__mt_index = STB__MT_LEN*sizeof(unsigned int);
}

#define STB__MT_IA           397
#define STB__MT_IB           (STB__MT_LEN - STB__MT_IA)
#define STB__UPPER_MASK      0x80000000
#define STB__LOWER_MASK      0x7FFFFFFF
#define STB__MATRIX_A        0x9908B0DF
#define STB__TWIST(b,i,j)    ((b)[i] & STB__UPPER_MASK) | ((b)[j] & STB__LOWER_MASK)
#define STB__MAGIC(s)        (((s)&1)*STB__MATRIX_A)

unsigned int stb_rand()
{
   unsigned int  * b = stb__mt_buffer;
   int idx = stb__mt_index;
   unsigned int  s,r;
   int i;
	
   if (idx >= STB__MT_LEN*sizeof(unsigned int)) {
      if (idx > STB__MT_LEN*sizeof(unsigned int))
         stb_srand(0);
      idx = 0;
      i = 0;
      for (; i < STB__MT_IB; i++) {
         s = STB__TWIST(b, i, i+1);
         b[i] = b[i + STB__MT_IA] ^ (s >> 1) ^ STB__MAGIC(s);
      }
      for (; i < STB__MT_LEN-1; i++) {
         s = STB__TWIST(b, i, i+1);
         b[i] = b[i - STB__MT_IB] ^ (s >> 1) ^ STB__MAGIC(s);
      }
      
      s = STB__TWIST(b, STB__MT_LEN-1, 0);
      b[STB__MT_LEN-1] = b[STB__MT_IA-1] ^ (s >> 1) ^ STB__MAGIC(s);
   }
   stb__mt_index = idx + sizeof(unsigned int);
   
   r = *(unsigned int *)((unsigned char *)b + idx);
   
   r ^= (r >> 11);
   r ^= (r << 7) & 0x9D2C5680;
   r ^= (r << 15) & 0xEFC60000;
   r ^= (r >> 18);
   
   return r;
}

double stb_frand(void)
{
   return stb_rand() / ((double) (1 << 16) * (1 << 16));
}

#endif

#undef STB_EXTERN
#endif // STB_INCLUDE_STB_H

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
