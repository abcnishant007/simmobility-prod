/*
 * Conflux.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: harish
 */

#include<map>
#include <stdexcept>
#include <vector>
#include "Conflux.hpp"
using namespace sim_mob;
typedef Entity::UpdateStatus UpdateStatus;

void sim_mob::Conflux::addAgent(sim_mob::Agent* ag) {
	/**
	 * The agents always start at a node (for now).
	 * we will always add the agent to the road segment in "lane infinity".
	 */
	sim_mob::SegmentStats* rdSegStats = segmentAgents.find(ag->getCurrSegment())->second;
	ag->setCurrLane(rdSegStats->laneInfinity);
	ag->distanceToEndOfSegment = ag->getCurrSegment()->computeLaneZeroLength();
	rdSegStats->addAgent(rdSegStats->laneInfinity, ag);
}

UpdateStatus sim_mob::Conflux::update(timeslice frameNumber) {
	currFrameNumber = frameNumber;
	resetPositionOfLastUpdatedAgentOnLanes();
	if (sim_mob::StreetDirectory::instance().signalAt(*multiNode) != nullptr) {
		updateUnsignalized(frameNumber); //TODO: Update Signalized must be implemented
	}
	else {
		updateUnsignalized(frameNumber);
	}
	updateSupplyStats(frameNumber);
	UpdateStatus retVal(UpdateStatus::RS_CONTINUE); //always return continue. Confluxes never die.
	return retVal;
}

void sim_mob::Conflux::updateSignalized() {
	throw std::runtime_error("Conflux::updateSignalized() not implemented yet.");
}

void sim_mob::Conflux::updateUnsignalized(timeslice frameNumber) {
	initCandidateAgents();
	sim_mob::Agent* ag = agentClosestToIntersection();
	while (ag) {
		updateAgent(ag);

		// get next agent to update
		ag = agentClosestToIntersection();
	}
}

void sim_mob::Conflux::updateAgent(sim_mob::Agent* ag) {
	const sim_mob::RoadSegment* segBeforeUpdate = ag->getCurrSegment();
	const sim_mob::Lane* laneBeforeUpdate = ag->getCurrLane();
	bool isQueuingBeforeUpdate = ag->isQueuing;
	sim_mob::SegmentStats* segStatsBfrUpdt = findSegStats(segBeforeUpdate);
	debugMsgs << "SegStatsBeforeUpdate:" <<segBeforeUpdate->getStart()->getID() << "Queuing count: "<<
			segStatsBfrUpdt->numQueueingInSegment(true)
			<<" Moving count: "<<segStatsBfrUpdt->numMovingInSegment(true)<<std::endl;
	debugMsgs << "Updating Agent " << ag->getId()  << "Start" << ag->getCurrSegment()->getStart()->getID() << std::endl;

	UpdateStatus res = ag->update(currFrameNumber);
	if (res.status == UpdateStatus::RS_DONE) {
		//This agent is done. Remove from simulation.
		killAgent(ag, segBeforeUpdate, laneBeforeUpdate);
		return;
	} else if (res.status == UpdateStatus::RS_CONTINUE) {
		// TODO: I think there will be nothing here. Have to make sure. ~ Harish
	} else {
		throw std::runtime_error("Unknown/unexpected update() return status.");
	}

	const sim_mob::RoadSegment* segAfterUpdate = ag->getCurrSegment();
	const sim_mob::Lane* laneAfterUpdate = ag->getCurrLane();
	bool isQueuingAfterUpdate = ag->isQueuing;
	sim_mob::SegmentStats* segStatsAftrUpdt = findSegStats(segAfterUpdate);

	if((segBeforeUpdate != segAfterUpdate) || (laneBeforeUpdate == segStatsBfrUpdt->laneInfinity && laneBeforeUpdate != laneAfterUpdate))
	{
		segStatsBfrUpdt->dequeue(laneBeforeUpdate);
		if(laneAfterUpdate) {
			segStatsAftrUpdt->addAgent(laneAfterUpdate, ag);
		}
		else {
			// If we don't know which lane the agent has to go to, we add him to lane infinity.
			// NOTE: One possible scenario for this is an agent who is starting on a new trip chain item.
			ag->setCurrLane(segStatsAftrUpdt->laneInfinity);
			ag->distanceToEndOfSegment = segAfterUpdate->computeLaneZeroLength();
			segStatsAftrUpdt->addAgent(segStatsAftrUpdt->laneInfinity, ag);
			laneAfterUpdate = segStatsAftrUpdt->laneInfinity;
		}
	}
	else if (isQueuingBeforeUpdate != isQueuingAfterUpdate)
	{
		segStatsAftrUpdt->updateQueueStatus(laneAfterUpdate, ag);
	}

	// set the position of the last updated agent in his current lane (after update)
	segStatsAftrUpdt->setPositionOfLastUpdatedAgentInLane(ag->distanceToEndOfSegment, laneAfterUpdate);

	debugMsgs << "SegStatsAfterUpdate:" <<segAfterUpdate->getStart()->getID() << "Queuing count: "<<
			segStatsAftrUpdt->numQueueingInSegment(true)
			<<" Moving count: "<<segStatsAftrUpdt->numMovingInSegment(true)<<std::endl;
	std::cout << debugMsgs.str();
	debugMsgs.str("");

}

