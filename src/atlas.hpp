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

#include <rapidjson/document.h>
#include <vector>

using rapidjson::Document;
using rapidjson::GenericArray;
using rapidjson::GenericValue;
using rapidjson::UTF8;
using std::string;
using std::tuple;
using std::vector;

namespace atlas {

	/**
	 * Convert a JSON representation of a bunch of hops into a more concise list of tuples that's easier to process.
	 * @param hops List of hops
	 * @return Vector form of hops
	 */
	vector<tuple<string, float>> convertAtlas(const Document& traceroute);

	/**
	 * Get the average RTT for a given series of one hop
	 * @param hops Hops
	 * @return Average for the given hop, -1 if average failed.
	 */
	float rttAverage(const GenericArray<true, GenericValue<UTF8<char>>::ValueType>& hops);

	/**
	 * Get the source of a hop (because Atlas is weird)
	 * @param hops List of hop measurements
	 * @return Hop source, empty if no hop recorded
	 */
	string getHopSource(const GenericArray<true, GenericValue<UTF8<char>>::ValueType>& hops);

	/**
	 * Check if the traceroute failed and should be discarded
	 * @param traceroute
	 * @return
	 */
	inline bool checkIfFailed(const Document& traceroute) {
		return traceroute["result"].Size() == 1 && traceroute["result"][0].HasMember("error");
	}

	/**
	 * Get timestamp
	 * @param traceroute Traceroute
	 * @return Timestamp
	 */
	inline int getTimestamp(const Document& traceroute) {
		return traceroute["timestamp"].GetInt();
	}
};
