/*
 * Conflux.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: harish
 */

#include<map>
#include "Conflux.hpp"
using namespace sim_mob;
typedef Entity::UpdateStatus UpdateStatus;

void sim_mob::Conflux::addStartingAgent(sim_mob::Agent* ag, sim_mob::RoadSegment* rdSeg) {
	/**
	 * The agents always start at a node (for now), i.e. the start of a road segment.
	 * So for now, we will always add the agent to the road segment in "lane infinity".
	 */
	sim_mob::SegmentStats* rdSegStats = segmentAgents.at(rdSeg);
	rdSegStats->laneInfinity.push(ag);
}

void sim_mob::Conflux::addAgent(sim_mob::Agent* ag) {
	segmentAgents[ag->getCurrSegment()]->laneInfinity.push(ag);
}

UpdateStatus sim_mob::Conflux::update(timeslice frameNumber) {
	currFrameNumber = frameNumber;

	if (sim_mob::StreetDirectory::instance().signalAt(*multiNode) != nullptr) {
		updateUnsignalized(frameNumber); //TODO: Update Signalized must be implemented
	}
	else {
		updateUnsignalized(frameNumber);
	}
	updateSupplyStats(frameNumber);

	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it_sa=segmentAgents.begin();
	while((it_sa) != segmentAgents.end()){
		std::map<sim_mob::Lane*, std::pair<unsigned int, unsigned int> > counts = (*it_sa).second->getAgentCountsOnLanes();
		std::map<sim_mob::Lane*, std::pair<unsigned int, unsigned int> >::iterator i = counts.begin();
		while(i != counts.end()) {
			std::cout << multiNode->getID()<<": "<< (*it_sa).second << ": " << (*i).second.second << std::endl;
			i++;
		}
		it_sa++;
	}

	UpdateStatus retVal(UpdateStatus::RS_CONTINUE); //always return continue. Confluxes never die.
	return retVal;
}

void sim_mob::Conflux::updateSignalized() {
}

void sim_mob::Conflux::updateUnsignalized(timeslice frameNumber) {
//	debugMsgs << "\nUpdate " << frameNumber << " - Multinode :" << getMultiNode()->getID() << std::endl;
	initCandidateAgents();
	for(std::map<const sim_mob::RoadSegment*, sim_mob::Agent* >::iterator i = candidateAgents.begin();
			i != candidateAgents.end(); i++) {
		debugMsgs << "candidateAgent[start:" << (*i).first->getStart()->getID()
				<< ", end: " << (*i).first->getEnd()->getID() << "]: Agent : "
				<< (((*i).second == nullptr)?  0 : (*i).second->getId())
				<< std::endl;
	}
	sim_mob::Agent* ag = agentClosestToIntersection();
	while (ag) {
		updateAgent(ag);
		// get next agent to update
		ag = agentClosestToIntersection();
	}
	std::cout << debugMsgs.str();
	debugMsgs.str(std::string());
}

void sim_mob::Conflux::updateAgent(sim_mob::Agent* ag) {
	const sim_mob::RoadSegment* segBeforeUpdate = ag->getCurrSegment();
	const sim_mob::Lane* laneBeforeUpdate = ag->getCurrLane();
	UpdateStatus res = ag->update(currFrameNumber);
	if (res.status == UpdateStatus::RS_DONE) {
		//This Entity is done; schedule for deletion.
		parentWorker->scheduleForRemoval(ag);
	} else if (res.status == UpdateStatus::RS_CONTINUE) {
		// TODO: I think there will be nothing here. Have to make sure. ~ Harish
	} else {
		throw std::runtime_error("Unknown/unexpected update() return status.");
	}

	const sim_mob::RoadSegment* segAfterUpdate = ag->getCurrSegment();
	const sim_mob::Lane* laneAfterUpdate = ag->getCurrLane();
	if((segBeforeUpdate != segAfterUpdate) || ( !laneBeforeUpdate)) {
		segmentAgents[segBeforeUpdate]->dequeue(laneBeforeUpdate);
		if(segmentAgents.find(segAfterUpdate) != segmentAgents.end()) {
			segmentAgents[segAfterUpdate]->addAgent(laneAfterUpdate, ag);
		}
		else if (segmentAgentsDownstream.find(segAfterUpdate) != segmentAgentsDownstream.end()) {
			segmentAgentsDownstream[segAfterUpdate]->addAgent(laneAfterUpdate, ag);
		}
	}
}