double sim_mob::Conflux::getSegmentSpeed(const RoadSegment* rdSeg, bool hasVehicle){
	if (hasVehicle){
		return findSegStats(rdSeg)->getSegSpeed(hasVehicle);
	}
	//else pedestrian lanes are not handled
	return 0.0;
}

void sim_mob::Conflux::initCandidateAgents() {
	candidateAgents.clear();
	resetCurrSegsOnUpLinks();

	sim_mob::Link* lnk = nullptr;
	const sim_mob::RoadSegment* rdSeg = nullptr;
	for (std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin(); i != upstreamSegmentsMap.end(); i++) {
		lnk = i->first;
		int count = 0;
		while (currSegsOnUpLinks.find(lnk) != currSegsOnUpLinks.end() && currSegsOnUpLinks.at(lnk)) {
			count ++;
			rdSeg = currSegsOnUpLinks.at(lnk);

			// To be removed
			if(!rdSeg){
				throw std::runtime_error("Road Segment NULL");
			}
			segmentAgents.at(rdSeg)->resetFrontalAgents();
			candidateAgents.insert(std::make_pair(rdSeg,segmentAgents.at(rdSeg)->getNext()));
			if(!candidateAgents.at(rdSeg)) {
				// this road segment is deserted. search the next (which is, technically, the previous).
				const std::vector<sim_mob::RoadSegment*> segments = i->second; // or upstreamSegmentsMap.at(lnk);
				std::vector<sim_mob::RoadSegment*>::const_iterator rdSegIt = std::find(segments.begin(), segments.end(), rdSeg);
				currSegsOnUpLinks.erase(lnk);
				if(rdSegIt != segments.begin()) {
					rdSegIt--;
					currSegsOnUpLinks.insert(std::make_pair(lnk, *rdSegIt));
				}
				else {
					currSegsOnUpLinks.erase(lnk);
					const sim_mob::RoadSegment* nullSeg = nullptr;
					currSegsOnUpLinks.insert(std::make_pair(lnk, nullSeg));
				}
			}
			else { break; }
		}
	}
}

std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > sim_mob::Conflux::getLanewiseAgentCounts(const sim_mob::RoadSegment* rdSeg) {
	return findSegStats(rdSeg)->getAgentCountsOnLanes();
}

unsigned int sim_mob::Conflux::numMovingInSegment(const sim_mob::RoadSegment* rdSeg, bool hasVehicle) {
	return findSegStats(rdSeg)->numMovingInSegment(hasVehicle);
}

void sim_mob::Conflux::resetCurrSegsOnUpLinks() {
	currSegsOnUpLinks.clear();
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin(); i != upstreamSegmentsMap.end(); i++) {
		currSegsOnUpLinks.insert(std::make_pair(i->first, i->second.back()));
	}
}

sim_mob::Agent* sim_mob::Conflux::agentClosestToIntersection() {
	sim_mob::Agent* ag = nullptr;
	const sim_mob::RoadSegment* agRdSeg = nullptr;
	double minDistance = std::numeric_limits<double>::max();
	std::map<const sim_mob::RoadSegment*, sim_mob::Agent*>::iterator i = candidateAgents.begin();
	while (i != candidateAgents.end()) {
		if (i->second != nullptr) {
			if (minDistance == (i->second->distanceToEndOfSegment + lengthsOfSegmentsAhead.at(i->first))) {
				// If current ag and (*i) are at equal distance to the stop line, we toss a coin and choose one of them
				bool coinTossResult = ((rand() / (double) RAND_MAX) < 0.5);
				if (coinTossResult) {
					agRdSeg = i->first;
					ag = i->second;
				}
			} else if (minDistance > (i->second->distanceToEndOfSegment + lengthsOfSegmentsAhead.at(i->first))) {
				minDistance = i->second->distanceToEndOfSegment + lengthsOfSegmentsAhead.at(i->first);
				agRdSeg = i->first;
				ag = i->second;
			}
		}
		i++;
	}
	if (ag) {
		candidateAgents.erase(agRdSeg);
		const std::vector<sim_mob::RoadSegment*> segments = agRdSeg->getLink()->getSegments();
		sim_mob::Agent* nextAg = segmentAgents.at(agRdSeg)->getNext();
		std::vector<sim_mob::RoadSegment*>::const_iterator rdSegIt = std::find(segments.begin(), segments.end(), agRdSeg);
		while (!nextAg && rdSegIt != segments.begin()) {
			rdSegIt--;
			nextAg = segmentAgents.at(*rdSegIt)->getNext();
		}
		candidateAgents.insert(std::make_pair(agRdSeg, nextAg));
	}
	return ag;
}

