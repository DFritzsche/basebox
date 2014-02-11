/*
 * cconfig.cc
 *
 *  Created on: 17.12.2013
 *      Author: andreas
 */

#include "cconfig.h"

using namespace ethercore;

std::string const cconfig::CONFPATH = std::string("./ethcored.conf");

cconfig* cconfig::scconfig = (cconfig*)0;


cconfig&
cconfig::get_instance() {
	if (0 == cconfig::scconfig) {
		cconfig::scconfig = new cconfig();
	}
	return *(cconfig::scconfig);
}


void
cconfig::open(std::string const& confpath)
{
	try {
		this->confpath = confpath;
		Config::readFile(confpath.c_str());
	} catch (FileIOException& e) {
		std::cerr << "[ethcore][config] unable to find config file \"" << confpath << "\", continuing with defaults." << std::endl;
	}
}


