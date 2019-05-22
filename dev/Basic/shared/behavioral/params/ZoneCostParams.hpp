//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once
#include <string>
#include <sstream>

namespace sim_mob
{
const unsigned int NUM_30MIN_TIME_WINDOWS_IN_DAY = 48;

/**
 * Class to hold properties of a zone
 *
 * \author Harish Loganathan
 */
class ZoneParams
{
public:
    virtual ~ZoneParams()
    {
    }

    double getArea() const
    {
        return area;
    }

    void setArea(double area)
    {
        this->area = area;
    }

    int getCentralDummy() const
    {
        return centralZone;
    }

    void setCentralDummy(bool centralDummy)
    {
        this->centralZone = centralDummy;
    }

    double getEmployment() const
    {
        return employment;
    }

    void setEmployment(double employment)
    {
        this->employment = employment;
    }

    double getParkingRate() const
    {
        return parkingRate;
    }

    void setParkingRate(double parkingRate)
    {
        this->parkingRate = parkingRate;
    }

    double getPopulation() const
    {
        return population;
    }

    void setPopulation(double population)
    {
        this->population = population;
    }

    double getResidentStudents() const
    {
        return residentStudents;
    }

    void setResidentStudents(double residentStudents)
    {
        this->residentStudents = residentStudents;
    }

    double getResidentWorkers() const
    {
        return residentWorkers;
    }

    void setResidentWorkers(double residentWorkers)
    {
        this->residentWorkers = residentWorkers;
    }

    double getShop() const
    {
        return shop;
    }

    void setShop(double shop)
    {
        this->shop = shop;
    }

    double getTotalEnrollment() const
    {
        return totalEnrollment;
    }

    void setTotalEnrollment(double totalEnrollment)
    {
        this->totalEnrollment = totalEnrollment;
    }

    int getZoneCode() const
    {
        return zoneCode;
    }

    void setZoneCode(int zoneCode)
    {
        this->zoneCode = zoneCode;
    }

    int getZoneId() const
    {
        return zoneId;
    }

    void setZoneId(int zoneId)
    {
        this->zoneId = zoneId;
    }

    int getCbdDummy() const
    {
        return (cbdZone ? 1 : 0);
    }

    void setCbdDummy(int cbdZone)
    {
        this->cbdZone = cbdZone;
    }

private:
    int zoneId;
    int zoneCode;
    double shop;
    double parkingRate;
    double residentWorkers;
    bool centralZone;
    double employment;
    double population;
    double area;
    double totalEnrollment;
    double residentStudents;
    int cbdZone;
};

/**
 * Class to hold cost related parameters
 *
 * \author Harish Loganathan
 */
class CostParams
{
public:
    virtual ~CostParams()
    {
    }

    const std::string getOrgDest() const
    {
        return orgDest;
    }

    void setOrgDest()
    {
        std::stringstream ss;
        ss << originZone << "," << destinationZone;
        orgDest = ss.str();
    }

    double getAvgTransfer() const
    {
        return avgTransfer;
    }

    // a != a returns true if a is NaN. This check is used in many setters in this class.
    void setAvgTransfer(double avgTransfer)
    {
        if (avgTransfer != avgTransfer)
        {
            this->avgTransfer = 0;
        }
        else
        {
            this->avgTransfer = avgTransfer;
        }
    }

    double getCarCostErp() const
    {
        return carCostERP;
    }

    void setCarCostErp(double carCostErp)
    {
        if (carCostErp != carCostErp)
        {
            this->carCostERP = 0;
        }
        else
        {
            this->carCostERP = carCostErp;
        }
    }

    double getCarIvt() const
    {
        return carIvt;
    }

    void setCarIvt(double carIvt)
    {
        if (carIvt != carIvt)
        {
            this->carIvt = 0;
        }
        else
        {
            this->carIvt = carIvt;
        }
    }

    int getDestinationZone() const
    {
        return destinationZone;
    }

    void setDestinationZone(int destinationZone)
    {
        this->destinationZone = destinationZone;
    }

    double getDistance() const
    {
        return distance;
    }

    void setDistance(double distance)
    {
        if (distance != distance)
        {
            this->distance = 0;
        }
        else
        {
            this->distance = distance;
        }
    }

    int getOriginZone() const
    {
        return originZone;
    }

    void setOriginZone(int originZone)
    {
        this->originZone = originZone;
    }

    double getPubCost() const
    {
        return pubCost;
    }

    void setPubCost(double pubCost)
    {
        if (pubCost != pubCost)
        {
            this->pubCost = 0;
        }
        else
        {
            this->pubCost = pubCost;
        }
    }

    double getPubIvt() const
    {
        return pubIvt;
    }

    void setPubIvt(double pubIvt)
    {
        if (pubIvt != pubIvt)
        {
            this->pubIvt = 0;
        }
        else
        {
            this->pubIvt = pubIvt;
        }
    }

    double getPubOut() const
    {
        return pubOut;
    }

    void setPubOut(double pubOut)
    {
        if (pubOut != pubOut)
        {
            this->pubOut = 0;
        }
        else
        {
            this->pubOut = pubOut;
        }
    }

    double getPubWalkt() const
    {
        return pubWalkt;
    }

    void setPubWalkt(double pubWalkt)
    {
        if (pubWalkt != pubWalkt)
        {
            this->pubWalkt = 0;
        }
        else
        {
            this->pubWalkt = pubWalkt;
        }
    }

    double getPubWtt() const
    {
        return pubWtt;
    }

	double getSMSWtt() const
	{
		return smsWtt;
	}

	double getSMSPoolWtt() const
	{
		return smsPoolWtt;
	}

	double getAMODWtt() const
	{
		return amodWtt;
	}

	double getAMODPoolWtt() const
	{
		return amodPoolWtt;
	}

