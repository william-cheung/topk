#ifndef UTILITY_H_
#define UTILITY_H_


#include <sstream>  // for std::string, std::stringstream
#include <chrono>   // for std::chrono
#include <iostream> // for std::cout

#include <stdio.h>  // for vsnprintf
#include <stdarg.h> // for va_list, va_begin, ...


inline void cxx_printf(const char *format, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	std::cout << buf;
}


inline std::string format(const char *format, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	return std::string(buf);
}


// Splits a string into multiple parts with a given delimiter.
template <class Container>
inline void split(const std::string& s, Container& c, char delim=' ') {
	std::stringstream ss(s);
	std::string token;
	while (std::getline(ss, token, delim))
		c.push_back(token);
}


class Timer {
	typedef std::chrono::time_point<
		std::chrono::steady_clock> time_point_t;
	time_point_t start_;
public:
	Timer(): start_(now()) {}

	void reset() {
		start_ = now();
	}

	double elapsed_seconds() {
		auto end = now();
		std::chrono::duration<double> diff = end - start_;
		return diff.count();
	}

private:
	time_point_t now() {
		return std::chrono::steady_clock::now();
	}
};


#endif  // UTILITY_H_

