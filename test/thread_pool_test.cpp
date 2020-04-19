#include <vector>

#include "thread_pool.h"
#include "unittest.h"


int count = 0;
std::mutex mutex;

void* increment(int delta) {
	for (int i = 0; i < delta; ++i) {
		std::lock_guard<std::mutex> lock_guard(mutex);
		++count;
		//std::cout << count << std::endl;
	}

}

void test_counter() {
	int ntasks = 10;
	int delta = 1000;
	
	ThreadPool pool(4);
	std::vector<Future<void*>> futures;
	for (int i = 0; i < ntasks; ++i) {
		Future<void*> future = pool.async_apply(increment, delta);
		futures.push_back(future);
	}
	for (auto&& future: futures) {
		future.get(NULL);
	}	
	pool.close();
	pool.join();

	ASSERT_EQUAL(ntasks * delta, count);
}

int sum(const std::vector<int>& nums, int begin, int end) {
	int result = 0;
	for (int i = begin; i < end; i++) {
		result += nums[i];
	}
	return result;
}

void test_sum() {
	int ntasks = 10;
	std::vector<int> nums(10000);
	int expected = 0;
	for (int i = 0; i < nums.size(); i++) {
		nums[i] = i;
		expected += nums[i];
	}
	
	ThreadPool pool(4);
	std::vector<Future<int>> futures;
	for (int i = 0; i < ntasks; ++i) {
		int begin = i * nums.size() / ntasks;
		int end = (i + 1) * nums.size() / ntasks;
		Future<int> future = pool.async_apply(
				sum, nums, begin, end);
		futures.push_back(future);
	}

	int actrual = 0;
	for (auto&& future: futures) {
		int value;
		future.get(&value);
		actrual += value;
	}	
	pool.close();
	pool.join();

	ASSERT_EQUAL(expected, actrual);
}

int main() {
	RUN_TEST("concurrent increment", test_counter);
	RUN_TEST("summation of a vector", test_sum);
	return 0;
}