void sim_mob::Conflux::prepareLengthsOfSegmentsAhead() {
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin(); i != upstreamSegmentsMap.end(); i++)
	{
		for(std::vector<sim_mob::RoadSegment*>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
		{
			double lengthAhead = 0.0;
			for(std::vector<sim_mob::RoadSegment*>::const_iterator k = j+1; k != i->second.end(); k++)
			{
				lengthAhead = lengthAhead + (*k)->computeLaneZeroLength();
			}
			lengthsOfSegmentsAhead.insert(std::make_pair(*j, lengthAhead));
		}
	}
}

unsigned int sim_mob::Conflux::numQueueingInSegment(const sim_mob::RoadSegment* rdSeg, bool hasVehicle) {
	return findSegStats(rdSeg)->numQueueingInSegment(hasVehicle);
}

double sim_mob::Conflux::getOutputFlowRate(const Lane* lane) {
	return findSegStats(lane->getRoadSegment())->getLaneParams(lane)->getOutputFlowRate();
}

int sim_mob::Conflux::getOutputCounter(const Lane* lane) {
	return findSegStats(lane->getRoadSegment())->getLaneParams(lane)->getOutputCounter();
}

void sim_mob::Conflux::absorbAgentsAndUpdateCounts(sim_mob::SegmentStats* sourceSegStats) {
	//sourceSegStats is the downstream SegmentStats copy from an adjacent conflux.
	if(sourceSegStats->hasAgents()) {
		segmentAgents.at(sourceSegStats->getRoadSegment())->absorbAgents(sourceSegStats);
		std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > laneCounts = segmentAgents.at(sourceSegStats->getRoadSegment())->getAgentCountsOnLanes();
		sourceSegStats->setPrevTickLaneCountsFromOriginal(laneCounts);
		sourceSegStats->clear();
	}
}

double sim_mob::Conflux::getAcceptRate(const Lane* lane) {
	return findSegStats(lane->getRoadSegment())->getLaneParams(lane)->getAcceptRate();
}

void sim_mob::Conflux::updateSupplyStats(const Lane* lane, double newOutputFlowRate) {
	findSegStats(lane->getRoadSegment())->updateLaneParams(lane, newOutputFlowRate);
}

void sim_mob::Conflux::restoreSupplyStats(const Lane* lane) {
	findSegStats(lane->getRoadSegment())->restoreLaneParams(lane);
}

void sim_mob::Conflux::updateSupplyStats(timeslice frameNumber) {
	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it = segmentAgents.begin();
	for( ; it != segmentAgents.end(); ++it )
	{
		(it->second)->updateLaneParams(frameNumber);
		(it->second)->reportSegmentStats(frameNumber);
	}
}

std::pair<unsigned int, unsigned int> sim_mob::Conflux::getLaneAgentCounts(const sim_mob::Lane* lane) {
	return findSegStats(lane->getRoadSegment())->getLaneAgentCounts(lane);
}

unsigned int sim_mob::Conflux::getInitialQueueCount(const Lane* lane) {
	return findSegStats(lane->getRoadSegment())->getInitialQueueCount(lane);
}

void sim_mob::Conflux::killAgent(sim_mob::Agent* ag, const sim_mob::RoadSegment* prevRdSeg, const sim_mob::Lane* prevLane) {
	findSegStats(prevRdSeg)->removeAgent(prevLane, ag);
}

void sim_mob::Conflux::handoverDownstreamAgents() {
	for(std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator i = segmentAgentsDownstream.begin(); i != segmentAgentsDownstream.end(); i++)
	{
		i->first->getParentConflux()->absorbAgentsAndUpdateCounts(i->second);
	}
}

double sim_mob::Conflux::getLastAccept(const Lane* lane) {
	return findSegStats(lane->getRoadSegment())->getLaneParams(lane)->getLastAccept();
}

void sim_mob::Conflux::setLastAccept(const Lane* lane, double lastAcceptTime) {
	findSegStats(lane->getRoadSegment())->getLaneParams(lane)->setLastAccept(lastAcceptTime);
}

void sim_mob::Conflux::resetPositionOfLastUpdatedAgentOnLanes() {
	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it = segmentAgents.begin();
	for( ; it != segmentAgents.end(); ++it )
	{
		(it->second)->resetPositionOfLastUpdatedAgentOnLanes();
	}
}

sim_mob::SegmentStats* sim_mob::Conflux::findSegStats(const sim_mob::RoadSegment* rdSeg) {
	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it = segmentAgents.find(rdSeg);
	if(it == segmentAgents.end()){
		it = segmentAgentsDownstream.find(rdSeg);
	}
	return it->second;
}
