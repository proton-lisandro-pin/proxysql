#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "proxysql_gtid.h"


// Initializes a GTID interval.
// Implemented via private method as C++03 does not support delegated constructors :(
void Gtid_Interval::init(const int64_t _start, const int64_t _end) {
	start = _start;
	end = _end;

	if (start > end) {
		std::swap(start, end);
	}
}

// Initializes a GTID interval from a string buffer, in [gtid]{-[gtid]} format.
void Gtid_Interval::init(const char *s) {
	uint64_t _start, _end;

	if (sscanf(s, "%lu-%lu", &_start, &_end) == 2) {
		init((int64_t)_start, (int64_t)_end);
		return;
	}
	if (sscanf(s, "%lu", &_start) == 1) {
		init((int64_t)_start, (int64_t)_start);
		return;
	}

	init(0, 0);
}


Gtid_Interval::Gtid_Interval(const int64_t _start, const int64_t _end) {
	init(_start, _end);
}

Gtid_Interval::Gtid_Interval(const char *s) {
	init(s);
}

Gtid_Interval::Gtid_Interval(const std::string& s) {
	init(s.c_str());
}

// Checks if a given GTID is contained in this interval.
const bool Gtid_Interval::contains(int64_t gtid) {
	return (gtid >= start && gtid <= end);
}

// Yields a string representation for a GTID interval.
const std::string Gtid_Interval::to_string(void) {
	if (start == end) {
		return std::to_string(start);
	}
	return std::to_string(start) + "-" + std::to_string(end);
}

// Attempts to merge two GTID intervals. Returns true if the intervals were merged (and potentially modified), false otherwise.
const bool Gtid_Interval::merge(const Gtid_Interval& other) {
	if (other.start >= start && other.end <= end) {
		// other is contained by interval
		return true;
	}
	if (other.start <= start && other.end >= end) {
		// other contains whole of existing interval
		start = other.start;
		end = other.end;
		return true;
	}
	if (other.start <= start && other.end >= (start-1)) {
		// other overlaps interval at start
		start = other.start;
		return true;
	}
	if (other.end >= end && other.start <= (end+1)) {
		// other overlaps interval at end
		end = other.end;
		return true;
	}

	return false;
}

// Comapres two GTID intervals, by strict weak ordering.
const int Gtid_Interval::cmp(const Gtid_Interval& other) {
	if (start < other.start) {
		return -1;
	}
	if (start > other.start) {
		return 1;
	}
	if (end < other.end) {
		return -1;
	}
	if (end > other.end) {
		return 1;
	}
	return 0;
}

const bool Gtid_Interval::operator<(const Gtid_Interval& other) {
	return cmp(other) == -1;
}

const bool Gtid_Interval::operator==(const Gtid_Interval& other) {
	return cmp(other) == 0;
}
