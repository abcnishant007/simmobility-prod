/*
 * TaxiDriverFacets.cpp
 *
 *  Created on: 5 Nov 2016
 *      Author: jabir
 */

#include <entities/roles/driver/TaxiDriverFacets.hpp>
#include "TaxiDriver.hpp"
#include "path/PathSetManager.hpp"
#include "geospatial/network/RoadNetwork.hpp"
#include "config/MT_Config.hpp"
#include "entities/TaxiStandAgent.hpp"
#include "entities/misc/TaxiTrip.hpp"

namespace sim_mob
{

namespace medium
{

/**define the threshold for long time waiting, timeout is 15 minutes in seconds*/
const double timeoutForLongWaiting = 15*60;

TaxiDriverMovement::TaxiDriverMovement()
{
}

TaxiDriverMovement::~TaxiDriverMovement()
{
}

void TaxiDriverMovement::frame_init()
{
	Vehicle* newVeh = new Vehicle(Vehicle::TAXI, sim_mob::TAXI_LENGTH);
	parentTaxiDriver->setResource(newVeh);
	parentTaxiDriver->taxiDriverMode = CRUISE;
	parentTaxiDriver->driverMode = CRUISE;
	parentDriver->driverMode = CRUISE;
	assignFirstNode();
	selectNextNodeAndLinksWhileCruising();
}

void TaxiDriverMovement::setParentTaxiDriver(TaxiDriver * taxiDriver)
{
	parentTaxiDriver = taxiDriver;
}

void TaxiDriverMovement::assignFirstNode()
{
	const sim_mob::RoadNetwork *roadNetwork = sim_mob::RoadNetwork::getInstance();
	const std::map<unsigned int, Node *>& mapOfIdsVsNode = roadNetwork->getMapOfIdvsNodes();
	Node *firstNode = roadNetwork->getFirstNode();
	const std::vector<TripChainItem *>&tripChain = parentDriver->getParent()->getTripChain();
	const TaxiTrip *taxiTrip = dynamic_cast<const TaxiTrip*>(tripChain[0]);
	originNode = const_cast<Node*>(taxiTrip->origin.node);
}

const Lane* TaxiDriverMovement::getBestTargetLane(const SegmentStats* nextSegStats,const SegmentStats* nextToNextSegStats)
{
	DriverMode mode = parentTaxiDriver->getDriverMode();
	if (mode == DRIVE_TO_TAXISTAND && destinationTaxiStand && nextSegStats->hasTaxiStand(destinationTaxiStand))
	{
		return nextSegStats->getOutermostLane();
	}
	else
	{
		const Lane* minLane = nullptr;
		double minQueueLength = std::numeric_limits<double>::max();
		double minLength = std::numeric_limits<double>::max();
		double que = 0.0;
		double total = 0.0;
		const Link* nextLink = getNextLinkForLaneChoice(nextSegStats);
		const std::vector<Lane*>& lanes = nextSegStats->getRoadSegment()->getLanes();
		if (!minLane)
		{
			for (vector<Lane*>::const_iterator lnIt = lanes.begin();lnIt != lanes.end(); ++lnIt)
			{
				if (!((*lnIt)->isPedestrianLane()))
				{
					const Lane* lane = *lnIt;
					total = nextSegStats->getLaneTotalVehicleLength(lane);
					que = nextSegStats->getLaneQueueLength(lane);
					if (minLength > total)
					{
						//if total length of vehicles is less than current minLength
						minLength = total;
						minQueueLength = que;
						minLane = lane;
					} else if (minLength == total)
					{
						//if total length of vehicles is equal to current minLength
						if (minQueueLength > que)
						{
							//and if the queue length is less than current minQueueLength
							minQueueLength = que;
							minLane = lane;
						}
					}
				}
			}
		}
		return minLane;
	}
}

void TaxiDriverMovement::setCurrentNode(const Node *node)
{
	currentNode = node;
}

const Node* TaxiDriverMovement::getCurrentNode()
{
	return currentNode;
}

void TaxiDriverMovement::setDestinationNode(const Node *node)
{
	destinationNode = node;
}

const Node* TaxiDriverMovement::getDestinationNode()
{
	return destinationNode;
}

bool TaxiDriverMovement::moveToNextSegment(DriverUpdateParams& params)
{

	const SegmentStats* currSegStat = pathMover.getCurrSegStats();
	const SegmentStats* nxtSegStat = pathMover.getNextSegStats(false);
	bool res = false;

	/*sim_mob::BasicLogger& ptMoveLogger = sim_mob::Logger::log("DriversMovePath.csv");
	ptMoveLogger<<parentTaxiDriver->parent->getDatabaseId()<<","<<currSegStat->getRoadSegment()->getRoadSegmentId()<<"-"<<currSegStat->getStatsNumberInSegment()<<","<<currSegStat->getRoadSegment()->getLinkId();
	if(nxtSegStat){
		ptMoveLogger<<","<<nxtSegStat->getRoadSegment()->getRoadSegmentId()<<"-"<<nxtSegStat->getStatsNumberInSegment()<<","<<nxtSegStat->getRoadSegment()->getLinkId()<<",";
	} else {
		ptMoveLogger<<","<<"0"<<","<<"0"<<",";
	}
	ptMoveLogger<<parentTaxiDriver->parent->distanceToEndOfSegment<<",";
	ptMoveLogger<<DailyTime(params.now.ms()).getStrRepr()<<","<<parentTaxiDriver->getDriverMode()<<","<<parentTaxiDriver->taxiPassenger<<std::endl;*/

	/*parentTaxiDriver->parent->routeStr<<parentTaxiDriver->parent->getDatabaseId()<<",";
	parentTaxiDriver->parent->routeStr<<DailyTime(params.now.ms()).getStrRepr()<<",";
	parentTaxiDriver->parent->routeStr<<currSegStat->getRoadSegment()->getRoadSegmentId()<<"-"<<currSegStat->getStatsNumberInSegment()<<",";
	parentTaxiDriver->parent->routeStr<<currSegStat->getRoadSegment()->getLinkId()<<",";
	parentTaxiDriver->parent->routeStr<<"before nodes"<<",";
	if(this->currentNode)
		parentTaxiDriver->parent->routeStr<<this->currentNode->getNodeId()<<",";
	else
		parentTaxiDriver->parent->routeStr<<"0"<<",";
	if(this->destinationNode)
		parentTaxiDriver->parent->routeStr<<this->destinationNode->getNodeId()<<",";
	else
		parentTaxiDriver->parent->routeStr<<"0"<<",";

	parentTaxiDriver->parent->routeStr<<"before conflux,"<<currSegStat->getParentConflux()->getConfluxNode()->getNodeId()<<","<<parentTaxiDriver->getDriverMode()<<std::endl;
	*/

	if (parentTaxiDriver->getDriverMode() == DRIVE_TO_TAXISTAND)
	{
		const SegmentStats* currSegStat = pathMover.getCurrSegStats();
		if (destinationTaxiStand && currSegStat->hasTaxiStand(destinationTaxiStand))
		{
			TaxiStandAgent* taxiStandAgent = TaxiStandAgent::getTaxiStandAgent(destinationTaxiStand);
			if (taxiStandAgent)
			{
				bool canAccomodate = taxiStandAgent->acceptTaxiDriver(parentTaxiDriver->parent);
				if (canAccomodate)
				{
					//add to taxi stand
					if (parentTaxiDriver->getResource()->isMoving())
					{
						if (isQueuing)
						{
							removeFromQueue();
						}
					}
					parentTaxiDriver->setTaxiDriveMode(QUEUING_AT_TAXISTAND);
					parentTaxiDriver->getResource()->setMoving(false);
					params.elapsedSeconds = params.secondsInTick;
					parentTaxiDriver->parent->setRemainingTimeThisTick(0.0);
					toBeRemovedFromTaxiStand = false;
					return res;
				}
				else
				{
					setCruisingMode();
					return moveToNextSegment(params);
				}
			}
			return res;
		}
	}

	if (parentTaxiDriver->getDriverMode() == CRUISE)
	{
		if(cruisingTooLongTime > timeoutForLongWaiting)
		{
			cruisingTooLongTime = 0.0;
			const PolyPoint point = currSegStat->getRoadSegment()->getPolyLine()->getLastPoint();
			destinationTaxiStand = TaxiStand::allTaxiStandMap.searchNearestObject(point.getX(), point.getY());
			if(destinationTaxiStand)
			{
				driveToTaxiStand();
			}
		}
		else if (pathMover.isEndOfPath())
		{
			Conflux *parentConflux = currSegStat->getParentConflux();
			parentTaxiDriver->pickUpPassngerAtNode(parentConflux);
			if (parentTaxiDriver->getPassenger() == nullptr)
			{
				selectNextNodeAndLinksWhileCruising();
			}
		}
	}
	else if (parentTaxiDriver->getDriverMode() == DRIVE_WITH_PASSENGER && pathMover.isEndOfPath())
	{
		parentTaxiDriver->alightPassenger();
		parentTaxiDriver->taxiDriverMode = CRUISE;
		selectNextNodeAndLinksWhileCruising();
	}

	res = DriverMovement::moveToNextSegment(params);

	/*parentTaxiDriver->parent->routeStr<<parentTaxiDriver->parent->getDatabaseId()<<",";
	parentTaxiDriver->parent->routeStr<<DailyTime(params.now.ms()).getStrRepr()<<",";
	parentTaxiDriver->parent->routeStr<<currSegStat->getRoadSegment()->getRoadSegmentId()<<"-"<<currSegStat->getStatsNumberInSegment()<<",";
	parentTaxiDriver->parent->routeStr<<currSegStat->getRoadSegment()->getLinkId()<<",";
	parentTaxiDriver->parent->routeStr<<"after nodes"<<",";
	if(this->currentNode)
		parentTaxiDriver->parent->routeStr<<this->currentNode->getNodeId()<<",";
	else
		parentTaxiDriver->parent->routeStr<<"0"<<",";
	if(this->destinationNode)
		parentTaxiDriver->parent->routeStr<<this->destinationNode->getNodeId()<<",";
	else
		parentTaxiDriver->parent->routeStr<<"0"<<",";

	parentTaxiDriver->parent->routeStr<<"after conflux,"<<currSegStat->getParentConflux()->getConfluxNode()->getNodeId()<<","<<parentTaxiDriver->getDriverMode()<<std::endl;
	*/

	return res;
}

void TaxiDriverMovement::frame_tick()
{
	DriverUpdateParams& params = parentTaxiDriver->getParams();
	const DriverMode &mode = parentTaxiDriver->getDriverMode();
	if (parentTaxiDriver->taxiPassenger != nullptr)
	{
		parentTaxiDriver->taxiPassenger->Movement()->frame_tick();
	}

	if( mode == CRUISE)
	{
		cruisingTooLongTime += params.secondsInTick;
	}
	if (mode == QUEUING_AT_TAXISTAND)
	{
		queuingTooLongTime += params.secondsInTick;
		TaxiStandAgent *agent = TaxiStandAgent::getTaxiStandAgent(destinationTaxiStand);
		if (agent->isTaxiFirstInQueue(parentTaxiDriver))
		{
			Person_MT * personPickedUp = agent->pickupOneWaitingPerson();
			if (personPickedUp != nullptr)
			{
				std::string id = personPickedUp->getDatabaseId();
				Role<Person_MT>* curRole = personPickedUp->getRole();
				sim_mob::medium::Passenger* passenger = dynamic_cast<sim_mob::medium::Passenger*>(curRole);
				if (passenger)
				{
					std::vector<SubTrip>::iterator subTripItr = personPickedUp->currSubTrip;
					WayPoint personTravelDestination = (*subTripItr).destination;
					const Node * personDestinationNode = personTravelDestination.node;
					std::vector<WayPoint> currentRouteChoice;
					currentNode = destinationNode;
					if (currentNode != personDestinationNode)
					{
						const Link* link = destinationTaxiStand->getRoadSegment()->getParentLink();
						SubTrip currSubTrip;
						currSubTrip.origin = WayPoint(link->getFromNode());
						currSubTrip.destination = WayPoint(personDestinationNode);
						currentRouteChoice = PrivateTrafficRouteChoice::getInstance()->getPath(currSubTrip, false, link, parentTaxiDriver->parent->usesInSimulationTravelTime());
						if(currentRouteChoice.size()==0)
						{
							currentRouteChoice = PrivateTrafficRouteChoice::getInstance()->getPath(currSubTrip, false, nullptr, parentTaxiDriver->parent->usesInSimulationTravelTime());
							if(currentRouteChoice.size()>0)
							{
								const Link* destLink = nullptr;
								if(currentRouteChoice.front().type==WayPoint::LINK)
								{
									destLink = currentRouteChoice.front().link;
								}
								if(destLink)
								{
									vector<WayPoint> path =	StreetDirectory::Instance().SearchShortestDrivingPath<Link, Link>(*link,*destLink);
									vector<WayPoint> pathOfLinks;
									for (vector<WayPoint>::iterator itWayPts =path.begin();itWayPts != path.end();	++itWayPts) {
										if (itWayPts->type == WayPoint::LINK) {
											pathOfLinks.push_back(*itWayPts);
										}
									}
									if(pathOfLinks.size()>0)
									{
										const Link* lastLink = pathOfLinks.back().link;
										pathOfLinks.erase(pathOfLinks.end()-1);
										currentRouteChoice.insert(currentRouteChoice.begin(),pathOfLinks.begin(),pathOfLinks.end());
									}
									else
									{
										currentRouteChoice.clear();
									}
								}
							}
							if (currentRouteChoice.size() > 0)
							{
								bool isAdded = parentTaxiDriver->addPassenger(passenger);
								if (isAdded)
								{
									destinationNode = personDestinationNode;
									setCurrentNode(currentNode);
									setDestinationNode(destinationNode);
									addTaxiStandPath(currentRouteChoice);
									passenger->setStartPoint(WayPoint(destinationTaxiStand));
									passenger->setEndPoint(WayPoint(destinationNode));
									//passenger->setService(currentRouteChoice);
									parentTaxiDriver->setTaxiDriveMode(DRIVE_WITH_PASSENGER);
									parentTaxiDriver->getResource()->setMoving(true);
									toBeRemovedFromTaxiStand = true;
									previousTaxiStand = destinationTaxiStand;
									destinationTaxiStand = nullptr;
									isQueuingTaxiStand = false;
								}
							}
							else
							{
								sim_mob::BasicLogger& ptMoveLogger = sim_mob::Logger::log("nopathAfterPickupInStand.csv");
								ptMoveLogger << passenger->getParent()->getDatabaseId()<<",";
								ptMoveLogger << link->getLinkId()<<",";
								ptMoveLogger << destinationTaxiStand->getRoadSegmentId()<<",";
								ptMoveLogger << link->getFromNode()->getNodeId()<<",";
								ptMoveLogger << personDestinationNode->getNodeId()<<std::endl;
							}
						}
					}
				}
			}
			else if(queuingTooLongTime>timeoutForLongWaiting)
			{
				setCruisingMode();
				parentTaxiDriver->getResource()->setMoving(true);
				toBeRemovedFromTaxiStand = true;
				isQueuingTaxiStand = true;
			}
		}
		params.elapsedSeconds = params.secondsInTick;
		parentTaxiDriver->parent->setRemainingTimeThisTick(0.0);
	}
	else if (mode == CRUISE || mode == DRIVE_WITH_PASSENGER || mode == DRIVE_TO_TAXISTAND)
	{
		DriverMovement::frame_tick();
	}
}

void TaxiDriverMovement::addCruisingPath(const Link* selectedLink)
{
	MT_Config& mtCfg = MT_Config::getInstance();
	std::vector<const SegmentStats*> path = getMesoPathMover().getPath();
	std::vector<RoadSegment *> currentRoute;
	currentRoute.insert(currentRoute.end(),selectedLink->getRoadSegments().begin(),selectedLink->getRoadSegments().end());
	const Conflux* confluxForLnk = mtCfg.getConfluxForNode(destinationNode);
	if (!isPathInitialized)
	{
		path.erase(path.begin(), path.end());
		for (std::vector<RoadSegment *>::iterator itr = currentRoute.begin();itr != currentRoute.end(); itr++)
		{
			const vector<SegmentStats*>& statsInSegment = confluxForLnk->findSegStats(*itr);
			path.insert(path.end(), statsInSegment.begin(),statsInSegment.end());
		}
		pathMover.setPath(path);
		const SegmentStats* firstSegStat = path.front();
		parentTaxiDriver->parent->setCurrSegStats(firstSegStat);
		parentTaxiDriver->parent->setCurrLane(firstSegStat->laneInfinity);
		parentTaxiDriver->parent->distanceToEndOfSegment = firstSegStat->getLength();
		isPathInitialized = true;
	}
	else
	{
		if (path.size() > 0)
		{
			path.erase(path.begin(), path.end() - 1);
		}

		for (std::vector<RoadSegment *>::iterator itr = currentRoute.begin();itr != currentRoute.end(); itr++)
		{
			const vector<SegmentStats*>& statsInSegment = confluxForLnk->findSegStats(*itr);
			path.insert(path.end(), statsInSegment.begin(),statsInSegment.end());
		}

		const SegmentStats* currSegStats = pathMover.getCurrSegStats();
		pathMover.setPath(path);
		pathMover.setSegmentStatIterator(currSegStats);
	}
}

TaxiDriver* TaxiDriverMovement::getParentDriver()
{
	return parentTaxiDriver;
}
void TaxiDriverMovement::selectNextNodeAndLinksWhileCruising()
{
	if (!currentNode)
	{
		currentNode = originNode;
	}
	else
	{
		currentNode = destinationNode;
	}

	const SegmentStats* currSegStats = parentTaxiDriver->parent->getCurrSegStats();
	if (currSegStats)
	{
		const RoadSegment *currentRoadSegment = currSegStats->getRoadSegment();
		if (currentRoadSegment)
		{
			const Link * currLink = currentRoadSegment->getParentLink();
			if (currLink)
			{
				if (mapOfLinksAndVisitedCounts.find(currLink) == mapOfLinksAndVisitedCounts.end())
				{
					mapOfLinksAndVisitedCounts[currLink] = 1;
				}
				else
				{
					mapOfLinksAndVisitedCounts[currLink] = mapOfLinksAndVisitedCounts[currLink] + 1;
				}
			}
		}
	}

	std::stringstream msg;
	selectedNextLinkInCrusing = nullptr;
	std::vector<Node*> nodeVector = currentNode->getNeighbouringNodes();
	if (nodeVector.size() > 0)
	{
		std::vector<Node*>::iterator itr = nodeVector.begin();
		Node *destNode = (*(itr));
		std::map<unsigned int, Link*> mapOfDownStreamLinks = currentNode->getDownStreamLinks();
		std::map<unsigned int, Link*>::iterator linkItr = mapOfDownStreamLinks.begin();
		const SegmentStats* currSegStats = pathMover.getCurrSegStats();
		/*int minNumOfVisits = -1;
		for(auto itLink=mapOfDownStreamLinks.begin();itLink!=mapOfDownStreamLinks.end(); itLink++)
		{
			if(currSegStats)
			{
				std::map<const Link*, std::vector<LaneStats*> >::const_iterator lnGpIt = currSegStats->getLaneGroup().find(itLink->second);
				if (lnGpIt == currSegStats->getLaneGroup().end())
				{
					msg << "no connect, "<<currSegStats->getRoadSegment()->getRoadSegmentId()<<","<<currSegStats->getRoadSegment()->getLinkId()<<","<<itLink->second->getLinkId()<<std::endl;
					continue;
				}
			}
			if (mapOfLinksAndVisitedCounts.find(itLink->second)== mapOfLinksAndVisitedCounts.end())
			{
				selectedNextLinkInCrusing = itLink->second;
				break;
			}
			else if (minNumOfVisits == -1|| mapOfLinksAndVisitedCounts[itLink->second] < minNumOfVisits)
			{
				minNumOfVisits = mapOfLinksAndVisitedCounts[itLink->second];
				selectedNextLinkInCrusing = itLink->second;
			}
		}*/
		int minNumOfVisits = -1;
		const Lane *currLane = getCurrentlane();
		const RoadNetwork* rdNetwork = RoadNetwork::getInstance();
		const std::map<const Lane*, std::map<const Lane*, const TurningPath *>>&turningPathsFromLanes = rdNetwork->getTurningPathsFromLanes();
		std::map<const Lane*, std::map<const Lane*, const TurningPath *>>::const_iterator turningPathsLaneitr = turningPathsFromLanes.find(currLane);
		if (!currLane || turningPathsLaneitr != turningPathsFromLanes.end())
		{
			while (linkItr != mapOfDownStreamLinks.end())
			{
				Link *link = (*linkItr).second;
				if (link->getFromNode() == link->getToNode())
				{
					linkItr++;
					continue;
				}
				if (currLane)
				{
					std::map<const Lane*, const TurningPath *> mapOfLaneTurningPath = turningPathsLaneitr->second;
					const std::vector<RoadSegment*>& rdSegments = link->getRoadSegments();
					const RoadSegment* rdSegment = rdSegments.front();
					const std::vector<Lane*> &segLanes = rdSegment->getLanes();
					bool foundTurningPath = false;
					for (std::vector<Lane*>::const_iterator laneItr = segLanes.begin(); laneItr != segLanes.end();laneItr++)
					{
						if (mapOfLaneTurningPath.find(*laneItr)!= mapOfLaneTurningPath.end())
						{
							foundTurningPath = true;
							break;
						}
					}

					if (foundTurningPath == false)
					{
						linkItr++;
						continue;
					}
				}

				if (mapOfLinksAndVisitedCounts.find(link)== mapOfLinksAndVisitedCounts.end())
				{
					selectedNextLinkInCrusing = link;
					break;
				}
				else if (minNumOfVisits == -1|| mapOfLinksAndVisitedCounts[link] < minNumOfVisits)
				{
					minNumOfVisits = mapOfLinksAndVisitedCounts[link];
					selectedNextLinkInCrusing = link;
				}
				linkItr++;
			}
		}
	}

	if (selectedNextLinkInCrusing)
	{
		destinationNode = selectedNextLinkInCrusing->getToNode();
		addCruisingPath(selectedNextLinkInCrusing);
	}

	if(!selectedNextLinkInCrusing){
		sim_mob::BasicLogger& ptMoveLogger = sim_mob::Logger::log("nopathInCruising.csv");
		const SegmentStats* currentStats = getMesoPathMover().getCurrSegStats();
		if(currentStats){
			ptMoveLogger << parentTaxiDriver->parent->getDatabaseId()<<",";
			ptMoveLogger << currentStats->getRoadSegment()->getLinkId()<<",";
			ptMoveLogger << currentStats->getRoadSegment()->getRoadSegmentId()<<",";
			ptMoveLogger << currentNode->getNodeId()<<std::endl;
		}
	}
}

void TaxiDriverMovement::addRouteChoicePath(std::vector<WayPoint> &currentRouteChoice)
{
	std::vector<const SegmentStats*> path = getMesoPathMover().getPath();
	if (!isPathInitialized)
	{
		path.erase(path.begin(), path.end());
	}
	else
	{
		if (path.size() > 0)
		{
			path.erase(path.begin(), path.end() - 1);
		}
	}
	for (std::vector<WayPoint>::const_iterator itr = currentRouteChoice.begin();itr != currentRouteChoice.end(); itr++)
	{
		if((*itr).type==WayPoint::LINK){
			const Link *link = (*itr).link;
			Node *toNode = link->getToNode();
			Conflux *nodeConflux = Conflux::getConfluxFromNode(toNode);
			const std::vector<RoadSegment*>& roadSegments = link->getRoadSegments();
			for (std::vector<RoadSegment*>::const_iterator segItr =	roadSegments.begin(); segItr != roadSegments.end(); segItr++)
			{
				const std::vector<SegmentStats*>& segStatsVector = nodeConflux->findSegStats(*segItr);
				path.insert(path.end(), segStatsVector.begin(),	segStatsVector.end());
			}
		}
	}

	if (!isPathInitialized)
	{
		const SegmentStats* firstSegStat = path.front();
		pathMover.setPath(path);
		parentTaxiDriver->parent->setCurrSegStats(firstSegStat);
		parentTaxiDriver->parent->setCurrLane(firstSegStat->laneInfinity);
		parentTaxiDriver->parent->distanceToEndOfSegment =firstSegStat->getLength();
		isPathInitialized = true;
	}
	else
	{
		const SegmentStats* currSegStats = pathMover.getCurrSegStats();
		pathMover.setPath(path);
		pathMover.setSegmentStatIterator(currSegStats);
	}
}

void TaxiDriverMovement::setCruisingMode()
{
	previousTaxiStand = destinationTaxiStand;
	destinationTaxiStand = nullptr;
	parentTaxiDriver->taxiDriverMode = CRUISE;
	parentDriver->driverMode = CRUISE;
	if(pathMover.isEndOfPath()){
		selectNextNodeAndLinksWhileCruising();
	}
}

void TaxiDriverMovement::driveToTaxiStand()
{
	bool useInSimulationTT = parentTaxiDriver->getParent()->usesInSimulationTravelTime();
	const RoadSegment *taxiStandSegment = destinationTaxiStand->getRoadSegment();
	const Link *taxiStandLink = taxiStandSegment->getParentLink();
	//get the current segment
	const SegmentStats* currSegStat = pathMover.getCurrSegStats();
	const Link *currSegmentParentLink = currSegStat->getRoadSegment()->getParentLink();
	Node *originNode = currSegmentParentLink->getFromNode();
	Node *destForRouteChoice = taxiStandLink->getToNode();
	//set origin and destination node
	SubTrip currSubTrip;
	currSubTrip.origin = WayPoint(originNode);
	currSubTrip.destination = WayPoint(destForRouteChoice);
	vector<WayPoint> routeToTaxiStand = PrivateTrafficRouteChoice::getInstance()->getPathWhereToStand(currSubTrip, false, currSegmentParentLink, nullptr, taxiStandLink, useInSimulationTT);
	if(routeToTaxiStand.empty()||currSegmentParentLink==taxiStandLink)
	{
		setCruisingMode();
	}
	else
	{
		queuingTooLongTime = 0.0;
		parentTaxiDriver->setTaxiDriveMode(DRIVE_TO_TAXISTAND);
		destinationNode = destForRouteChoice;
		addTaxiStandPath(routeToTaxiStand);
	}
}

void TaxiDriverMovement::addTaxiStandPath(vector<WayPoint> &routeToTaxiStand)
{
	const SegmentStats* currSegStat = pathMover.getCurrSegStats();
	std::vector<const SegmentStats*> path;
	for (std::vector<WayPoint>::const_iterator itr = routeToTaxiStand.begin();itr != routeToTaxiStand.end(); itr++)
	{
		if((*itr).type==WayPoint::LINK){
			const Link *link = (*itr).link;
			Node *toNode = link->getToNode();
			Conflux *nodeConflux = Conflux::getConfluxFromNode(toNode);
			const std::vector<RoadSegment*>& roadSegments = link->getRoadSegments();
			for (std::vector<RoadSegment*>::const_iterator segItr =	roadSegments.begin(); segItr != roadSegments.end(); segItr++)
			{
				const std::vector<SegmentStats*>& segStatsVector = nodeConflux->findSegStats(*segItr);
				path.insert(path.end(), segStatsVector.begin(),	segStatsVector.end());
			}
		}
	}
	pathMover.setPath(path);
	pathMover.setSegmentStatIterator(currSegStat);
}

bool TaxiDriverMovement::isToBeRemovedFromTaxiStand()
{
	return toBeRemovedFromTaxiStand;
}

TaxiDriverBehavior::TaxiDriverBehavior()
{

}

TaxiDriverBehavior::~TaxiDriverBehavior()
{

}
}
}
