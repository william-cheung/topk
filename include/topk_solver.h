#ifndef TOPK_SOLVER_H_
#define TOPK_SOLVER_H_

#include <string>     // for std::string
#include <queue>      // for std::priority_queue
#include <vector>     // for std::vector
#include <algorithm>  // for std::reverse


// TopKSolver: used to find top k frequent elements given frequency
// of each element in O(n*logk) time, where n is the total number of
// elements.
//

class TopKSolver {
private:
	struct Comp {
		bool operator()(
				const std::pair<std::string, int>& p1,
		       		const std::pair<std::string, int>& p2) {
			return p1.second > p2.second || 
			(p1.second == p2.second && p1.first < p2.first);
		}
	};

	typedef std::priority_queue<
		std::pair<std::string, int>, 
		std::vector<std::pair<std::string, int>>, Comp> PQ;

private:
	PQ pq_;
	int k_;

public:
	TopKSolver(int k): k_(k) {}

	// Add an element to the solver. Each element is represented
	// by a pair of its value and its freqency.
	void add(const std::pair<std::string, int>& p) {
		pq_.push(p);
		if (pq_.size() > k_) {
			pq_.pop();
		}
	}

	// Get top k frequent elements. Elements are returned in 
	// descending order of frequency.
	std::vector<std::pair<std::string, int>> get_result() {
		std::vector<std::pair<std::string, int>> result;
		while (!pq_.empty()) {
			result.push_back(pq_.top());
			pq_.pop();
		}
		std::reverse(result.begin(), result.end());
		return result;
	}
};

#endif  // TOPK_SOLVER_H_