double sim_mob::Conflux::getSegmentSpeed(const RoadSegment* rdSeg, bool hasVehicle){
	if (hasVehicle)
		return getSegmentAgents()[rdSeg]->getSegSpeed(hasVehicle);
	else
		return getSegmentAgents()[rdSeg]->getSegSpeed(hasVehicle);
}

void sim_mob::Conflux::initCandidateAgents() {
	resetCurrSegsOnUpLinks();
	typedef std::map<sim_mob::Link*, std::vector<sim_mob::RoadSegment*>::const_reverse_iterator >::iterator currSegsOnUpLinksIt;
	sim_mob::Link* lnk = nullptr;
	const sim_mob::RoadSegment* rdSeg = nullptr;
	for (currSegsOnUpLinksIt i = currSegsOnUpLinks.begin(); i != currSegsOnUpLinks.end(); i++) {
		lnk = i->first;
		if(i->second != upstreamSegmentsMap[lnk].rend()){
			do {
				rdSeg = *(i->second);
				if(rdSeg == 0){
					throw std::runtime_error("Road Segment NULL");
				}
				segmentAgents[rdSeg]->resetFrontalAgents();
				candidateAgents[rdSeg] = segmentAgents[rdSeg]->getNext();
				debugMsgs << std::endl << "candidateAgents[" << rdSeg->getStart()->getID() << "] = null? " << ((candidateAgents[rdSeg] == nullptr)? "yes" : "no");
				std::cout << debugMsgs.str() << std::endl;
				debugMsgs.clear();
				if(candidateAgents[rdSeg] == nullptr) {
					// this road segment is deserted. search the next (which is technically the previous).
					currSegsOnUpLinks[lnk]++;
				}
			} while (currSegsOnUpLinks[lnk] != upstreamSegmentsMap[lnk].rend() && candidateAgents[rdSeg] == nullptr);
		}
	}
}

void sim_mob::Conflux::prepareAgentForHandover(sim_mob::Agent* ag) {
	throw std::runtime_error("Conflux::prepareAgentForHandover() not implemented");
}

std::map<sim_mob::Lane*, std::pair<unsigned int, unsigned int> > sim_mob::Conflux::getLanewiseAgentCounts(const sim_mob::RoadSegment* rdSeg) {
	return segmentAgents[rdSeg]->getAgentCountsOnLanes();
}

unsigned int sim_mob::Conflux::numMovingInSegment(const sim_mob::RoadSegment* rdSeg, bool hasVehicle) {
	return segmentAgents[rdSeg]->numMovingInSegment(hasVehicle);
}

void sim_mob::Conflux::resetCurrSegsOnUpLinks() {
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin();
			i != upstreamSegmentsMap.end(); i++) {
		currSegsOnUpLinks.insert(std::make_pair((*i).first, (*i).second.rbegin()));
	}

}

