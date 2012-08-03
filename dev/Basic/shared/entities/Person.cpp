/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "Person.hpp"

#include <algorithm>

//For debugging
#include "entities/roles/driver/Driver.hpp"
#include "entities/roles/activityRole/ActivityPerformer.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "entities/roles/pedestrian/Pedestrian.hpp"
#include "util/DebugFlags.hpp"
#include "util/OutputUtil.hpp"

#include "geospatial/Node.hpp"
#include "entities/misc/TripChain.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "entities/misc/TripChain.hpp"
#endif

using std::vector;
using namespace sim_mob;
typedef Entity::UpdateStatus UpdateStatus;

sim_mob::Person::Person(const MutexStrategy& mtxStrat, int id) :
	Agent(mtxStrat, id), prevRole(nullptr), currRole(nullptr), currTripChainItem(nullptr), currSubTrip(nullptr), firstFrameTick(true)
{
	//throw 1;
}

sim_mob::Person::~Person() {
	safe_delete_item(currRole);
}

Person* sim_mob::Person::GeneratePersonFromPending(const PendingEntity& p)
{
	const ConfigParams& config = ConfigParams::GetInstance();

	//Raw agents can simply be returned as-is.
	if (p.type == ENTITY_RAWAGENT) {
		return p.rawAgent;
	}

	//Create a person object.
	Person* res = new Person(config.mutexStategy, p.manualID);

	//Set its mode.
	if (p.type == ENTITY_DRIVER) {
		res->changeRole(new Driver(res, config.mutexStategy));
	} else if (p.type == ENTITY_PEDESTRIAN) {
		res->changeRole(new Pedestrian(res, res->getGenerator()));
	} else if (p.type == ENTITY_BUSDRIVER) {
		res->changeRole(new BusDriver(res, config.mutexStategy));
	} else if (p.type == ENTITY_ACTIVITYPERFORMER){
		// First trip chain item is Activity when Person is generated from Pending
		res->changeRole(new ActivityPerformer(res, dynamic_cast<const sim_mob::Activity&>(*(p.entityTripChain.front())))); 
	} else {
		throw std::runtime_error("PendingEntity currently only supports Drivers, Pedestrians and Activity performers.");
	}

	//Set its origin, destination, startTime, etc.
	res->originNode = p.origin;
	res->destNode = p.dest;
	res->setStartTime(p.start);

	//added by Jenny to handle activities
	//NOTE: I am disabling these for now; please check Harish's work and selectively enable them as needed. ~Seth
	//res->setActivities(p.activities);
	//res->setNextEvent(nullptr);
	//res->setNextActivity(nullptr);
	//res->setOnActivity(false);
	res->setNextPathPlanned(false);
	res->setTripChain(p.entityTripChain);
	res->findNextItemInTripChain();

	return res;
}

void sim_mob::Person::getNextSubTripInTrip(){
	if(!currTripChainItem || currTripChainItem->itemType == sim_mob::TripChainItem::IT_ACTIVITY){
		currSubTrip = nullptr;
	}
	else if(this->currTripChainItem->itemType == sim_mob::TripChainItem::IT_TRIP){
		const Trip* currTrip = dynamic_cast<const Trip*>(this->currTripChainItem);
		const vector<SubTrip>& currSubTripsList = currTrip->getSubTrips();
		if(!currSubTrip) {
			//Return the first sub trip if the current sub trip which is passed in is null
			currSubTrip = &currSubTripsList.front();
		} else {
			// else return the next sub trip if available; return nullptr otherwise.
			vector<SubTrip>::const_iterator subTripIterator = std::find(currSubTripsList.begin(), currSubTripsList.end(), (*currSubTrip));
			currSubTrip = nullptr;
			if (subTripIterator!=currSubTripsList.end()) {
				//Set it equal to the next item, assuming we are not at the end of the list.
				if (++subTripIterator != currSubTripsList.end()) {
					currSubTrip = &(*subTripIterator);
				}
			}
		}
	}
	else{
		throw std::runtime_error("Invalid trip chain item type!");
	}
}

void sim_mob::Person::findNextItemInTripChain() {
	if(!this->currTripChainItem){
		//set the first item if the current item is null
		this->currTripChainItem = this->tripChain.front();
	} else {
		// else set the next item if available; return nullptr otherwise.
		std::vector<const TripChainItem*>::const_iterator itemIterator = std::find(tripChain.begin(), tripChain.end(), currTripChainItem);
		currTripChainItem = nullptr;
		if (itemIterator!=tripChain.end()) {
			//Set it equal to the next item, assuming we are not at the end of the list.
			if (++itemIterator != tripChain.end()) {
				currTripChainItem = *itemIterator;
			}
		}
	}

	this->getNextSubTripInTrip(); //if currTripChainItem is Trip, set currSubTrip as well

}

