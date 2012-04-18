/**********************************************************************

  eden_alloc.c - allocator for eden heap area

  $Author$
  created at: Mon Apr  16 09:44:46 JST 2012

  included from gc.c

**********************************************************************/

#include <unistd.h>

#define HASH_FUNC0(obj) ((((uintptr_t)(obj)) / 127) & 0x7f)
#define HASH_FUNC1(obj) (((((uintptr_t)(obj)) >> 1) + (((uintptr_t)(obj)) / 255)) & 0x7f)

int
eden_add_table(ggrb_eden_arena_node_t *table[2][128], ggrb_eden_arena_node_t *val)
{
  int tcnt = 0;
  ggrb_eden_arena_node_t *nval;
  ggrb_eden_arena_node_t *cval;
  int hv;

  cval = val;
  for (tcnt = 0; tcnt < 10; tcnt++) {

    /* insert table 0 */
    hv = HASH_FUNC0(cval);
    if (nval = table[0][hv]) {
      table[0][hv] = cval;
      cval = nval;
    }
    else {
      table[0][hv] = cval;
      return 1;
    }

    /* insert table 1 */
    hv = HASH_FUNC1(cval);
    if (nval = table[1][hv]) {
      table[1][hv] = cval;
      cval = nval;
    }
    else {
      table[1][hv] = cval;
      return 1;
    }
  }

  /* Opps! fail to insert. Rollback and drop val*/
  while (cval != val) {
    /* rollback table 0 */
    hv = HASH_FUNC0(cval);
    nval = table[0][hv];
    table[0][hv] = cval;
    cval = nval;

    /* rollback table 1 */
    hv = HASH_FUNC1(cval);
    nval = table[1][hv];
    table[1][hv] = cval;
    cval = nval;
  }

  /* Overwrite entry of val. There is no entry of val. */
  table[0][hv] = cval;

  /* Fail to insert. You must retry */
  return 0;
}

void
add_eden_arena(rb_objspace_t *objspace)
{
  unsigned i;
  ggrb_eden_arena_node_t *arena;
  ggrb_eden_arena_node_t *narena;

  arena = NULL;
  do {
    narena = (ggrb_eden_arena_node_t *)aligned_malloc(HEAP_ALIGN, EDEN_ARENA_SIZE);
    if (arena) {
      aligned_free(arena);
    }
    arena = narena;
    arena->next = objspace->eden_arena_root;
    for (i = 0; i < EDEN_BITMAP_SIZE; i++) {
      arena->bitmap[i] = -1;
    }
    i--;
    arena->bitmap[i] ^= 0x7 << 29;
    objspace->eden_arena_root = arena;
    objspace->eden_arena_current = arena;
  } while (!eden_add_table(objspace->eden_arena_tab, arena));
}

eden_body_t *
eden_alloc(void)
{
  rb_objspace_t *objspace = &rb_objspace;
  ggrb_eden_arena_node_t *arena = objspace->eden_arena_current;

  unsigned bitmappos = objspace->eden_last_allocae_pos;
  unsigned *bitmap;
  unsigned bitcont;
  int freepos;

 retry:

  bitmap = arena->bitmap;
  /* Skip bitmap all used */
  while (bitmap[bitmappos] == 0){
    bitmappos++;
  }
  if (bitmappos >= EDEN_BITMAP_SIZE) {
    arena = arena->next;
    objspace->eden_arena_current = arena;
    if (arena) {
      bitmappos = 0;		/* Try next arena node */
      puts("Change\n");
      goto retry;
    } 
    else {
      // eden_gc(objspace);
      puts("You must GC\n");
    }
  }

  objspace->eden_last_allocae_pos = bitmappos;
  bitcont = bitmap[bitmappos];
  freepos = ffs(bitcont) - 1;
  bitmap[bitmappos] = bitcont & (bitcont - 1);

  return (arena->body + EDEN_BODY_OFFSET(bitmappos, freepos));
}

void
eden_free(eden_body_t *obj)
{
  ggrb_eden_arena_node_t *arena;
  int bitmappos;
  int byteoff;
  int bitoff;

  arena = EDEN_OBJ2TOP(obj);
  bitmappos = (uintptr_t)obj - (uintptr_t)arena;
  bitmappos -= offsetof(ggrb_eden_arena_node_t, body);
  bitmappos /= sizeof(eden_body_t);
  byteoff = EDEN_BITMAP_BYTEOFF(bitmappos);
  bitoff = EDEN_BITMAP_BITOFF(bitmappos);

  arena->bitmap[byteoff] |= (1 << bitoff);
}
