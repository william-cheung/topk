#include <vector>

#include "util.h"
#include "unittest.h"


void test_split_space() {
	std::vector<std::string> parts;

	std::string s1 = "";
	split(s1, parts);
	ASSERT_EQUAL(0, parts.size());

	std::string s2 = "a ";
	split(s2, parts);
	ASSERT_EQUAL(1, parts.size())
	ASSERT_EQUAL("a", parts[0]);

	std::string s3 = "  a";
	parts.clear();
	split(s3, parts);
	ASSERT_EQUAL(3, parts.size());
	ASSERT_EQUAL("", parts[0]);
	ASSERT_EQUAL("", parts[1])
	ASSERT_EQUAL("a", parts[2]);

	std::string s4 = "ab c d";
	parts.clear();
	split(s4, parts);
	ASSERT_EQUAL(3, parts.size());
	ASSERT_EQUAL("ab", parts[0]);
	ASSERT_EQUAL("c", parts[1])
	ASSERT_EQUAL("d", parts[2]);
}

void test_split_dollar() {
	std::vector<std::string> parts;
	std::string s("$1$ 2$$3  $");
	split(s, parts, '$');
	ASSERT_EQUAL(5, parts.size());
	ASSERT_EQUAL("", parts[0]);
	ASSERT_EQUAL("1", parts[1])
	ASSERT_EQUAL(" 2", parts[2]);
	ASSERT_EQUAL("", parts[3]);
	ASSERT_EQUAL("3  ", parts[4]);
}


void test_format() {
	ASSERT_EQUAL("abc123def3.45", format(
		"a%s1%dde%c%.2lf", "bc", 23, 'f', 3.451));
}


int main() {
	RUN_TEST("split string with space", test_split_space);
	RUN_TEST("split string with $", test_split_dollar);
	RUN_TEST("format", test_format);
	return 0;
}
