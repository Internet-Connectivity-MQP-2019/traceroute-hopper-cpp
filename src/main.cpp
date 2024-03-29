/*
 * Copyright (c) 2019 Christopher Myers
 *
 * This file is part of traceroute_hopper.
 *
 * traceroute_hopper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * traceroute_hopper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with traceroute_hopper.  If not, see <https://www.gnu.org/licenses/>.
 */

// Enable SSE4.2/SSE2 optimizations for rapidjson
#if defined(__SSE4_2__)
#  define RAPIDJSON_SSE42
#elif defined(__SSE2__)
#  define RAPIDJSON_SSE2
#endif
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <pqxx/pqxx>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "cmdline.h"
#include "caida.hpp"
#include "atlas.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::get;

void saveToDb(const vector<tuple<string, string, float, int, bool> >& hops, pqxx::dbtransaction& cxn);

int main(int argc, char** argv) {
	gengetopt_args_info args{};
	cmdline_parser(argc, argv, &args);

	// Load database parameters for connection
	std::ifstream dbconfig_file(args.dbconfig_arg);
	if (!dbconfig_file.is_open()) {
		cerr << "Failed to open database configuration in " << args.dbconfig_arg << endl;
		return 1;
	}
	Document dbconfig;
	rapidjson::IStreamWrapper isw(dbconfig_file);
	dbconfig.ParseStream(isw);
	dbconfig_file.close();

	// Connect to database
	pqxx::connection cxn("host=" + string(dbconfig["host"].GetString()) + " dbname=" + string(dbconfig["database"].GetString()) +
								" user=" + string(dbconfig["user"].GetString()));
	pqxx::work work(cxn);

	// Process each file in the list of given input files
	bool skippedFiles = false;
	vector<tuple<string, string, float, int, bool>> hops;
	hops.reserve(args.buffer_arg * 10);
	for (unsigned int fileNum = 0; fileNum < args.inputs_num; fileNum++) {
		if (args.verbose_given)
			cout << "Processing " << args.inputs[fileNum] << endl;

		// Map file into memory for faster processing
		struct stat st{};
		stat(args.inputs[fileNum], &st);
		int fd = open(args.inputs[fileNum], O_RDONLY, 0);
		if (fd < 0) {
			cerr << "Failed to open file " << args.inputs[fileNum] << ": " << strerror(fd) << endl;
			skippedFiles = true;
			continue;
		}
		void* data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		if (data == MAP_FAILED) {
			cerr << "Failed to map file " << args.inputs[fileNum] << endl;
			perror("Cause:");
			skippedFiles = true;
			continue;
		}
		std::stringstream file;
		file.rdbuf()->pubsetbuf(static_cast<char *>(data), st.st_size); // Have to convert this into a stream, somehow

		// Loop over each individual traceroute and process it
		string line;
		string baseSrc;
		int time;
		unsigned int count = 0;
		vector<tuple<string, float>> rawHops(50);
		while(getline(file, line)) {
			Document traceroute;
			traceroute.Parse(line.c_str());
			count++;

			// If the buffer is full, save its contents to the database
			if (count % args.buffer_arg == 0 && !hops.empty()) {
				saveToDb(hops, work);
				if (args.verbose_given)
					cout << "Processed " << count << " traceroutes" << endl;
				hops.clear();
			}

			// Establish base source, timestamp, perform basic checking, and convert to a generic list of hops
			if (args.caida_given) {
				// Resolve hostname and skip this line since it's not an actual traceroute
				if (traceroute.HasMember("hostname")) {
					baseSrc = caida::resolveHostname(traceroute["hostname"].GetString());
					if (baseSrc.empty())
						return 2;
					continue;
				}
				time = caida::getTimestamp(traceroute);
				rawHops = caida::convertCaida(traceroute);

			} else if (args.atlas_given) {
				baseSrc = traceroute["from"].GetString();
				if (baseSrc.empty() || atlas::checkIfFailed(traceroute))
					continue;

				time = atlas::getTimestamp(traceroute);
				rawHops = atlas::convertAtlas(traceroute);
			}

			if (args.ping_given) {
				// Ping mode -- just do one entry, source to final destination
				float rtt = get<1>(rawHops[rawHops.size() - 1]);
				if (rtt < 0)
					continue; // Can't parse this traceroute
				hops.emplace_back(baseSrc, get<0>(rawHops[rawHops.size() - 1]), rtt, time, false);
			} else {
				// Either direct or calculated mode
				int j = -1;
				for (auto& hop : rawHops) {
					j++;

					// Add a hop for the direct source-> hop entry
					if (get<1>(hop) < 0 || get<0>(hop).empty())
						continue; // Bad hop
					hops.emplace_back(baseSrc, get<0>(hop), get<1>(hop), time, false);

					// If we're not on calculate mode OR we're at the first hop, skip. Processing the first hop would be
					// redundant since direct mode already picks it up.
					if (!args.calculate_given || j == 0)
						continue;

					// Make sure the last hop is valid, otherwise we can't calculate an rtt
					tuple<string, float> lastHop = rawHops[j - 1];
					if (get<0>(lastHop).empty() || get<1>(lastHop) < 0)
						continue;

					hops.emplace_back(get<0>(lastHop), get<0>(hop), get<1>(hop) - get<1>(lastHop), time, true);
				}
			}
		}

		if (args.verbose_given)
			cout << "Finished processing " << args.inputs[fileNum] << endl;
		saveToDb(hops, work);
		munmap(data, st.st_size);
		close(fd);
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
