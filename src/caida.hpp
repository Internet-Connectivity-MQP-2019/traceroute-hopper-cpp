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
#ifndef TRACEROUTE_HOPPER_CAIDA_HPP
#define TRACEROUTE_HOPPER_CAIDA_HPP

#import <rapidjson/document.h>
#import <vector>

using rapidjson::Document;
using std::vector;
using std::tuple;
using std::string;

namespace caida {
	/**
	 * Converts a CAIDA traceroute into a series of IP/RTT tuples.
	 * @param traceroute Traceroute.
	 * @return IP/RTT tuples.
	 */
	vector<tuple<string, float>> convertCaida(const Document& traceroute);

	/**
	 * Does what it says on the box
	 * @param domain Domain name
	 * @return IP
	 */
	string resolveHostname(string domain);
}

#endif //TRACEROUTE_HOPPER_CAIDA_HPP
