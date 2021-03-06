//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "CrossingLoader.hpp"


#include <iostream>
#include <limits>

#include "logging/Log.hpp"
#include "util/GeomHelpers.hpp"
#include "geospatial/network/RoadNetwork.hpp"
#include "geospatial/network/RoadSegment.hpp"
#include "geospatial/network/Lane.hpp"
#include "util/OutputUtil.hpp"

#include "Section.hpp"


using std::pair;
using std::map;
using std::vector;
using namespace sim_mob::aimsun;


namespace {



//Retrieve the minimum value in a list of IDs
int minID(const vector<double>& vals)
{
    int res = -1;
    for (size_t i=0; i<vals.size(); i++) {
        if (res==-1 || (vals[i]<vals[res])) {
            res = i;
        }
    }
    return res;
}


} //End anon namespace




void sim_mob::aimsun::CrossingLoader::DecorateCrossings(map<int, Node>& nodes, vector<Crossing>& crossings)
{
/*
    if(crossings.empty()) { return; }

    //Step 5: Tag all Nodes with the crossings that are near to these nodes.
    for (vector<Crossing>::iterator it=crossings.begin(); it!=crossings.end(); it++) {
        //Given the section this crossing is on, find which node on the section it is closest to.
        double dFrom = sim_mob::dist(it->xPos, it->yPos, it->atSection->fromNode->xPos, it->atSection->fromNode->yPos);
        double dTo = sim_mob::dist(it->xPos, it->yPos, it->atSection->toNode->xPos, it->atSection->toNode->yPos);
        Node* atNode = (dFrom<dTo) ? it->atSection->fromNode : it->atSection->toNode;

        //Now, store by laneID
        it->atNode = atNode;
        atNode->crossingsAtNode[it->laneID].push_back(&(*it));
    }

    //Step 6: Tag all laneIDs for Crossings in a Node with the Node they lead to. Do this in the most obvious
    //        way possible: simply construct pairs of points, and see if one of these intersects an outgoing
    //        Section from that Node.
    vector<int> skippedCrossingLaneIDs;
    for (map<int, Node>::iterator itN=nodes.begin(); itN!=nodes.end(); itN++) {
        Node& n = itN->second;
        for (map<int, std::vector<Crossing*> >::iterator it=n.crossingsAtNode.begin(); it!=n.crossingsAtNode.end(); it++) {
            //Search through pairs of points
            bool found = false;
            for (size_t i=0; i<it->second.size()&&!found; i++) {
                for (size_t j=i+1; j<it->second.size()&&!found; j++) {
                    //NOTE:The following are OVERRIDES; they should be set somewhere else eventually.
                    if (it->first==4550 || it->first==4215) {
                        std::cout <<"OVERRIDE: Manually skipping laneID: " <<it->first <<"\n";
                        i = j = it->second.size();
                        continue;
                    }

                    //And search through all RoadSegments
                    for (vector<Section*>::iterator itSec=n.sectionsAtNode.begin(); itSec!=n.sectionsAtNode.end()&&!found; itSec++) {
                        //Get the intersection between the two Points, and the Section we are considering
                        Point intRes = sim_mob::LineLineIntersect(it->second[i], it->second[j], *itSec);
                        if (intRes.getX()==std::numeric_limits<int>::max()) {
                            //Lines are parallel
                            continue;
                        }

                        //Check if this Intersection is actually ON both lines
                        bool actuallyIntersects = sim_mob::lineContains(it->second[i], it->second[j], intRes.getX(), intRes.getY()) && lineContains(*itSec, intRes.getX(), intRes.getY());
                        if (actuallyIntersects) {
                            Node* other = ((*itSec)->fromNode!=&n) ? (*itSec)->fromNode : (*itSec)->toNode;
                            n.crossingLaneIdsByOutgoingNode[other].push_back(it->first);
                            found = true;
                        }
                    }
                }
            }

            //Double-check that we found at least one.
            if (!found) {
                skippedCrossingLaneIDs.push_back(it->first);
            }
        }
    }

    //Print all skipped lane-crossing IDs:
    sim_mob::PrintArray(skippedCrossingLaneIDs, std::cout, "Skipped \"crossing\" laneIDs: ", "[", "]", ", ", 4);
*/
}


