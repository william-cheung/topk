
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <utility>
#include <sstream>

#include "thread_pool.h"

using namespace std;


#define MiB 1024*1024*1024
#define GiB 1024*MiB

#define NSHARD_SIZE 100


class TopKSolver {
private:
	struct Comp {
		bool operator()(
				const pair<string, int>& p1,
		       		const pair<string, int>& p2) {
			return p1.second > p2.second || 
			(p1.second == p2.second && p1.first < p2.first);
		}
	};

	typedef priority_queue<
		pair<string, int>, 
		vector<pair<string, int>>, Comp> PQ;

public:
	TopKSolver(int k): _k(k) {}

	void add(string value, int freq) {
		add(make_pair(value, freq));
	}

	void add(const pair<string, int>& p) {
		_pq.push(p);
		if (_pq.size() > _k) {
			_pq.pop();
		}
	}

	vector<pair<string, int>> get_result() {
		vector<pair<string, int>> result;
		while (!_pq.empty()) {
			result.push_back(_pq.top());
			_pq.pop();
		}
		reverse(result.begin(), result.end());
		return result;
	}

private:
	PQ _pq;
	int _k;
};



template <class Container>
void split(const string& s, char delim, Container& c) {
	stringstream ss(s);
	string token;
	while (getline(ss, token, delim))
		c.push_back(token);
}

vector<pair<string, int>> topk(string filename, int k) {
	fstream fs(filename, fstream::in);
	unordered_map<string, int> count;
	string line;
	while (getline(fs, line)) {
		vector<string> parts;
		split(line, ' ', parts);
		int n = parts.size();
		if (n == 2) {
			count[parts[0]] += stoi(parts[2]);
		} else if (n == 1) {
			++count[parts[0]];
		}
	}
	fs.close();

	TopKSolver tks(k);
	for (auto p: count) {
		tks.add(p);
	}
	return tks.get_result();
}


struct Shard {
	string filename;
	unordered_map<string, int> buffer;

	Shard(const string& filename): filename(filename) {}
};


int partition(const string& filename, int nshards, vector<Shard>* shards) {
	if (shards == NULL || nshards < 1)
		return -1;

	fstream fs(filename, fstream::in);
	if (!fs.is_open())
		return -1;


	vector<fstream> ss;

	for (int i = 0; i < nshards; i++) {
		string&& name = string("data/shard-") + to_string(i);
		shards->emplace_back(name);
		ss.emplace_back(name, fstream::out);

	}

	string line;
	int ret = 0;
	for (int i = 0; i < nshards; i++) {
		if (!ss[i].is_open()) {
			ret = -1;
			goto clean;
		}
	}

	while (getline(fs, line)) {
		size_t h = hash<string>{}(line);
		cout << line << ' ' << h << endl;
		if (!(ss[h%nshards] << line << "\n")) {
			ret = -1;
			goto clean;
		}
	}

	//if (fs.fail())
	//	ret = -1;

clean:
	for (int i = 0; i < nshards; i++) {
		if (ss[i].is_open())
			ss[i].close();
	}
	fs.close();
	return ret;
}


int main() {
	vector<Shard> shards;
	int nshards = 3;
	int ret = partition("data/test.txt", nshards, &shards);
	if (ret != 0) {
		// error handling
		cout << "error partitioning" << endl;
		return 0;
	}


	int k = 2;
	ThreadPool thread_pool(4);
	vector<Future<vector<pair<string, int>>>> futures;
	for (int i = 0; i < nshards; i++) {
		futures.push_back(thread_pool.async_apply(topk, shards[i].filename, k));
	}

	TopKSolver tks(k);
	for (int i = 0; i < nshards; i++) {
		vector<pair<string, int>> result;
		if (futures[i].get(&result)) {
			for (auto& p: result)
				tks.add(p);
		}
	}
	vector<pair<string, int>> result = tks.get_result();
	for (auto p : result) {
		cout << p.first << " " << p.second << endl;
	}
	thread_pool.close();
	thread_pool.join();

	return 0;
}

