#include <unistd.h>
#include <cstddef>
#ifndef __AFL_FUZZ_TESTCASE_LEN
ptrdiff_t fuzz_len;
#define AFL_FUZZ_TESTCASE_LEN fuzz_len
unsigned char fuzz_buf[1024000];
#define AFL_FUZZ_TESTCASE_BUF fuzz_buf
#define AFL_FUZZ_INIT() void sync(void)
#define AFL_LOOP(x) ((fuzz_len = read(0, fuzz_buf, sizeof(fuzz_buf))) > 0 ? 1 : 0)
#define AFL_INIT() sync()
#else
#define AFL_FUZZ_TESTCASE_LEN __AFL_FUZZ_TESTCASE_LEN
#define AFL_FUZZ_TESTCASE_BUF __AFL_FUZZ_TESTCASE_BUF
#define AFL_FUZZ_INIT() __AFL_FUZZ_INIT()
#define AFL_LOOP(x) __AFL_LOOP(x)
#define AFL_INIT() __AFL_INIT()
#endif