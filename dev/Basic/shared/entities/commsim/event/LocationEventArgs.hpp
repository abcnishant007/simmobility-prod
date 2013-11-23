//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * LocationEventArgs.hpp
 *
 *  Created on: May 28, 2013
 *      Author: vahid
 */

#pragma once

#include "event/args/EventArgs.hpp"
#include "event/EventListener.hpp"
#include<iostream>
#include "entities/commsim/serialization/JsonParser.hpp"

namespace sim_mob {
class Agent;
DECLARE_CUSTOM_CALLBACK_TYPE(LocationEventArgs)
class LocationEventArgs: public sim_mob::event::EventArgs {
public:
	LocationEventArgs(const sim_mob::Agent *);
	virtual ~LocationEventArgs();

	//todo should be a virtual from a base class
	Json::Value ToJSON()const;

private:
	const sim_mob::Agent *agent;

};

} /* namespace sim_mob */
