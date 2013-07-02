/* 
 * Copyright Singapore-MIT Alliance for Research and Technology
 * 
 * File:   HouseholdSellerRole.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on May 16, 2013, 5:13 PM
 */
#include <math.h>
#include "HouseholdSellerRole.hpp"
#include "message/LT_Message.hpp"
#include "agent/impl/HouseholdAgent.hpp"
#include "util/Statistics.hpp"
#include "util/Math.hpp"

using namespace sim_mob::long_term;
using std::list;
using std::endl;
using sim_mob::Math;

HouseholdSellerRole::HouseholdSellerRole(HouseholdAgent* parent, Household* hh,
        HousingMarket* market)
: LT_AgentRole(parent), market(market), hh(hh), currentTime(0, 0), hasUnitsToSale(true) {
}

HouseholdSellerRole::~HouseholdSellerRole() {
    maxBidsOfDay.clear();
}

void HouseholdSellerRole::Update(timeslice now) {
    if (hasUnitsToSale) {
        list<Unit*> units;
        GetParent()->GetUnits(units);
        for (list<Unit*>::iterator itr = units.begin(); itr != units.end();
                itr++) {
            if ((*itr)->IsAvailable()) {
                CalculateUnitExpectations(*(*itr));
                market->AddUnit((*itr));
            }
        }
        hasUnitsToSale = false;
    }
    if (now.ms() > currentTime.ms()) {
        // Day has changed we need to notify the last day winners.
        NotifyWinnerBidders();
        // TODO: adjust the price based on Expectations.
        AdjustNotSelledUnits();
    }
    currentTime = now;
}

void HouseholdSellerRole::HandleMessage(MessageType type, MessageReceiver& sender,
        const Message& message) {

    switch (type) {
        case LTMID_BID:// Bid received 
        {
            BidMessage* msg = MSG_CAST(BidMessage, message);
            Unit* unit = GetParent()->GetUnitById(msg->GetBid().GetUnitId());
            LogOut("Seller: [" << GetParent()->getId() <<
                    "] received a bid: " << msg->GetBid() <<
                    " at day: " << currentTime.ms() << endl);
            bool decision = false;
            if (unit && unit->IsAvailable()) {
                //verify if is the bid satisfies the asking price.
                decision = Decide(msg->GetBid(), *unit);
                if (decision) {
                    //get the maximum bid of the day
                    Bids::iterator bidItr = maxBidsOfDay.find(unit->GetId());
                    Bid* maxBidOfDay = nullptr;
                    if (bidItr != maxBidsOfDay.end()) {
                        maxBidOfDay = &(bidItr->second);
                    }

                    if (!maxBidOfDay) {
                        maxBidsOfDay.insert(BidEntry(unit->GetId(),
                                msg->GetBid()));
                    } else if (maxBidOfDay->GetValue() < msg->GetBid().GetValue()) {
                        // bid is higher than the current one of the day.
                        // it is necessary to notify the old max bidder
                        // that his bid was not accepted.
                        //reply to sender.
                        maxBidOfDay->GetBidder()->Post(LTMID_BID_RSP, GetParent(),
                                new BidMessage(Bid(*maxBidOfDay), BETTER_OFFER));
                        maxBidsOfDay.erase(unit->GetId());
                        //update the new bid and bidder.
                        maxBidsOfDay.insert(BidEntry(unit->GetId(), msg->GetBid()));
                    } else {
                        sender.Post(LTMID_BID_RSP, GetParent(),
                                new BidMessage(Bid(msg->GetBid()), BETTER_OFFER));
                    }
                } else {
                    sender.Post(LTMID_BID_RSP, GetParent(),
                            new BidMessage(Bid(msg->GetBid()), NOT_ACCEPTED));
                }
            } else {
                // Sellers is not the owner of the unit or unit is not available.
                sender.Post(LTMID_BID_RSP, GetParent(),
                        new BidMessage(Bid(msg->GetBid()), NOT_AVAILABLE));
            }
            Statistics::Increment(Statistics::N_BIDS);
            break;
        }
        default:break;
    }
}

