/**
 * @file public.c
 * @brief Public test cases (and location for new tests)
 * 
 * This file provides a few basic sanity test cases and a location to add new tests.
 */

#include "testsuite.h"

#ifndef SKIP_IN_DOXYGEN

TEST_PROGRAM(D_sanity_zero, 0, "def int main() { return 0; }")

TEST_EXPRESSION(D_expr_int,   7, "7")
TEST_EXPRESSION(D_expr_add,   5, "2+3")
TEST_EXPRESSION(D_expr_long, 10, "1+2+3+4")

TEST_EXPRESSION(D_expr_sanity_zero, 0, "0")

TEST_EXPRESSION(C_expr_negate, -4, "-4")

TEST_MAIN(C_assignment, 14, "int a; a = 2 + 3 * 4; return a;")
TEST_PROGRAM(C_global_assignment, 14, "int a; def int main() { a = 2 + 3 * 4; return a; }")

TEST_BOOL_EXPRESSION(B_expr_not_t, 0, "!true")

TEST_MAIN(B_conditional, 2,
        "  int r; "
        "  if (true) { r = 2; } "
        "  else { r = 3; } "
        "  return r;")

TEST_MAIN(B_whileloop, 10,
        "  int a; a = 0; "
        "  while (a < 10) { a = a + 1; } "
        "  return a;")

TEST_PROGRAM(B_funccall, 20,
        "def int ten() { return 10; } "
        "def int main() { return ten() + ten(); }")

TEST_PROGRAM(A_funccall_params, 5,
        "def int add(int a, int b) { return a + b; } "
        "def int main() { return add(2,3); }")

TEST_MAIN(Nested_whileloops, 6,
        "int a; int b; int c; a = 0; c = 0;"
        "while (a < 2) { b = 0; while (b < 3) { c = c + 1; b = b + 1; } a = a + 1; }"
        "return c;")

TEST_PROGRAM(Location_assigned_element, 4,
        "int a[1];"
        "def int main() { int b; a[0] = 4; b = a[0]; return b; }")

TEST_PROGRAM(Element_assigned_location, 2,
        "int a[1];"
        "def int main() { int b; b = 2; a[0] = b; return a[0]; }")

TEST_PROGRAM(Add_two_array_elements, 5,
        "int a[2];"
        "def int main() { a[0] = 2; a[1] = 3; return a[0] + a[1]; }")

TEST_MAIN(Nested_conditionals, 3,
        "int a; int b; int c;"
        "a = 2; b = 3; c = 1;"
        "if (a < 3) { if (b < 5) { c = 3; } else { c = 2; } }"
        "return c;")

TEST_PROGRAM(Funccall_array_element, 5,
        "def int add(int a, int b) { return a + b; }"
        "int a[1];"
        "def int main() { int c; c = 2; a[0] = 3; return add(a[0], c); }")

TEST_PROGRAM(Unaryop_array_element, -3,
        "int a[1];"
        "def int main() { a[0] = 3; return -a[0]; }")

#endif

/**
 * @brief Register all test cases
 * 
 * @param s Test suite to which the tests should be added
 */
void public_tests (Suite *s)
{
    TCase *tc = tcase_create ("Public");

    TEST(D_sanity_zero);
    TEST(D_expr_int);
    TEST(D_expr_add);
    TEST(D_expr_long);
    TEST(D_expr_sanity_zero);

    TEST(C_expr_negate);
    TEST(C_assignment);
    TEST(C_global_assignment);

    TEST(B_expr_not_t);
    TEST(B_conditional);
    TEST(B_whileloop);
    TEST(B_funccall);

    TEST(A_funccall_params);

    /* Custom tests */
    TEST(Nested_whileloops);
    TEST(Location_assigned_element);
    TEST(Element_assigned_location);
    TEST(Add_two_array_elements);
    TEST(Nested_conditionals);
    TEST(Funccall_array_element);
    TEST(Unaryop_array_element);

    suite_add_tcase (s, tc);
}

