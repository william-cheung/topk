#include <unordered_map>
#include <string>

#include <cstdlib>
#include <ctime>

#include "topk_solver.h"
#include "util.h"

using namespace std;

const char alpha[] = "abcdef";
int nalpha = sizeof(alpha) - 1;

inline string rand_string() {
	string result("http://www.");
	int size = 1 + rand() % 100;
	for (int i = 0; i < size; i++)
		result.append(1, alpha[rand()%nalpha]);
	return result.append(".com");
}

int main() {
	srand(time(0));

	int n = 1000000;
	int k = 100;

	Timer timer;
	unordered_map<string, int> count;
	for (int i = 0; i < n; i++) {
		++count[rand_string()];
	}

	cxx_printf("populate count map with %d rand"
			" strings in %.2lfs\n",
			n, timer.elapsed_seconds());
	timer.reset();


	TopKSolver tks(k);
	for (auto& p: count) {
		tks.add(p);
	}
	auto result = tks.get_result();
	for (auto& p: result) {
		cxx_printf("%4d %s\n", p.second, p.first.c_str());
	}
		
	cxx_printf("got top %d of %d elements in %.2lfs\n",
			k, count.size(), 
			timer.elapsed_seconds());

	return 0;
}

