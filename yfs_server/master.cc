/*
 * master.cc
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#include "master.h"

master::master(std::string primary, std::string me){
	rsm_server = new rsm(primary, me);
}
