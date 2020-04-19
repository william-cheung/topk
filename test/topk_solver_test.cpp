#include "topk_solver.h"
#include "unittest.h"

void test_topk_basic() {
	TopKSolver tks(2);
	tks.add(std::make_pair("a", 1));
	tks.add(std::make_pair("b", 2));
	tks.add(std::make_pair("c", 3));
	auto topk = tks.get_result();
	ASSERT_EQUAL(2, topk.size());
	ASSERT_EQUAL("c", topk[0].first);
	ASSERT_EQUAL(3, topk[0].second);
	ASSERT_EQUAL("b", topk[1].first);
	ASSERT_EQUAL(2, topk[1].second);
}

int main() {
	RUN_TEST("top k basic", test_topk_basic);
	return 0;
}
