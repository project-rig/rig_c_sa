/**
 * Shared header file for test suite.
 */
#ifndef TESTS_H
#define TESTS_H

#include <check.h>

Suite *make_sa_state_suite(void);
Suite *make_sa_manipulation_suite(void);
Suite *make_sa_algorithm_suite(void);

#endif
