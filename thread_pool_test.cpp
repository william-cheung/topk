#include <iostream>
#include <assert.h>

#include "thread_pool.h"


int count = 0;
std::mutex mutex;


void* increment(int delta) {
	for (int i = 0; i < delta; ++i) {
		std::lock_guard<std::mutex> lock_guard(mutex);
		++count;
		std::cout << count << std::endl;
	}

}

int main() {
	ThreadPool pool(4);

	std::vector<Future<void*>> futures;
	for (int i = 0; i < 10; ++i) {
		Future<void*> future = pool.async_apply(increment, 1000);
		futures.push_back(future);
	}
	for (auto&& future: futures) {
		future.get(NULL);
	}

	assert(count == 10000);	

	pool.close();
	pool.join();
	return 0;
}



