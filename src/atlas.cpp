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

#include <tuple>
#include "atlas.hpp"

using std::make_tuple;

namespace atlas {

	vector<tuple<string, float>> convertAtlas(const Document& traceroute) {
		vector<tuple<string, float>> hops(traceroute["result"].Size());

		const auto& hopsArray = traceroute["result"].GetArray();
		for (const auto& hop : hopsArray) {
			// Verification -- not all hops have results
			if (!hop.HasMember("result"))
				continue;

			string src = getHopSource(hop["result"].GetArray());
			if (src.empty())
				hops.emplace_back("", 0); // Error condition for this hop

			hops.emplace_back(src, rttAverage(hop["result"].GetArray()));
		}

		return hops;
	}

	float rttAverage(GenericArray<true, GenericValue<UTF8<char>>::ValueType> hops) {
		float total = 0;
		for (const auto& hop : hops) {
			if (!hop.HasMember("rtt"))
				continue;
			total += hop["rtt"].GetFloat();
		}

		if (total == 0)
			return -1;

		return total / (float)hops.Size();
	}

	string getHopSource(GenericArray<true, GenericValue<UTF8<char>>::ValueType> hops) {
		for (const auto& hop :hops) {
			if (hop.HasMember("from"))
				return hop["from"].GetString();
		}
		return "";
	}
}