sim_mob::Agent* sim_mob::Conflux::agentClosestToIntersection() {
	std::map<const sim_mob::RoadSegment*, sim_mob::Agent* >::iterator i = candidateAgents.begin();
	sim_mob::Agent* ag = nullptr;
	const sim_mob::RoadSegment* agRdSeg = nullptr;
	double minDistance = std::numeric_limits<double>::max();
	while(i != candidateAgents.end()) {
		if((*i).second != nullptr) {
			if(minDistance == ((*i).second->distanceToEndOfSegment + lengthsOfSegmentsAhead[(*i).first])) {
				// If current ag and (*i) are at equal distance to the stop line, we toss a coin and choose one of them
				bool coinTossResult = ((rand() / (double)RAND_MAX) < 0.5);
				if(coinTossResult) {
					agRdSeg = (*i).first;
					ag = (*i).second;
				}
			}
			else if (minDistance > ((*i).second->distanceToEndOfSegment + lengthsOfSegmentsAhead[(*i).first])) {
				minDistance = (*i).second->distanceToEndOfSegment + lengthsOfSegmentsAhead[(*i).first];
				agRdSeg = (*i).first;
				ag = (*i).second;
			}
		}
		i++;
	}
	if(ag) {
		candidateAgents[agRdSeg] = segmentAgents[agRdSeg]->getNext();
	}
	/*if(ag){
		debugMsgs << "Agent returned is: " << ag->getId() << std::endl;
	}*/
	return ag;
}

void sim_mob::Conflux::prepareLengthsOfSegmentsAhead() {
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin();
				i != upstreamSegmentsMap.end(); i++)
	{
		for(std::vector<sim_mob::RoadSegment*>::const_iterator j = (*i).second.begin();
				j != (*i).second.end(); j++)
		{
			double lengthAhead = 0.0;
			for(std::vector<sim_mob::RoadSegment*>::const_iterator k = j+1;
					k != (*i).second.end(); k++)
			{
				lengthAhead = lengthAhead + (*k)->computeLaneZeroLength();
			}
			lengthsOfSegmentsAhead.insert(std::make_pair(*j, lengthAhead));
		}
	}
}

unsigned int sim_mob::Conflux::numQueueingInSegment(const sim_mob::RoadSegment* rdSeg,
		bool hasVehicle) {
	std::cout << "rdSeg in Conflux: "<<rdSeg <<std::endl;
	return segmentAgents[rdSeg]->numQueueingInSegment(hasVehicle);
}

double sim_mob::Conflux::getOutputFlowRate(const Lane* lane) {
	return segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getOutputFlowRate();
}

int sim_mob::Conflux::getOutputCounter(const Lane* lane) {
	return segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getOutputCounter();
}

void sim_mob::Conflux::absorbAgentsAndUpdateCounts(sim_mob::SegmentStats* segStats) {
	segmentAgents[segStats->getRoadSegment()]->absorbAgents(segStats);
	std::map<sim_mob::Lane*, std::pair<unsigned int, unsigned int> > laneCounts = segmentAgents[segStats->getRoadSegment()]->getAgentCountsOnLanes();
	segStats->setPrevTickLaneCountsFromOriginal(laneCounts);
}

double sim_mob::Conflux::getAcceptRate(const Lane* lane) {
	return segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getAcceptRate();
}

void sim_mob::Conflux::updateSupplyStats(const Lane* lane, double newOutputFlowRate) {
	segmentAgents[lane->getRoadSegment()]->updateLaneParams(lane, newOutputFlowRate);
}

void sim_mob::Conflux::restoreSupplyStats(const Lane* lane) {
	segmentAgents[lane->getRoadSegment()]->restoreLaneParams(lane);
}

void sim_mob::Conflux::updateSupplyStats(timeslice frameNumber) {
	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it = segmentAgents.begin();
	for( ; it != segmentAgents.end(); ++it )
	{
		(it->second)->updateLaneParams(frameNumber);
		(it->second)->reportSegmentStats(frameNumber);
	}
}

std::pair<unsigned int, unsigned int> sim_mob::Conflux::getLaneAgentCounts(
		const sim_mob::Lane* lane) {
	return segmentAgents[lane->getRoadSegment()]->getLaneAgentCounts(lane);
}

unsigned int sim_mob::Conflux::getInitialQueueCount(const Lane* lane) {
	return segmentAgents[lane->getRoadSegment()]->getInitialQueueCount(lane);
}