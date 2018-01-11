/*
 * PacketManager.h
 *
 *  Created on: Dec 30, 2017
 *      Author: root
 */

#ifndef PACKETMANAGER_H_
#define PACKETMANAGER_H_
#include "packetstruct.pb.h"
#include "FSM.h"

namespace Cluster{
class PacketManager{

public:
	ipc::Packet message;
	virtual void recvPacket(ipc::Packet protpacket)=0;
	virtual void recvDiscover()=0;
	virtual void recvElectionStart()=0;
	virtual void recvElectionConsensus()=0;
	//virtual void recvElectionDone()=0;
	virtual void inElecStart()=0;
	virtual ~PacketManager(){};
	virtual void changeState(Cluster::ElectionState elecState)=0;
};
}



#endif /* PACKETMANAGER_H_ */