void sim_mob::aimsun::CrossingLoader::GenerateACrossing(sim_mob::RoadNetwork& resNW, Node& origin, Node& dest, vector<int>& laneIDs)
{/*
    //Nothing to do here?
    if (laneIDs.empty()) {
        return;
    }

    //Check errors
    if (laneIDs.size()!=2) {
        //TODO: Later, we can probably reduce the number of "Lanes" by automatically merging them.
        sim_mob::Warn() <<"ERROR: Crossing contains " <<laneIDs.size() <<" lane(s) instead of 2\n";
        return;
    }

    //Reduce the number of points on each "Lane" to 2. Also record the distance of the midpoint of the final
    // line from the origin node.
    std::vector<double> lineDistsFromOrigin;
    std::vector<Point> midPoints;
    std::vector< std::pair<Point, Point> > lineMinMaxes;
    std::vector<Crossing*> candidates;
    for (std::vector<int>::iterator it=laneIDs.begin(); it!=laneIDs.end(); it++) {
        //Quick check
        candidates = origin.crossingsAtNode.find(*it)->second;
        if (candidates.empty() || candidates.size()==1){
            sim_mob::Warn() <<"ERROR: Unexpected Crossing candidates size.\n";
            return;
        }

        //Reduce to 2 points
        while (candidates.size()>2) {
            //Our method is pretty simple; compute the combined distance from each point to each other. The one
            //  with the smallest combined distance is in the middle of the other 2, and can be removed.
            vector<double> dists;
            dists.push_back(sim_mob::dist(candidates[0], candidates[1]) + sim_mob::dist(candidates[0], candidates[2]));
            dists.push_back(sim_mob::dist(candidates[1], candidates[0]) + sim_mob::dist(candidates[1], candidates[2]));
            dists.push_back(sim_mob::dist(candidates[2], candidates[0]) + sim_mob::dist(candidates[2], candidates[1]));
            int pMin = minID(dists);
            if (pMin==-1) {
                sim_mob::Warn() <<"ERROR: No minimum point.\n";
                return;
            }

            candidates.erase(candidates.begin()+pMin);
        }

        //Now save these two points and their combined distance.
        pair<Point, Point> res = std::make_pair(Point(candidates[0]->xPos, candidates[0]->yPos), Point(candidates[1]->xPos, candidates[1]->yPos));
        lineMinMaxes.push_back(res);
        Point midPoint((res.second.getX()-res.first.getX())/2 + res.first.getX(),(res.second.getY()-res.first.getY())/2 + res.first.getY());
        double distOrigin = dist(midPoint.getX(), midPoint.getY(), origin.xPos, origin.yPos);
        lineDistsFromOrigin.push_back(distOrigin);
        midPoints.push_back(midPoint);
    }


    //We guarantee that the "first" and "second" points are nearest to each other (e.g., "near" and "far" point in the same direction)
    double d1 = dist(lineMinMaxes[0].first.getX(), lineMinMaxes[0].first.getY(), lineMinMaxes[1].first.getX(), lineMinMaxes[1].first.getY());
    double d2 = dist(lineMinMaxes[0].first.getX(), lineMinMaxes[0].first.getY(), lineMinMaxes[1].second.getX(), lineMinMaxes[1].second.getY());
    if (d2<d1) {
        std::swap(lineMinMaxes[1].first, lineMinMaxes[1].second);
    }


    //Create a sim_mob Crossing object.
    sim_mob::Crossing* res = new sim_mob::Crossing();
//  std::cout << "Crossing created : " << res << "\n";
    if (lineDistsFromOrigin[0] < lineDistsFromOrigin[1]) {
        res->nearLine = lineMinMaxes[0];
        res->farLine = lineMinMaxes[1];
    } else {
        res->nearLine = lineMinMaxes[1];
        res->farLine = lineMinMaxes[0];
    }
    //general book keeping
    candidates[0]->generatedCrossing = candidates[1]->generatedCrossing = res; //candidates size is mandatorily equal to 2

    //This crossing will now be listed as an obstacle in all Segments which share the same two nodes. Its "offset" will be determined from the "start"
    //   of the given segment to the "midpoint" of the two midpoints of the near/far lines.
    Point midPoint((midPoints[1].getX()-midPoints[0].getX())/2 + midPoints[0].getX(),(midPoints[1].getY()-midPoints[0].getY())/2 + midPoints[0].getY());
    for (vector<Section*>::iterator it=origin.sectionsAtNode.begin(); it!=origin.sectionsAtNode.end(); it++) {
        bool match =    ((*it)->generatedSegment->start==origin.generatedNode && (*it)->generatedSegment->end==dest.generatedNode)
                     || ((*it)->generatedSegment->end==origin.generatedNode && (*it)->generatedSegment->start==dest.generatedNode);
        if (match) {
            //Fortunately, this is always in the "forward" direction.
            double distOrigin = dist(midPoint.getX(), midPoint.getY(), (*it)->fromNode->xPos, (*it)->fromNode->yPos);
            if (distOrigin > (*it)->length) {
                //If the reported distance is too short, double-check the Euclidean distance...
                //TODO: Why would the reported distance ever be shorter? (It'd be longer if polylines were involved...)
                double euclideanCheck = dist((*it)->toNode->xPos, (*it)->toNode->yPos, (*it)->fromNode->xPos, (*it)->fromNode->yPos);
                if (distOrigin > euclideanCheck) {
                    sim_mob::Warn() <<"ERROR: Crossing appears after the maximum length of its parent Segment.\n"
                                    <<"  Requested offset is: " <<distOrigin/100000 <<"\n"
                                    <<"  Segment reports its length as: " <<(*it)->length/100000 <<"\n"
                                    <<"  Euclidean check: " <<euclideanCheck/100000 <<"\n";
                    return;
                }
            }
            centimeter_t distOrigin_ = distOrigin;//obstacles work with centimeter_t not double. this canversion is made due to problems experienced-vahid
            //Add it. Note that it is perfectly ok (and expected) for multiple Segments to reference the same Crossing.
            res->setParentSegment((*it)->generatedSegment);
            //(*it)->generatedSegment->obstacles[distOrigin_] = res;
            (*it)->generatedSegment->addObstacle(distOrigin,res);
            //(*it)->generatedSegment->getId() * 10 + (*it)->generatedSegment->obstacles.size()
            res->setCrossingID( res->generateRoadItemID(*((*it)->generatedSegment))); //sorry I have to put the formula here -vahid
//          if((res->getRoadSegment()->getId() == 100001005)||(res->getRoadSegment()->getId() == 100001004))
//          {
//              std::cout << "Segment " << (*it)->generatedSegment << "  "<< (*it)->generatedSegment->getId() << " with length "  << (*it)->generatedSegment->length << " has a crossing at  " << distOrigin_ << " with crossingID: " << res->getCrossingID() << "  " << res << "  Obstacles size=" << (*it)->generatedSegment->obstacles.size() << "\n" ;
//
//          }
        }
    }*/
}