UpdateStatus sim_mob::Person::update(frame_t frameNumber) {
#ifdef SIMMOB_AGENT_UPDATE_PROFILE
		profile.logAgentUpdateBegin(*this, frameNumber);
#endif

	UpdateStatus retVal(UpdateStatus::RS_CONTINUE);
#ifndef SIMMOB_STRICT_AGENT_ERRORS
	try {
#endif
		//First, we need to retrieve an UpdateParams subclass appropriate for this Agent.
		unsigned int currTimeMS = frameNumber * ConfigParams::GetInstance().baseGranMS;
		UpdateParams& params = currRole->make_frame_tick_params(frameNumber, currTimeMS);
		//std::cout<<"Person ID:"<<this->getId()<<"---->"<<"Person position:"<<"("<<this->xPos<<","<<this->yPos<<")"<<std::endl;

		//Has update() been called early?
		if(currTimeMS < getStartTime()) {
			//This only represents an error if dynamic dispatch is enabled. Else, we silently skip this update.
			if (!ConfigParams::GetInstance().DynamicDispatchDisabled()) {
				std::stringstream msg;
				msg << "Agent(" << getId() << ") specifies a start time of: " << getStartTime()
						<< " but it is currently: " << currTimeMS
						<< "; this indicates an error, and should be handled automatically.";
				throw std::runtime_error(msg.str().c_str());
			}
			return UpdateStatus::Continue;
		}

		//Has update() been called too late?
		if (isToBeRemoved()) {
			//This only represents an error if dynamic dispatch is enabled. Else, we silently skip this update.
			if (!ConfigParams::GetInstance().DynamicDispatchDisabled()) {
				throw std::runtime_error("Agent is already done, but hasn't been removed.");
			}
			return UpdateStatus::Continue;
		}

		//Is this the first frame tick for this Agent?
		if (firstFrameTick) {
			//Helper check; not needed once we trust our Workers.
			if (!ConfigParams::GetInstance().DynamicDispatchDisabled()) {
				if (abs(currTimeMS-getStartTime())>=ConfigParams::GetInstance().baseGranMS) {
					std::stringstream msg;
					msg << "Agent was not started within one timespan of its requested start time.";
					msg << "\nStart was: " << getStartTime() << ",  Curr time is: " << currTimeMS << "\n";
					msg << "Agent ID: " << getId() << "\n";
					throw std::runtime_error(msg.str().c_str());
				}
			}

			currRole->frame_init(params);
			firstFrameTick = false;
		}

		//Now perform the main update tick
		if (!isToBeRemoved()) {
			//added to get the detailed plan before next activity
			currRole->frame_tick(params);
			//if mid-term
			//currRole->frame_tick_med(params);
		}

		//Finally, save the output
		if (!isToBeRemoved()) {
			currRole->frame_tick_output(params);
			//if mid-term
			//currRole->frame_tick_med(params);
		}

		//If we're "done", try checking to see if we have any more items in our Trip Chain.
		// This is not strictly the right way to do things (we shouldn't use "isToBeRemoved()"
		// in this manner), but it's the easiest solution that uses the current API.
		if (isToBeRemoved()) {
			retVal = checkAndReactToTripChain(currTimeMS);
		}

		//Output if removal requested.
		if (Debug::WorkGroupSemantics && isToBeRemoved()) {
#ifndef SIMMOB_DISABLE_OUTPUT
			boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
			std::cout << "Person requested removal: " << (dynamic_cast<Driver*> (currRole) ? "Driver"
					: dynamic_cast<Pedestrian*> (currRole) ? "Pedestrian" : "Other") << "\n";
#endif
		}


//Respond to errors only if STRICT is off; otherwise, throw it (so we can catch it in the debugger).
#ifndef SIMMOB_STRICT_AGENT_ERRORS
	} catch (std::exception& ex) {
#ifdef SIMMOB_AGENT_UPDATE_PROFILE
		profile.logAgentException(*this, frameNumber, ex);
#endif

			//Add a line to the output file.
#ifndef SIMMOB_DISABLE_OUTPUT
			std::stringstream msg;
			msg <<"Error updating Agent[" <<getId() <<"], will be removed from the simulation.";
			msg <<"\nFrom node: " <<(originNode?originNode->originalDB_ID.getLogItem():"<Unknown>");
			msg <<"\nTo node: " <<(destNode?destNode->originalDB_ID.getLogItem():"<Unknown>");
			msg <<"\n" <<ex.what();
			LogOut(msg.str() <<std::endl);
#endif
			setToBeRemoved();
	}
#endif

	//Return true unless we are scheduled for removal.
	//NOTE: Make sure you set this flag AFTER performing your final output.
	if (isToBeRemoved()) {
		retVal.status = UpdateStatus::RS_DONE;
	}

#ifdef SIMMOB_AGENT_UPDATE_PROFILE
		profile.logAgentUpdateEnd(*this, frameNumber);
#endif
	return retVal;
}


