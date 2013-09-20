//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "geo10-pimpl.hpp"

#include <stdexcept>

using namespace sim_mob::xml;

namespace {

//Helper: Get the TripChain item type from a string.
sim_mob::TripChainItem::ItemType GetTripChainItemType(std::string name)
{
	if (name == "IT_TRIP") {
		return sim_mob::TripChainItem::IT_TRIP;
	} else if (name == "IT_ACTIVITY") {
		return sim_mob::TripChainItem::IT_ACTIVITY;
	} else if( name == "IT_FMODSIM"){
		return sim_mob::TripChainItem::IT_FMODSIM;
	}

	throw std::runtime_error("Unknown TripChain item type.");
}

} //End unnamed namespace


void sim_mob::xml::TripChainItem_t_pimpl::pre ()
{
	model = sim_mob::TripChainItem();
}

sim_mob::TripChainItem* sim_mob::xml::TripChainItem_t_pimpl::post_TripChainItem_t ()
{
	sim_mob::TripChainItem* res = new sim_mob::TripChainItem(model);
	return res;
}

void sim_mob::xml::TripChainItem_t_pimpl::personID (std::string value)
{
	model.setPersonID(value);
	//std::cout << "A tripchain Item for person " << model.personID << " is being read" << std::endl;
}

void sim_mob::xml::TripChainItem_t_pimpl::itemType (std::string value)
{
	model.itemType = GetTripChainItemType(value);
}

void sim_mob::xml::TripChainItem_t_pimpl::sequenceNumber (unsigned int value)
{
	model.sequenceNumber = value;
}

void sim_mob::xml::TripChainItem_t_pimpl::requestTime (long long value)
{
	model.requestTime = value;
}

void sim_mob::xml::TripChainItem_t_pimpl::startTime (const ::std::string& value)
{
	//This should be equivalent. ~Seth
	model.startTime = DailyTime(value);
}

void sim_mob::xml::TripChainItem_t_pimpl::endTime (const ::std::string& value)
{
	//This should be equivalent. ~Seth
	model.endTime = DailyTime(value);
}




