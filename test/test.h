/*
* test.h
* Luabins test basics
* See copyright notice in luabins.h
*/

#ifndef LUABINS_TEST_H_
#define LUABINS_TEST_H_

#define TEST(name, body) \
  static void name() \
  { \
    printf("---> BEGIN %s\n", #name); \
    body \
    printf("---> OK\n"); \
  }

void test_savebuffer();
void test_write_api();
void test_api();

#endif /* LUABINS_TEST_H_ */
