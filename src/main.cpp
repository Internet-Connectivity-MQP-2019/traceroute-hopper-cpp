#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include "cmdline.h"

using namespace std;
using json = nlohmann::json;

int main(int argc, char** argv) {
	gengetopt_args_info args{};
	cmdline_parser(argc, argv, &args);

	// Load database parameters for connection
	fstream dbconfig_file;
	dbconfig_file.open(args.dbconfig_arg, ios::in);
	if (!dbconfig_file.is_open()) {
		cerr << "Failed to open database configuration in " << args.dbconfig_arg << endl;
		return 1;
	}
	json dbconfig;
	dbconfig_file >> dbconfig;
	dbconfig_file.close();
}