UpdateStatus sim_mob::Person::checkAndReactToTripChain(unsigned int currTimeMS) {
	this->getNextSubTripInTrip();

	if(this->currSubTrip == nullptr){
		this->findNextItemInTripChain();
	}

	if (this->currTripChainItem == nullptr) {
		return UpdateStatus::Done;
	}

	//Prepare to delete the previous Role. We _could_ delete it now somewhat safely, but
	// it's better to avoid possible errors (e.g., if the equality operator is defined)
	// by saving it until the next time tick.
	safe_delete_item(prevRole);
	prevRole = currRole;

	//Create a new Role based on the trip chain type
	if(this->currTripChainItem->itemType == sim_mob::TripChainItem::IT_TRIP){
		if (this->currSubTrip->mode == "Car") {
			//Temp. (Easy to add in later)
			throw std::runtime_error("Cars not supported in Trip Chain role change.");
		} else if (this->currSubTrip->mode == "Walk") {
			changeRole(new Pedestrian(this, gen));
		} else {
			throw std::runtime_error("Unknown role type for trip chain role change.");
		}

		//Update our origin/dest pair.
		originNode = this->currSubTrip->fromLocation;
		destNode = this->currSubTrip->toLocation;
	} else if(this->currTripChainItem->itemType == sim_mob::TripChainItem::IT_ACTIVITY){
		const Activity& currActivity = dynamic_cast<const Activity&>(*currTripChainItem);
		changeRole(new ActivityPerformer(this, currActivity));
		//Update our origin/dest pair.
		originNode = destNode = currActivity.location;
	} else {
		throw std::runtime_error("Unknown item type in trip chain");
	}


	//Create a return type based on the differences in these Roles
	UpdateStatus res(UpdateStatus::RS_CONTINUE, prevRole->getSubscriptionParams(), currRole->getSubscriptionParams());

	//Set our start time to the NEXT time tick so that frame_init is called
	//  on the first pass through.
	//TODO: This might also be better handled in the worker class.
	setStartTime(currTimeMS + ConfigParams::GetInstance().baseGranMS);
	firstFrameTick = true;

	//Null out our trip chain, remove the "removed" flag, and return
//	setTripChainItem(nullptr);
	clearToBeRemoved();
	return res;
}



void sim_mob::Person::buildSubscriptionList(vector<BufferedBase*>& subsList) {
	//First, add the x and y co-ordinates
	Agent::buildSubscriptionList(subsList);

	//Now, add our own properties.
	vector<BufferedBase*> roleParams = this->getRole()->getSubscriptionParams();
	for (vector<BufferedBase*>::iterator it = roleParams.begin(); it != roleParams.end(); it++) {
		subsList.push_back(*it);
	}
}

//TODO: If we're going to use this, we'll have to integrate property management somewhere sensible (maybe here).
void sim_mob::Person::changeRole(sim_mob::Role* newRole) {
	if (this->currRole) {
		this->currRole->setParent(nullptr);
	}

	this->currRole = newRole;

	if (this->currRole) {
		this->currRole->setParent(this);
	}
}

sim_mob::Role* sim_mob::Person::getRole() const {
	return currRole;
}

sim_mob::Link* sim_mob::Person::getCurrLink(){
	return currLink;
}
void sim_mob::Person::setCurrLink(sim_mob::Link* link){
	currLink = link;
}

#ifndef SIMMOB_DISABLE_MPI
/*
 * package Entity, Agent, Person and Role
 */
//void sim_mob::Person::pack(PackageUtils& packageUtil) {
//	//package Entity
//	//std::cout << "Person package Called" << this->getId() << std::endl;
//	sim_mob::Agent::pack(packageUtil);
//
//	//package person
//	packageUtil.packBasicData(specialStr);
//	sim_mob::TripChain::pack(packageUtil, currTripChain);
//	packageUtil.packBasicData(firstFrameTick);
//}
//
//void sim_mob::Person::unpack(UnPackageUtils& unpackageUtil) {
//
//	sim_mob::Agent::unpack(unpackageUtil);
//	//std::cout << "Person unpackage Called" << this->getId() << std::endl;
//
//	specialStr = unpackageUtil.unpackBasicData<std::string> ();
//	currTripChain = sim_mob::TripChain::unpack(unpackageUtil);
//	firstFrameTick = unpackageUtil.unpackBasicData<bool> ();
//}
//
//void sim_mob::Person::packProxy(PackageUtils& packageUtil) {
//	//package Entity
//	sim_mob::Agent::pack(packageUtil);
//
//	//package person
//	packageUtil.packBasicData(specialStr);
//	packageUtil.packBasicData(firstFrameTick);
//}
//
//void sim_mob::Person::unpackProxy(UnPackageUtils& unpackageUtil) {
//	sim_mob::Agent::unpack(unpackageUtil);
//
//	specialStr = unpackageUtil.unpackBasicData<std::string> ();
//	firstFrameTick = unpackageUtil.unpackBasicData<bool> ();
//}

#endif