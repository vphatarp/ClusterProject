/*
 * ElectionManager.h
 *
 *  Created on: Dec 30, 2017
 *      Author: root
 */

#ifndef ELECTIONMANAGER_H_
#define ELECTIONMANAGER_H_
#define QUORUM 2
#include "PacketManager.h"
#include <memory>
//#include "NodeHandler.h"

namespace Cluster{

class NodeHandler;
class ElectionManager:public Cluster::PacketManager{
	typedef  void (*func)(void);
	bool recvElecConsensus = false;
	Cluster::ElectionState electionState = Cluster::ElectionState::DISCOVER;
	const std::shared_ptr< Cluster::NodeHandler> nodeHandler;
public:
	ElectionManager(Cluster::NodeHandler* _nodeHandler):nodeHandler(std::shared_ptr<Cluster::NodeHandler>(_nodeHandler, [](Cluster::NodeHandler*){})){}
	virtual void recvPacket(ipc::Packet protpacket);
	virtual void recvDiscover();
	//For the time being setting them to delete
	virtual void recvElectionStart(){};
	virtual void recvElectionConsensus();
	virtual void recvElectionComplete();
	virtual void changeState(Cluster::ElectionState elecState);
	virtual void inElecStart(){};
	void ifElecMaster();
	void ifElecOther();
	bool checkifinList(google::protobuf::Map<uint32_t, uint32_t>* ptr);
};
}



#endif /* ELECTIONMANAGER_H_ */

