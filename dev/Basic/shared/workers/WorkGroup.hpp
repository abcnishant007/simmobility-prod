/* Copyright Singapore-MIT Alliance for Research and Technology */

#pragma once

#include <queue>
#include <vector>
#include <stdexcept>
#include <boost/thread.hpp>
#include <string>

#include "GenConfig.h"

#include "util/LangHelpers.hpp"
#include "util/DebugFlags.hpp"

//Needed for ActionFunction
#include "workers/Worker.hpp"


namespace sim_mob
{

class StartTimePriorityQueue;
class EventTimePriorityQueue;
class PendingEntity;


/*
 * Worker wrapper, similar to thread_group but using barriers.
 * A SimpleWorkGroup provides a convenient wrapper for Workers, similarly to
 *   how a ThreadGroup manages threads. The main difference is that the number of
 *   worker threads cannot be changed once the object has been constructed.
 * A SimpleWorkGroup maintains one extra hold on the shared barrier; to "advance"
 *   a group, call SimpleWorkGroup::wait().
 *
 * \author Seth N. Hetu
 * \author Xu Yan
 */
class WorkGroup {
public:
	//Migration parameters
	struct EntityLoadParams {
		StartTimePriorityQueue& pending_source;
		std::vector<Entity*>& entity_dest;
		//EventTimePriorityQueue& agents_with_pending_event;
		//std::vector<Entity*>& agents_on_activity;
		EntityLoadParams(StartTimePriorityQueue& pending_source, std::vector<Entity*>& entity_dest)
					: pending_source(pending_source), entity_dest(entity_dest) {}

		//EntityLoadParams(StartTimePriorityQueue& pending_source, std::vector<Entity*>& entity_dest, EventTimePriorityQueue& agents_with_pending_event, std::vector<Entity*>& agents_on_activity)
			//: pending_source(pending_source), entity_dest(entity_dest), agents_with_pending_event(agents_with_pending_event), agents_on_activity(agents_on_activity) {}
	};


	//These are passed along to the Workers:
	//  endTick=0 means run forever.
	//  tickStep is used to allow Workers to skip ticks; no barriers are locked.
	explicit WorkGroup(size_t size, unsigned int endTick=0, unsigned int tickStep=1, bool auraManagerActive=false);

	virtual ~WorkGroup();

	//template <typename WorkType>  //For now, just assume Workers
	void initWorkers(/*Worker::ActionFunction* action,*/ EntityLoadParams* loader);

	//Worker<EntityType>* const getWorker(size_t id);
	void startAll();
	void interrupt();
	size_t size();

	void wait();
	void waitExternAgain();

	Worker* getWorker(int id);

	//Schedule an entity. Only call this during the "update" phase.
	void scheduleEntity(const PendingEntity& ent);

	void stageEntities();
	void collectRemovedEntities();
	std::vector< std::vector<Entity*> > entToBeRemovedPerWorker;

	void assignAWorker(Entity* ag);

	void assignLinkWorker();

	void assignAWorkerConstraint(Entity* ag);

	Worker* locateWorker(std::string linkID);

	//void scheduleEntForRemoval(Entity* ag);

//add by xuyan
#ifndef SIMMOB_DISABLE_MPI
public:
	void removeAgentFromWorker(Entity * ag);
	void addAgentInWorker(Entity * ag);
	int getTheMostFreeWorkerID() const;
#endif


protected:
	//Shared barrier
	boost::barrier shared_barr;
	boost::barrier external_barr;

	//Worker object management
	std::vector<Worker*> workers;

	//Used to coordinate which Worker gets the next Agent; currently round-robin.
	size_t nextWorkerID;

	//Passed along to Workers
	unsigned int endTick;
	unsigned int tickStep;

	//Maintain an offset. When it reaches zero, reset to tickStep and sync barriers
	unsigned int tickOffset;

	//Only used once
	size_t total_size;

	//Additional locking is required if the aura manager is active.
	bool auraManagerActive;

	//Needed to stay in sync with the workers
	frame_t nextTimeTickToStage;

	//Contains information needed to migrate Entities
	EntityLoadParams* loader;





};


}