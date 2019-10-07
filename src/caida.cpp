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
#include "caida.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <tuple>

using std::cerr;
using std::endl;

namespace caida {
	vector<tuple<string, float>> convertCaida(const Document& traceroute) {
		vector<tuple<string, float>> hops;
		hops.reserve(traceroute["hops"].GetArray().Size());

		for (const auto& hop : traceroute["hops"].GetArray())
			hops.emplace_back(hop["addr"].GetString(), hop["rtt"].GetFloat());

		return hops;
	}

	string resolveHostname(const string& domain) {
		sockaddr_in addr{};
		if (getnameinfo(reinterpret_cast<const sockaddr *>(&addr),
				sizeof(sockaddr_in), (char*)domain.c_str(),
				domain.length(),
				nullptr,
				0,
				NI_NAMEREQD) != 0) {
			cerr << "Failed to resolve " << domain << endl;
			return "";
		}

		return inet_ntoa(addr.sin_addr);
	}
}