#ifndef PROXYSQL_GTID
#define PROXYSQL_GTID
// highly inspired by libslave
// https://github.com/vozbu/libslave/
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

typedef int64_t gtid_t;

// Encapsulates a UUID:GTID pair.
class Uuid_Gtid {
	public:
		std::string uuid;
		gtid_t gtid;

	public:
		Uuid_Gtid(const std::string _uuid, const gtid_t _gtid);
};
typedef Uuid_Gtid ugtid_t;

// Encapuslates an interval of GTIDs.
class Gtid_Interval {
	public:
		gtid_t start;
		gtid_t end;

	private:
		void init(const gtid_t _start, const gtid_t _end);
		void init(const char* s);

	public:
		explicit Gtid_Interval(const gtid_t gtid);
		explicit Gtid_Interval(const gtid_t _start, const gtid_t _end);
		explicit Gtid_Interval(const char* s);
		explicit Gtid_Interval(const std::string& s);

		const std::string to_string(void);
		const bool contains(gtid_t gtid);
		const bool merge(const Gtid_Interval& other);

		const int cmp(const Gtid_Interval& other);
		const bool operator<(const Gtid_Interval& other);
		const bool operator==(const Gtid_Interval& other);
};
typedef Gtid_Interval gtid_interval_t;

// Encapuslates a map of UUID -> GTID intervals.
class Gtid_Set {
	public:
		std::unordered_map<std::string, std::list<gtid_interval_t>> map;

	public:
		Gtid_Set();

		void add(const std::string& uuid, const gtid_interval_t& iv);
		void add(const std::string& uuid, const gtid_t& gtid);
		void add(const std::string& uuid, const gtid_t& start, const gtid_t& end);

		const bool has_gtid(const std::string uuid, const gtid_t gtid);
		const std::string to_string(void);
};
typedef Gtid_Set gtid_set_t;

/*
class Gtid_Server_Info {
	public:
	gtid_set_t executed_gtid_set;
	char *hostname;
	uint16_t mysql_port;
	uint16_t gtid_port;
	bool active;
	Gtid_Server_Info(char *_h, uint16_t _mp, uint16_t _gp) {
		hostname = strdup(_h);
		mysql_port = _mp;
		gtid_port = _gp;
		active = true;
	};
	~Gtid_Server_Info() {
		free(hostname);
	};
};
*/

#endif /* PROXYSQL_GTID */