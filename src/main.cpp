
#include <fstream>       // for std::fstream
#include <vector>        // for std::vector
#include <unordered_map> // for std::unordered_map

#include <unistd.h>      // for getopt

#include "topk_solver.h" // for TopKSolver
#include "thread_pool.h" // for ThreadPool, Future
#include "util.h"        // for cxx_printf, format, Timer

using namespace std;


// Find top k frequent elements in a file.
vector<pair<string, int>> topk(const string& filename, int k) {
	fstream fs(filename, fstream::in);
	if (!fs.is_open()) {
		throw logic_error(format("cannot open file '%s'",
					filename.c_str()));
	}
	
	unordered_map<string, int> count;
	string line;
	while (getline(fs, line)) {
		++count[line];
	}

	TopKSolver tks(k);
	for (auto& p: count) {
		tks.add(p);
	}
	return tks.get_result();
}

inline string shardname(int i) {
	return string("/tmp/shard-").append(to_string(i));
}

inline string shardname(const string& input_file, int nshards, int i) {
	if (nshards == 1)
		return input_file;
	return shardname(i);
}

// Partition the input file into a number of shards using hashing.
int partition(const string& filename, int nshards) {
	if (nshards < 2) {
		cxx_printf("ERROR: invalid nshards: %d\n", nshards);
		return -1;
	}

	fstream fs(filename, fstream::in);
	if (!fs.is_open()) {
		cxx_printf("ERROR: cannot open file '%s'\n",
				filename.c_str());
		return -1;
	}

	vector<fstream> shards;
	for (int i = 0; i < nshards; i++) {
		shards.emplace_back(shardname(i), fstream::out);
	}

	int ret = 0;
	for (int i = 0; i < nshards; i++) {
		if (!shards[i].is_open()) {
			cxx_printf("ERROR: cannot open file '%s' to write\n",
					shardname(i).c_str());
			return -1;
		}
	}

	string line;
	while (getline(fs, line)) {
		size_t h = hash<string>{}(line);
		int i = h % nshards;
		if (!(shards[i] << line << endl)) {
			cxx_printf("ERROR: error occurred when writing"
					" data to file '%s'",
					shardname(i).c_str());
			return -1;
		}
	}
}


void print_usage(const string& program_name) {
	cxx_printf("Usage: %s FILE [OPTION]...\n", program_name.c_str());
	cxx_printf("Find top 100 frequent elements in FILE, assuming that elements are\n");
	cxx_printf("  displayed one per line in FILE.\n");
	cxx_printf("\n");
	cxx_printf("Options:\n");
	cxx_printf("  -k K         find top K frequent elements in FILE, instead of top\n");
	cxx_printf("                 100\n");
	cxx_printf("  -s NSHARDS   partition FILE into NSHARDS shards; the default is 1\n");
	cxx_printf("  -t NTHREADS  number of worker threads to execute tasks; the goal of\n");
	cxx_printf("                 each task is to find top K elments in one shard; the\n");
	cxx_printf("                 default is 1\n");
	cxx_printf("  -h           display this help and exit\n");
}


struct Flags {
	string file;
	int k;
	int nshards;
	int nthreads;
} flags;


int parse_flags(int argc, char* argv[]) {
	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	flags.file = string(argv[1]);
	flags.nshards = 1;
	flags.k = 100;
	flags.nthreads = 1;

	// TODO validate flag values
	int opt;
	while ((opt = getopt(argc, argv, "k:s:t:h")) != -1) {
		switch(opt) {
			case 'k':
				flags.k = stoi(optarg);
				break;
			case 's':
				flags.nshards = stoi(optarg);
				break;
			case 't':
				flags.nthreads = stoi(optarg);
				break;
			case 'h':
				print_usage(argv[0]);
				return -1;
		}
	}
	return 0;
}


int main(int argc, char *argv[]) {
	if (parse_flags(argc, argv) < 0) {
		return 0;
	}

	Timer timer;
	if (flags.nshards > 1) {
		cxx_printf("Start partitioning ... nshards: %d\n",
				flags.nshards);

		int ret = partition(flags.file, flags.nshards);
		
		cxx_printf(" ... Done (%.2lfs)\n",
				timer.elapsed_seconds());
		
		if (ret < 0)
			return 0;
	}

	timer.reset();
	cxx_printf("Find top %d in %d shard(s) respectively using %d"
			" thread(s), then merge the results ...\n", 
			flags.k, flags.nshards, flags.nthreads);

	ThreadPool thread_pool(flags.nthreads);
	vector<Future<vector<pair<string, int>>>> futures;
	for (int i = 0; i < flags.nshards; i++) {
		string name(shardname(flags.file, flags.nshards, i));
		futures.push_back(thread_pool.async_apply(
					topk, name, flags.k));
	}
	TopKSolver tks(flags.k);
	for (int i = 0; i < flags.nshards; i++) {
		vector<pair<string, int>> result;
		if (futures[i].get(&result)) {
			for (auto& p: result)
				tks.add(p);
		} else {
			string name = shardname(
					flags.file, flags.nshards, i);
			cxx_printf("ERROR: something went wrong when"
					" handling shard '%s': %s\n",
					name.c_str(), 
					futures[i].error().c_str());
		}
	}
	thread_pool.close();
	thread_pool.join();

	cxx_printf(" ... Done (%.2lfs)\n", timer.elapsed_seconds());

	vector<pair<string, int>> result = tks.get_result();
	if (result.size() > 0) {
		cxx_printf("Top %d elements in file '%s':\n", 
				flags.k, flags.file.c_str());
	}
	for (auto& p : result) {
		cxx_printf("  %4d  %s\n", p.second, p.first.c_str());	
	}

	return 0;
}

