/*
* savebuffer.c
* Luabins save buffer
* See copyright notice in luabins.h
*/

#include <string.h> /* memcpy() */

#include "luaheaders.h"

#include "saveload.h"
#include "savebuffer.h"

#if 0
  #define SPAM(a) printf a
#else
  #define SPAM(a) (void)0
#endif

/* TODO: Test this with custom allocator! */

void lbsSB_init(
    luabins_SaveBuffer * sb,
    lua_Alloc alloc_fn,
    void * alloc_ud,
    size_t block_size
  )
{
  sb->alloc_fn = alloc_fn;
  sb->alloc_ud = alloc_ud;

  sb->block_size = block_size;

  sb->buffer = NULL;
  sb->buf_size = 0UL;

  sb->end = 0UL;
}

/*
* Ensures that there is at least delta size available in buffer.
* New size is aligned by blockSize increments
* Returns non-zero if resize failed.
* If you pre-sized the buffer, subsequent writes up to the new size
* are guaranteed to not fail.
*/
int lbsSB_grow(luabins_SaveBuffer * sb, size_t delta)
{
  size_t newsize = sb->end + delta;
  SPAM(("end %lu delta %lu newsize %lu\n", sb->end, delta, newsize));

  if (newsize > sb->buf_size)
  {
    /* Pad to the next block boundary */
    size_t remainder = (newsize % sb->block_size);
    if (remainder > 0)
    {
      newsize += sb->block_size - remainder;
    }

    SPAM(("growing from %lu to %lu\n", sb->buf_size, newsize));

    sb->buffer = sb->alloc_fn(sb->alloc_ud, sb->buffer, sb->buf_size, newsize);
    if (sb->buffer == NULL)
    {
      sb->buf_size = 0UL;
      sb->end = 0;
      return LUABINS_ETOOLONG;
    }

    sb->buf_size = newsize;
  }

  return LUABINS_ESUCCESS;
}

/*
* Returns non-zero if write failed.
* Allocates buffer as needed.
*/
int lbsSB_write(
    luabins_SaveBuffer * sb,
    const unsigned char * bytes,
    size_t length
  )
{
  int result = lbsSB_grow(sb, length);
  if (result != LUABINS_ESUCCESS)
  {
    return result;
  }

  memcpy(&sb->buffer[sb->end], bytes, length);
  sb->end += length;

  return LUABINS_ESUCCESS;
}

/*
* If offset is greater than total length, data is appended to the end.
* Returns non-zero if write failed.
* Allocates buffer as needed.
*/
int lbsSB_overwrite(
    luabins_SaveBuffer * sb,
    size_t offset,
    const unsigned char * bytes,
    size_t length
  )
{
  if (offset > sb->end)
  {
    offset = sb->end;
  }

  if (offset + length > sb->end)
  {
    int result = lbsSB_grow(sb, length);
    if (result != LUABINS_ESUCCESS)
    {
      return result;
    }

    sb->end = offset + length;
  }

  memcpy(&sb->buffer[offset], bytes, length);

  return LUABINS_ESUCCESS;
}

/*
* Returns a pointer to the internal buffer with data.
* Note that buffer is NOT zero-terminated.
* Buffer is valid until next operation with the given sb.
*/
const unsigned char * lbsSB_buffer(luabins_SaveBuffer * sb, size_t * length)
{
  if (length != NULL)
  {
    *length = sb->end;
  }
  return sb->buffer;
}

void lbsSB_destroy(luabins_SaveBuffer * sb)
{
  if (sb->buffer != NULL)
  {
    /* Ignoring errors */
    SPAM(("dealloc size %lu\n", sb->buf_size));
    sb->alloc_fn(sb->alloc_ud, sb->buffer, sb->buf_size, 0UL);
    sb->buffer = NULL;
    sb->buf_size = 0UL;
    sb->end = 0UL;
  }
}
