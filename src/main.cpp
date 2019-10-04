#include <iostream>
#include "cmdline.h"

int main(int argc, char** argv) {
	gengetopt_args_info args{};
	cmdline_parser(argc, argv, &args);
}