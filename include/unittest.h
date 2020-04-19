#ifndef UNITTEST_H_
#define UNITTEST_H_

#include <iostream>  // for std::cout, std::endl 

#include <stdlib.h>  // for exit, EXIT_FAILURE


#define RUN_TEST(msg, test_case) { \
	std::cout << "Test: " << msg << " ..." << std::endl; \
	test_case(); \
	std::cout << " ... Passed" << std::endl; \
}

#define ASSERT(exp) { \
	if (!(exp)) { \
		std::cout << " ... FAIL: " << __FILE__ << ":" \
			<< __LINE__ << ", assertion: " \
			<< #exp << std::endl; \
		exit(EXIT_FAILURE); \
	} \
}

#define ASSERT_EQUAL(expected, actual) { \
	decltype(expected) e = (expected); \
	decltype(actual) a = (actual); \
	if (!(e == a)) { \
		std::cout << " ... FAIL: " << __FILE__ << ":" \
			<< __LINE__ << ", expected: " \
			<< e << ", but got: " << a << std::endl; \
		exit(EXIT_FAILURE); \
	} \
}

#endif  // UNITTEST_H_

