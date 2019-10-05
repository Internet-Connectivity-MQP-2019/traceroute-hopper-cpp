#include <algorithm>
#include <iostream>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <pqxx/pqxx>
#include <vector>
#include "cmdline.h"

using namespace std;
using namespace rapidjson;

void saveToDb(const vector<tuple<string, string, float, int, bool> >& hops, pqxx::dbtransaction& cxn);
float rttAverage(const GenericArray<false, GenericValue<UTF8<char>>::ValueType>& hops);
string getHopSource(const GenericArray<false, GenericValue<UTF8<char>>::ValueType>& hops);

int main(int argc, char** argv) {
	gengetopt_args_info args{};
	cmdline_parser(argc, argv, &args);

	// Load database parameters for connection
	ifstream dbconfig_file(args.dbconfig_arg);
	if (!dbconfig_file.is_open()) {
		cerr << "Failed to open database configuration in " << args.dbconfig_arg << endl;
		return 1;
	}
	Document dbconfig;
	IStreamWrapper isw(dbconfig_file);
	dbconfig.ParseStream(isw);
	dbconfig_file.close();

	// Connect to database
	pqxx::connection cxn("host=" + string(dbconfig["host"].GetString()) + " dbname=" + string(dbconfig["database"].GetString()) +
								" user=" + string(dbconfig["user"].GetString()));
	pqxx::work work(cxn);

	// Process each file in the list of given input files
	bool skippedFiles = false;
	vector<tuple<string, string, float, int, bool> > hops;
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
		unsigned int count = 0;
		while(getline(file, line)) {
			Document traceroute;
			traceroute.Parse(line.c_str());
			count++;

			// If the buffer is full, save it to the database
			if (count % args.buffer_arg == 0 && !hops.empty()) {
				saveToDb(hops, work);
				if (args.verbose_given)
					cout << "Processed " << count << " traceroutes" << endl;
				hops.clear();
			}

			// Establish base source and perform basic checking
			if (args.caida_given) {
				// TODO: RESOLVE DOMAIN NAME
			} else if (args.atlas_given) {
				baseSrc = traceroute["from"].GetString();
				if (baseSrc.length() == 0)
					continue;

				// Validation on Atlas data, since some traceroutes fail
				if (traceroute["result"].Size() == 1 && traceroute["result"][0].HasMember("error"))
					continue;
			}

			// Get timestamp
			if (args.atlas_given)
				time = traceroute["timestamp"].GetInt();
			else
				time = traceroute["start"]["sec"].GetInt();

			if (args.ping_given) {
				// Ping mode -- just do one entry, source to final destination
				if (args.atlas_given) {
					float avg = rttAverage(traceroute["result"][traceroute["result"].Size() - 1]["result"].GetArray());
					if (avg < 0)
						continue; // Can't parse this traceroute
					hops.push_back(make_tuple(baseSrc, traceroute["dst_addr"].GetString(), avg, time, false));
				} else if (args.caida_given) {
					hops.push_back(make_tuple(baseSrc, traceroute["dst"].GetString(), traceroute["hops"][traceroute["hops"].Size() - 1]["rtt"].GetFloat(), time, false));
				}
			} else {
				// Either direct or calculated mode
				const auto& hopsArray = args.caida_given ? traceroute["hops"].GetArray() : traceroute["result"].GetArray();
				int j = -1;
				for (auto& hop : hopsArray) {
					j++;
					float rtt = 0;

					// Verification and pre-processing for Atlas data
					if (args.atlas_given) {
						if (!hop.HasMember("result"))
							continue;
					}

					// Add a hop for the direct source-> hop entry
					if (args.atlas_given) {
						string dst = getHopSource(hop["result"].GetArray());
						if (dst.empty())
							continue;
						rtt = rttAverage(hop["result"].GetArray());
						hops.push_back(make_tuple(baseSrc, dst, rtt, time, false));
					}
					else if (args.caida_given)
						hops.push_back(make_tuple(baseSrc, hop["addr"].GetString(), hop["rtt"].GetFloat(), time, false));

					if (!args.calculate_given || j == 0)
						continue;

					// Full calculation mode for times between individual nodes in the path
					string src, dst;
					if (args.atlas_given) {
						if (hop["result"].Size() == 0)
							continue;

						const auto& last_hop = traceroute["result"][j-1].GetObject();
						src = getHopSource(last_hop["result"].GetArray());
						float temp = rttAverage(last_hop["result"].GetArray());
						if (temp < 0)
							continue;
						rtt = rtt - temp;
						dst = getHopSource(hop["result"].GetArray());
						if (dst.empty())
							continue;
					} else if (args.caida_given) {
						const auto& last_hop = traceroute["hops"][j - 1].GetObject();
						src = last_hop["addr"].GetString();
						rtt = hop["rtt"].GetFloat() - last_hop["rtt"].GetFloat();
						dst = hop["addr"].GetString();
					}
					hops.push_back(make_tuple(src, dst, rtt, time, true));
				}
			}

		}
		saveToDb(hops, work);
		file.close();
	}

	if (skippedFiles) {
		cerr << "Warning: files were skipped, check log for details!" << endl;
		return 2;
	}

	cout << "Committing to database..." << endl;
	work.commit();
	cxn.disconnect();
	cout << "Committed!" << endl;

	return 0;
}

void saveToDb(const vector<tuple<string, string, float, int, bool> >& hops, pqxx::dbtransaction& cxn) {
	pqxx::stream_to stream(cxn, "hops", vector<string>{"src", "dst", "rtt", "time", "indirect"});
	for (const auto& hop : hops)
		stream << hop;
}

float rttAverage(const GenericArray<false, GenericValue<UTF8<char>>::ValueType>& hops) {
	float total = 0;
	for (auto& hop : hops) {
		if (!hop.HasMember("rtt"))
			continue;
		total += hop["rtt"].GetFloat();
	}
	if (total == 0)
		return -1;
	return total / (float)hops.Size();
}

string getHopSource(const GenericArray<false, GenericValue<UTF8<char>>::ValueType>& hops) {
	for (const auto& hop : hops) {
		if (hop.HasMember("from"))
			return hop["from"].GetString();
	}
	return "";
}
