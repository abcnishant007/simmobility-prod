/**
 * Worker wrapper, similar to thread_group but using barriers.
 * A WorkGroup provides a convenient wrapper for Workers, similarly to
 *   how a ThreadGroup manages threads. The main difference is that the number of
 *   worker threads cannot be changed once the object has been constructed.
 * A WorkGroup maintains one extra hold on the shared barrier; to "advance"
 *   a group, call WorkGroup::wait().
 */

#pragma once

#include <vector>
#include <stdexcept>
#include <boost/thread.hpp>

#include "workers/Worker.hpp"
#include "entities/Entity.hpp"


namespace sim_mob
{


template <class EntityType>
class WorkGroup {
public:
	//These are passed along to the Workers:
	//  endTick=0 means run forever.
	//  tickStep is used to allow Workers to skip ticks; no barriers are locked.
	WorkGroup(size_t size, unsigned int endTick=0, unsigned int tickStep=1);

	~WorkGroup();

	//template <typename WorkType>  //For now, just assume Workers
	void initWorkers(boost::function<void(Worker<EntityType>*)>* action=NULL);

	Worker<EntityType>& getWorker(size_t id);
	void startAll();
	void interrupt();
	size_t size();

	void wait();

	//TODO: Move this to the Worker, not the work group.
	void migrate(EntityType * ag, int fromID, int toID);


private:
	//Shared barrier
	boost::barrier shared_barr;
	boost::barrier external_barr;

	//Worker object management
	std::vector<Worker<EntityType>*> workers;

	//Passed along to Workers
	unsigned int endTick;
	unsigned int tickStep;

	//Maintain an offset. When it reaches zero, reset to tickStep and sync barriers
	unsigned int tickOffset;

	//Only used once
	size_t total_size;

};

}


/**
 * Template function must be defined in the same translational unit as it is declared.
 */
template <class EntityType>
void sim_mob::WorkGroup<EntityType>::initWorkers(boost::function<void(sim_mob::Worker<EntityType>*)>* action)
{
	for (size_t i=0; i<total_size; i++) {
		workers.push_back(new Worker<EntityType>(action, &shared_barr, &external_barr, endTick/tickStep));
	}
}


template <class EntityType>
sim_mob::Worker<EntityType>& sim_mob::WorkGroup<EntityType>::getWorker(size_t id)
{
	if (id >= workers.size())
		throw std::runtime_error("Invalid Worker id.");
	return *workers[id];
}


//////////////////////////////////////////////
// These also need the temoplate parameter,
// but don't actually do anythign with it.
//////////////////////////////////////////////


template <class EntityType>
sim_mob::WorkGroup<EntityType>::WorkGroup(size_t size, unsigned int endTick, unsigned int tickStep) :
		shared_barr(size+1), external_barr(size+1), endTick(endTick), tickStep(tickStep), total_size(size)
{
}


template <class EntityType>
sim_mob::WorkGroup<EntityType>::~WorkGroup()
{
	for (size_t i=0; i<workers.size(); i++) {
		workers[i]->join();  //NOTE: If we don't join all Workers, we get threading exceptions.
		delete workers[i];
	}
}


template <class EntityType>
void sim_mob::WorkGroup<EntityType>::startAll()
{
	tickOffset = tickStep;
	for (size_t i=0; i<workers.size(); i++) {
		workers[i]->start();
	}
}


template <class EntityType>
size_t sim_mob::WorkGroup<EntityType>::size()
{
	return workers.size();
}



template <class EntityType>
void sim_mob::WorkGroup<EntityType>::wait()
{
	if (--tickOffset>0) {
		return;
	}
	tickOffset = tickStep;

	shared_barr.wait();
	external_barr.wait();
}


template <class EntityType>
void sim_mob::WorkGroup<EntityType>::interrupt()
{
	for (size_t i=0; i<workers.size(); i++)
		workers[i]->interrupt();
}


/**
 * Set "fromID" or "toID" to -1 to skip that step.
 */
template <class EntityType>
void sim_mob::WorkGroup<EntityType>::migrate(EntityType* ag, int fromID, int toID)
{
	if (ag==NULL)
		return;

	//Remove from the old location
	if (fromID >= 0) {
		getWorker(fromID).remEntity(ag);
	}

	//Add to the new location
	if (toID >= 0) {
		getWorker(toID).addEntity(ag);
	}
}