	double getAMODMinibusWtt() const
	{
		return amodMinibusWtt;
	}

	void setPubWtt(double pubWtt)
	{
		if (pubWtt != pubWtt)
		{
			this->pubWtt = 0;
		}
		else
		{
			this->pubWtt = pubWtt;
		}
	}

	void setSMSWtt(double smsWtt)
	{
		if (smsWtt != smsWtt)				// f != f will be true only if f is NaN.
		{
			this->smsWtt = 0;
		}
		else
		{
			this->smsWtt = smsWtt;
		}
	}
	void setSMSPoolWtt(double smsPoolWtt)
	{
		if (smsPoolWtt != smsPoolWtt)
		{
			this->smsPoolWtt = 0;
		}
		else
		{
			this->smsPoolWtt = smsPoolWtt;
		}
	}
	void setAMODWtt(double amodWtt)
	{
		if (amodWtt != amodWtt)
		{
			this->amodWtt = 0;
		}
		else
		{
			this->amodWtt = amodWtt;
		}
	}
	void setAMODPoolWtt(double amodPoolWtt)
	{
		if (amodPoolWtt != amodPoolWtt)
		{
			this->amodPoolWtt = 0;
		}
		else
		{
			this->amodPoolWtt = amodPoolWtt;
		}
	}
	void setAMODMinibusWtt(double amodMinibusWtt)
	{
		if (amodMinibusWtt != amodMinibusWtt)
		{
			this->amodMinibusWtt = 0;
		}
		else
		{
			this->amodMinibusWtt = amodMinibusWtt;
		}
	}

private:
	int originZone;
	int destinationZone;
	std::string orgDest;
	double pubWtt;
	double carIvt;
	double pubOut;
	double pubWalkt;
	double distance;
	double carCostERP;
	double pubIvt;
	double avgTransfer;
	double pubCost;
	double smsWtt;
	double smsPoolWtt;
	double amodWtt;
	double amodPoolWtt;
	double amodMinibusWtt;
};

/**
 * Class to hold time dependent travel times for one pair OD zones
 *
 * \author Harish Loganathan
 */
class TimeDependentTT_Params
{
public:
    TimeDependentTT_Params() : originZone{0}, destinationZone{0}, infoUnavailable{false}, arrivalBasedTT{}, departureBasedTT{}
    {
    }

    int getOriginZone() const
    {
        return originZone;
    }

    void setOriginZone(int originZone)
    {
        this->originZone = originZone;
    }

    int getDestinationZone() const
    {
        return destinationZone;
    }

    void setDestinationZone(int destinationZone)
    {
        this->destinationZone = destinationZone;
    }

    bool isInfoUnavailable() const
    {
        return infoUnavailable;
    }

    void setInfoUnavailable(bool infoUnavailable)
    {
        this->infoUnavailable = infoUnavailable;
    }

    double* getArrivalBasedTT()
    {
        return arrivalBasedTT;
    }

    double* getDepartureBasedTT()
    {
        return departureBasedTT;
    }

    /**
     * fetches i-th element of arrivalBasedTT
     * @param i index to fetch
     * @return arrivalBasedTT[i] if i is a valid index; -1 otherwise (invalid travel time)
     */
    double getArrivalBasedTT_at(int i) const
    {
        if(i<0 || i>=NUM_30MIN_TIME_WINDOWS_IN_DAY)
        {
            return -1;
        }
        return arrivalBasedTT[i];
    }

    /**
     * fetches i-th element of departureBasedTT
     * @param i index to fetch
     * @return departureBasedTT[i] if i is a valid index; -1 otherwise (invalid travel time)
     */
    double getDepartureBasedTT_at(int i) const
    {
        if(i<0 || i>=NUM_30MIN_TIME_WINDOWS_IN_DAY)
        {
            return -1;
        }
        return departureBasedTT[i];
    }

private:
    int originZone;
    int destinationZone;
    bool infoUnavailable;
    double arrivalBasedTT[NUM_30MIN_TIME_WINDOWS_IN_DAY];
    double departureBasedTT[NUM_30MIN_TIME_WINDOWS_IN_DAY];
};

/*
 * aa: This class associates to each node, the information about its Traffic Analysis Zone (TAZ)
 */
class ZoneNodeParams
{
public:
    virtual ~ZoneNodeParams()
    {
    }

    unsigned int getNodeId() const
    {
        return simmobNodeId;
    }

    void setNodeType(unsigned int nodeType)
    {
        this->simmobNodeType = nodeType;
    }

    unsigned int getNodeType() const
    {
        return simmobNodeType;
    }

    void setNodeId(unsigned int aimsunNodeId)
    {
        this->simmobNodeId = aimsunNodeId;
    }

    bool isSinkNode() const
    {
        return sinkNode;
    }

    void setSinkNode(bool sinkNode)
    {
        this->sinkNode = sinkNode;
    }

    bool isSourceNode() const
    {
        return sourceNode;
    }

    void setSourceNode(bool sourceNode)
    {
        this->sourceNode = sourceNode;
    }

    int getZone() const
    {
        return zone;
    }

    void setZone(int zone)
    {
        this->zone = zone;
    }

    bool isBusTerminusNode() const
    {
        return busTerminusNode;
    }

    void setBusTerminusNode(bool busTerminusNode)
    {
        this->busTerminusNode = busTerminusNode;
    }

private:
    /** taz code */
    int zone;
    /** simmobility node id */
    unsigned int simmobNodeId;
    /** simmobility node type */
    unsigned int simmobNodeType;
    /** is this a node with no upstream segments*/
    bool sourceNode;
    /** is this a node with no downstream segments*/
    bool sinkNode;
    /** is this node a bus-terminus node*/
    bool busTerminusNode;
};

} // namespace sim_mob
