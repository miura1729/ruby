/**********************************************************************

  eden_alloc.h - allocator for eden heap arena

  $Author$
  created at: Mon Apr  16 09:44:46 JST 2012

  included from gc.c

**********************************************************************/

/* eden_arena is same align and size. Current size is 16 Kbytes.
   Structure of eden_arena is following format

    Hi    16K(32k) |                           |
                   +---------------------------+
                   |  allocation area          | 
         alocarea  |                           |
	           |                           |
                   + Gatekeeper all 1          +
	   64(128) +---------------------------+
                   |  bitmap(1 free, 0 used)   |
                   |                           |
              4(8) +---------------------------+
   Lo           0  | next                      |
                   +---------------------------+


  Size of RVALIE is 20 or 40. But eden_alloc returns chunk whose size is 
  32 or 64. Because it is faster than 20 or 40 and eden_arena is small than
  other arena - so small impact for Ruby memory usage.
*/
#define EDEN_ARENA_PAGE (16 * 1024)
#define EDEN_RVALUE_NUM ((EDEN_ARENA_PAGE - 8) / (8 * sizeof(VALUE)))
#define EDEN_BITMAP_SIZE (((EDEN_RVALUE_NUM + 63) / 64) * (8 / sizeof(unsigned)))
#define EDEN_ARENA_SIZE (sizeof(struct eden_arena_node) + (8 * sizeof(VALUE)* EDEN_RVALUE_NUM))

#define EDEN_BODY_OFFSET(byteoff, bitoff) \
  ((byteoff) * sizeof(unsigned) + (bitoff))
#define EDEN_BITMAP_BYTEOFF(body_offset) \
  ((body_offset) / sizeof(unsigned))
#define EDEN_BITMAP_BITOFF(body_offset) \
  ((body_offset) % sizeof(unsigned))

typedef struct eden_arena_node {
  struct eden_arena_node *next;
  unsigned bitmap[EDEN_BITMAP_SIZE + 1];
  RVALUE body[1];
} ggrb_eden_arena_node_t;

