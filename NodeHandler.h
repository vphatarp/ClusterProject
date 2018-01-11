/*
 * NodeHandler.h
 *
 *  Created on: Dec 30, 2017
 *      Author: root
 */

#ifndef NODEHANDLER_H_
#define NODEHANDLER_H_
#include <unordered_map>
#include <set>
#include "MulticastIO.h"
#include "FSM.h"
#include "packetstruct.pb.h"
#include "PacketManager.h"
//#include "ElectionManager.h"
#include <chrono>
#include <iomanip>
#include <ctime>
#include <time.h>

namespace Cluster{

class ElectionManager;

class NodeHandler{
	typedef void(*stateptr)(void);
	typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_ms;
	typedef int cuant;
	bool masterDown;
	//Protocol Buffer Variables to process Send and Receive packets
	ipc::Packet sendproto;
	google::protobuf::Map<uint32_t, uint32_t>* mapptr = sendproto.mutable_knownneighbors();
	//google::protobuf::Map<uint32_t, uint32_t> knownNeighbor();
	ipc::Packet recvproto;

	//TO be used to serialize the packet structure
	std::string sendmessage;

	//The base class for initializing different handlers based on Node states
	//For example:- Election State, Full State, Master or Backup State
	std::shared_ptr<Cluster::PacketManager> pkmgr;

	//All Multicast Socket functionality will be handled by mhandler
	IO_Manager::MulticastIO mhandler;

	//ID and priorty and the current state of a node
	int id ;
	int priority;


	//To keep track of the Master ID during elections
	//int master_id=0;
	//int master_priority=0;

	//To maintain state of neighbors
	class Neighbor{
	public:
		uint32_t id;
		uint32_t priority;
		uint32_t candidate;
		uint32_t master;
		uint32_t backup;
		timePoint_ms tp;
		bool is_alive;
	};
	std::unordered_map<uint32_t, std::shared_ptr<Neighbor> > ntable;
	std::unordered_map<uint32_t, uint32_t> knownNeighbors;
	std::set<uint32_t> neighborSet; //For faster comparison of dead and live neighbors
	std::set<uint32_t> consensusSet;
	uint32_t noOfPeers;

public:
	Cluster::NodeState fsmstate=Cluster::NodeState::INIT;
	friend class ElectionManager;
	uint32_t prevMaster=0;
	uint32_t master_id=0;
	uint32_t master_priority=0;
	NodeHandler(uint32_t id=0, uint32_t priority=0);
	int randfunc();
	bool addNeighbor(uint32_t neighbor_id, uint32_t neighbor_priority, timePoint_ms tp);
	void delNeighbor();
	void fsmStateChange();
	bool checkRecvSocket(int timer_ms);
	void send();
	bool receive(int timer_ms);
	bool setMasterID(uint32_t id, uint32_t priority);
	bool monitorMaster();
	void startElection();
	void elecConsensus();
	void masterNodeElecConsensus();
	void fullNodeElecConsensus();
	void monitorNeighborHealth();
	void networkMonitor();
	void changeStateBasedonQuorum(int totalNodes);
	void restartElections();
	void setDiscoverMode();
	void setElecConsensusMode();
};
}



#endif /* NODEHANDLER_H_ */
