#include <ctype.h>   // Para toupper
#include <string.h>  // Para strdup
#include "CuTest.h"

// Mal implementado a propósito para que falle
char* StrToUpper(char* str) {
    // descomentar la siguiente línea para que pase el test
    // for (char* p = str ; *p ; ++p) *p = toupper(*p);
    return str;
}

void TestStrToUpper(CuTest *tc) {
    char* input = strdup("hello world");
    char* actual = StrToUpper(input);
    char* expected = "HELLO WORLD";
    CuAssertStrEquals(tc, expected, actual);
}

CuSuite* testThatFailsGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestStrToUpper);
    return suite;
}