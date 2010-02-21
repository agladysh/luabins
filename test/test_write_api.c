/*
* test_write_api.c
* Luabins Lua-less write API tests
* See copyright notice in luabins.h
*/

/*
* WARNING: This suite is format-specific. Change it when format changes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Should be included first */
#include "lualess.h"
#include "write.h"

#include "test.h"

/******************************************************************************/

static void fprintbuf(FILE * out, const unsigned char * b, size_t len)
{
  size_t i = 0;
  for (i = 0; i < len; ++i)
  {
    fprintf(out, "%02X ", b[i]);
  }
  fprintf(out, "\n");
}

/*
* Note it is different from test_savebuffer variant.
* We're interested in higher level stuff here.
*/
static void check_buffer(
    luabins_SaveBuffer * sb,
    const char * expected_buf_c,
    size_t expected_length
  )
{
  const unsigned char * expected_buf = (const unsigned char *)expected_buf_c;

  size_t actual_length = (size_t)-1;
  const unsigned char * actual_buf = lbsSB_buffer(sb, &actual_length);
  if (actual_length != expected_length)
  {
    fprintf(
        stderr,
        "lsbSB_buffer length mismatch: got %lu, expected %lu\n",
        actual_length, expected_length
      );
    fprintf(stderr, "actual:\n");
    fprintbuf(stderr, actual_buf, actual_length);
    fprintf(stderr, "expected:\n");
    fprintbuf(stderr, expected_buf, expected_length);
    exit(1);
  }

  if (memcmp(actual_buf, expected_buf, expected_length) != 0)
  {
    fprintf(stderr, "lsbSB_buffer buffer mismatch\n");
    fprintf(stderr, "actual:\n");
    fprintbuf(stderr, actual_buf, actual_length);
    fprintf(stderr, "expected:\n");
    fprintbuf(stderr, expected_buf, expected_length);

    exit(1);
  }
}

/******************************************************************************/

TEST (test_writeTupleSize,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    unsigned char tuple_size = 0xAB;

    lbs_writeTupleSize(&sb, tuple_size);
    check_buffer(&sb, "\xAB", 1);
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeTableHeader,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    int array_size = 0xAB;
    int hash_size = 0xCD;

    lbs_writeTableHeader(&sb, array_size, hash_size);
    check_buffer(&sb, "T" "\xAB\x00\x00\x00" "\xCD\x00\x00\x00", 1 + 4 + 4);
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeTableHeaderAt,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    unsigned char tuple_size = 0x01;
    int array_size = 0x00;
    int hash_size = 0x00;
    int table_header_pos = 0;

    lbs_writeTupleSize(&sb, tuple_size);
    table_header_pos = lbsSB_length(&sb);
    lbs_writeTableHeader(&sb, array_size, hash_size);

    check_buffer(
        &sb,
        "\x01" "T" "\x00\x00\x00\x00" "\x00\x00\x00\x00",
        1 + 1 + 4 + 4
      );

    array_size = 0xAB;
    hash_size = 0xCD;

    lbs_writeTableHeaderAt(&sb, table_header_pos, array_size, hash_size);
    check_buffer(
        &sb,
        "\x01" "T" "\xAB\x00\x00\x00" "\xCD\x00\x00\x00",
        1 + 1 + 4 + 4
      );
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeNil,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    lbs_writeNil(&sb);
    check_buffer(&sb, "-", 1);
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeBoolean,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    lbs_writeBoolean(&sb, 1);
    check_buffer(&sb, "1", 1);

    lbs_writeBoolean(&sb, 0);
    check_buffer(&sb, "10", 1 + 1);
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeNumber,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    /* Note number is a double */
    lbs_writeNumber(&sb, 1.0);
    check_buffer(&sb, "N" "\x00\x00\x00\x00\x00\x00\xF0\x3F", 1 + 8);
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeInteger,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    /* Note integer is alsow written as a double */
    lbs_writeInteger(&sb, 1);
    check_buffer(&sb, "N" "\x00\x00\x00\x00\x00\x00\xF0\x3F", 1 + 8);
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

TEST (test_writeStringEmpty,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    lbs_writeString(&sb, "", 0);
    check_buffer(&sb, "S" "\x00\x00\x00\x00\x00\x00\x00\x00", 1 + 8);
  }

  lbsSB_destroy(&sb);
})

TEST (test_writeStringSimple,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    lbs_writeString(&sb, "Luabins", 7);
    check_buffer(
        &sb,
        "S" "\x07\x00\x00\x00\x00\x00\x00\x00" "Luabins",
        1 + 8 + 7
      );
  }

  lbsSB_destroy(&sb);
})

TEST (test_writeStringEmbeddedZero,
{
  luabins_SaveBuffer sb;
  lbsSB_init(&sb, lbs_simplealloc, NULL);

  {
    lbs_writeString(&sb, "Embedded\0Zero", 13);
    check_buffer(
        &sb,
        "S" "\x0D\x00\x00\x00\x00\x00\x00\x00" "Embedded\0Zero",
        1 + 8 + 13
      );
  }

  lbsSB_destroy(&sb);
})

/******************************************************************************/

void test_write_api()
{
  test_writeTupleSize();

  test_writeTableHeader();
  test_writeTableHeaderAt();

  test_writeNil();

  test_writeBoolean();

  test_writeNumber();

  test_writeInteger();

  test_writeStringEmpty();
  test_writeStringSimple();
  test_writeStringEmbeddedZero();
}
