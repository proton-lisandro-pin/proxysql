#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "proxysql_gtid.h"

// Initializes a UUID:GTID pair.
Uuid_Gtid::Uuid_Gtid(const std::string _uuid, const gtid_t _gtid) : uuid(_uuid), gtid(_gtid) {
}

// Initializes a GTID interval.
// Implemented via private method as C++03 does not support delegated constructors :(
void Gtid_Interval::init(const gtid_t _start, const gtid_t _end) {
	start = _start;
	end = _end;
}

// Initializes a GTID interval from a string buffer, in [gtid]{-[gtid]} format.
void Gtid_Interval::init(const char *s) {
	uint64_t _start, _end;

	if (sscanf(s, "%lu-%lu", &_start, &_end) == 2) {
		init((gtid_t)_start, (gtid_t)_end);
		return;
	}
	if (sscanf(s, "%lu", &_start) == 1) {
		init((gtid_t)_start, (gtid_t)_start);
		return;
	}

	init(0, 0);
}

Gtid_Interval::Gtid_Interval(const gtid_t gtid) {
	init(gtid, gtid);
}

Gtid_Interval::Gtid_Interval(const gtid_t _start, const gtid_t _end) {
	init(_start, _end);
}

Gtid_Interval::Gtid_Interval(const char *s) {
	init(s);
}

Gtid_Interval::Gtid_Interval(const std::string& s) {
	init(s.c_str());
}

// Checks if a given GTID is contained in this interval.
const bool Gtid_Interval::contains(gtid_t gtid) {
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

// Initializes a GTID interval set.
Gtid_Set::Gtid_Set() {}

// Adds a new GTID interval for a given UUID.
void Gtid_Set::add(const std::string& uuid, const gtid_interval_t& iv) {
	auto it = map.find(uuid);
	if (it == map.end()) {
		// new UUID entry
		map[uuid].emplace_back(iv);
		return;
	}

	// insert/merge GTID interval
	auto pos = it->second.begin();
	for (; pos != it->second.end(); ++pos) {
		if (pos->merge(iv))
			break;
	}
	if (pos == it->second.end()) {
		it->second.emplace_back(iv);
	}

	// merge overlapping GTID ranges, if any
	it->second.sort();
	auto a = it->second.begin();
	while (a != it->second.end()) {
		auto b = std::next(a);
		if (b == it->second.end()) {
			break;
		}
		if (a->merge(*b)) {
				it->second.erase(b);
				continue;
		}
		a++;
	}
}

// Adds a single GTID for a given UUID.
void Gtid_Set::add(const std::string& uuid, const gtid_t& gtid) {
    add(uuid, gtid_interval_t(gtid));
}

// Adds a new GTID range for a given UUID.
void Gtid_Set::add(const std::string& uuid, const gtid_t& start, const gtid_t& end) {
    add(uuid, gtid_interval_t(start, end));
}

// Adds a new GTID range for a given UUID, as a C string buffer.
void Gtid_Set::add(const std::string& uuid, const char *str) {
    add(uuid, gtid_interval_t(str));
}

// Evaluates whether a GTID is present in any of the intervals for a given UUID.
const bool Gtid_Set::has_gtid(const std::string uuid, const gtid_t gtid) {
	auto it = map.find(uuid);
	// fprintf(stderr,"Checking if server %s:%d has GTID %s:%lu ... ", address, port, gtid_uuid, gtid_trxid);
	if (it == map.end()) {
		// fprintf(stderr,"NO\n");
		return false;
	}
	for (auto itr = it->second.begin(); itr != it->second.end(); ++itr) {
		if (itr->contains(gtid)) {
			// fprintf(stderr,"YES\n");
			return true;
		}
	}
	// fprintf(stderr,"NO\n");
	return false;
}

// Yields a string representation for a GTID interval set.
const std::string Gtid_Set::to_string(void) {
	std::string out;
	for (auto it=map.begin(); it!=map.end(); ++it) {
		std::string s = it->first;
		s.insert(8,"-");
		s.insert(13,"-");
		s.insert(18,"-");
		s.insert(23,"-");
		for (auto itr = it->second.begin(); itr != it->second.end(); ++itr) {
			out += s + ":" + itr->to_string() + ",";
		}
	}
	// Extract latest comma only in case the output string isn't empty
	if (!out.empty()) {
		out.pop_back();
	}
	return out;
}