bool HouseholdSellerRole::Decide(const Bid& bid, const Unit& unit) {
    return 0;//bid.GetValue() > unit.GetReservationPrice();
}

void HouseholdSellerRole::AdjustUnitParams(Unit& unit) {
    /*float denominator = pow((1 - 2 * unit.GetFixedPrice()), 0.5f);
    //re-calculates the new reservation price.
    float reservationPrice =
            (unit.GetReservationPrice() + unit.GetFixedPrice()) / denominator;
    //re-calculates the new hedonic price.
    float hedonicPrice = reservationPrice / denominator;
    //update values.
    unit.SetReservationPrice(reservationPrice);*/
}

void HouseholdSellerRole::NotifyWinnerBidders() {
    for (Bids::iterator itr = maxBidsOfDay.begin(); itr != maxBidsOfDay.end();
            itr++) {
        Bid* maxBidOfDay = &(itr->second);
        maxBidOfDay->GetBidder()->Post(LTMID_BID_RSP, GetParent(),
                new BidMessage(Bid(*maxBidOfDay), ACCEPTED));
        Unit* unit = GetParent()->GetUnitById(maxBidOfDay->GetUnitId());
        if (unit && unit->IsAvailable()) {
            unit->SetAvailable(false);
            unit = GetParent()->RemoveUnit(unit->GetId());
            // clean all expectations for the unit.
            unitExpectations.erase(unit->GetId());
        }
    }
    // notify winners.
    maxBidsOfDay.clear();
}

void HouseholdSellerRole::AdjustNotSelledUnits() {
    list<Unit*> units;
    GetParent()->GetUnits(units);
    for (list<Unit*>::iterator itr = units.begin(); itr != units.end(); itr++) {
        if ((*itr)->IsAvailable()) {
            AdjustUnitParams((**itr));
        }
    }
}

inline double ExpectationFunction(double x, double* params) {
    double v = params[0];
    double theta = params[1];
    double alpha = params[2];
    double price = x; // last expectation (V(t+1))
    // Calculates the bids distribution using F(X) = X/Price where F(V(t+1)) = V(t+1)/Price
    double bidsDistribution = v / price;
    // Calculates the probability of not having any bid greater than v.
    double priceProb = pow(Math::E, -((theta / pow(price, alpha)) * (1 - bidsDistribution)));
    // Calculates expected maximum bid.
    double p1 = pow(price, 2 * alpha + 1);
    double p2 = (price * (theta * pow(price, -alpha) - 1));
    double p3 = pow(Math::E, (theta * pow(price, -alpha)* (bidsDistribution - 1)));
    double p4 = (price - theta * v * pow(price, -alpha));
    double expectedMaxBid = (p1 * (p2 + p3 * p4)) / (theta * theta);
    return (v * priceProb + (1 - priceProb) * expectedMaxBid) - (0.01f * price);
}

void HouseholdSellerRole::CalculateUnitExpectations(const Unit& unit) {
    ExpectationList expectationList;
    double price = 20; //unit.GetReservationPrice();
    double expectation = 4;
    //double initialExpectation = unit.GetHedonicPrice();
    double theta = 1.0f; // hh->GetWeightExpectedEvents()
    double alpha = 2.0f; //hh->GetWeightPriceImportance()
    for (int i = 0; i < (2 * TIME_UNIT); i++) {
        ExpectationEntry entry;

        entry.price = Math::FindMaxArg(ExpectationFunction,
                price, (double[]) {
            expectation, theta, alpha
        }, .001f, 100000);

        entry.expectation = ExpectationFunction(entry.price, (double[]) {
            expectation, theta, alpha
        });
        expectation = entry.expectation;
        expectationList.push_back(entry);
        LogOut("Expectation on: [" << i << std::setprecision(15) <<
                "] Unit: [" << unit.GetId() <<
                "] expectation: [" << entry.expectation <<
                "] price: [" << entry.price <<
                "] theta: [" << theta <<
                "] alpha: [" << alpha <<
                "]" << endl);
    }
    unitExpectations.erase(unit.GetId());
    unitExpectations.insert(ExpectationMapEntry(unit.GetId(), expectationList));
}

