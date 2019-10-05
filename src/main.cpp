#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <vector>
#include "cmdline.h"

using namespace std;
using json = nlohmann::json;

struct Hop {
public:
	Hop(const string& src, const string& dst, const float rtt, const int time, const bool indirect) {
		this->src = src;
		this->dst = dst;
		this->rtt = rtt;
		this->time = time;
		this->indirect = indirect;
	}

	string src;
	string dst;
	float rtt;
	int time;
	bool indirect;
};

void saveToDb(const vector<Hop>& hops, const pqxx::connection& cxn);
float average(const vector<float>& values);

int main(int argc, char** argv) {
	gengetopt_args_info args{};
	cmdline_parser(argc, argv, &args);

	// Load database parameters for connection
	ifstream dbconfig_file(args.dbconfig_arg);
	if (!dbconfig_file.is_open()) {
		cerr << "Failed to open database configuration in " << args.dbconfig_arg << endl;
		return 1;
	}
	json dbconfig;
	dbconfig_file >> dbconfig;
	dbconfig_file.close();

	// Connect to database
	pqxx::connection cxn("host=" + string(dbconfig["host"]) + " dbname=" + string(dbconfig["database"]) +
								" user=" + string(dbconfig["user"]));

	// Process each file in the list of given input files
	bool skippedFiles = false;
	vector<Hop> hops;
	hops.reserve(args.buffer_arg + 1);
	for (unsigned int fileNum = 0; fileNum < args.inputs_num; fileNum++) {
		if (args.verbose_given)
			cout << "Processing " << args.inputs[fileNum] << endl;

		ifstream file(args.inputs[fileNum]);
		if (!file.is_open()) {
			cerr << "Failed to open input file " << args.inputs[fileNum] << ", skipping!";
			skippedFiles = true;
		}

		// Loop over each individual traceroute and process it
		string line;
		string baseSrc;
		int time;
		vector<float> tempRtts; // Every Atlas traceroute will need special processing, so save a vector on the side.
		while(getline(file, line)) {
			json traceroute = json::parse(line);

			// If the buffer is full, save it to the database
			if (hops.size() % args.buffer_arg == 0 && !hops.empty()) {
				saveToDb(hops, cxn);
				if (args.verbose_given)
					cout << "Processed " << hops.size() << " traceroutes";
				cout.flush();
				hops.clear();
			}

			// Establish base source and perform basic checking
			if (args.caida_given) {
				// TODO: RESOLVE DOMAIN NAME
			} else if (args.atlas_given) {
				baseSrc = traceroute["from"].get<string>();
				if (baseSrc.length() == 0)
					continue;

				// Validation on Atlas data, since some traceroutes fail
				if (traceroute["result"].size() == 1 && traceroute["result"][0].contains("error"))
					continue;
			}

			// Get timestamp
			if (args.atlas_given)
				time = traceroute["timestamp"].get<int>();
			else
				time = traceroute["start"]["sec"].get<int>();

			if (args.ping_given) {
				// Ping mode -- just do one entry, source to final destination
				if (args.atlas_given) {
					bool invalid = false;
					for (auto result : traceroute["result"][traceroute["result"].size() - 1]["result"]) {
						if (!result["rtt"].is_number_float()) {
							invalid = true;
							break;
						}
						tempRtts.push_back(result["rtt"].get<float>());
					}
					if (invalid)
						continue; // Can't parse this traceroute

					Hop hop(baseSrc, traceroute["dst_addr"].get<string>(), average(tempRtts), time, false);
					tempRtts.clear();
					hops.push_back(hop);
				} else if (args.caida_given) {
					Hop hop(baseSrc, traceroute["dst"].get<string>(), traceroute["hops"][traceroute["hops"].size() - 1]["rtt"].get<float>(), time, false);
					hops.push_back(hop);
				}
			}

		}
		cout << "Processed " << hops.size() << " hops" << endl;
		file.close();
	}

	if (skippedFiles) {
		cerr << "Warning: files were skipped, check log for details!" << endl;
		return 2;
	}

	return 0;
}

void saveToDb(const vector<Hop>& hops, const pqxx::connection& cxn) {

}

float average(const vector<float>& values) {
	float total = 0;
	for (auto value: values) total += value;
	return values.empty() ? 0 : total / values.size();
}
