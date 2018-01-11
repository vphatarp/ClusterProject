/*
 * NodeHandler.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: root
 */

#include "NodeHandler.h"
#include "ElectionManager.h"
#define MESSAGE_INTERVAL 1
#define QUOR 9


namespace Cluster{

static int cnt=0;

NodeHandler::NodeHandler(uint32_t _id, uint32_t _priority){
	id = _id;
	priority = _priority;
	master_id = id;
	noOfPeers = 0;
	master_priority = priority;
	sendproto.set_packettype(static_cast<int>(Cluster::ElectionState::DISCOVER));
	sendproto.set_id(id);
	sendproto.set_priority(priority);
	sendproto.set_master(0);
	sendproto.set_backup(0);
	(*mapptr)[id] = priority;
	mhandler.mcastSocket();
	mhandler.setNonBlocking();
	mhandler.setReuse();
	mhandler.createEpoll();
	mhandler.bindListeningPort("239.253.1.1", "127.0.0.1");
	mhandler.addEpoll();
	mhandler.bindSendingPort("239.253.1.1", "127.0.0.1");
	pkmgr = std::make_shared<Cluster::ElectionManager>(this);
	masterDown = false;
}


void NodeHandler::startElection(){
	int waitTime; //To set the wait time on epoll socket
	//Needed 2 loops because for testing all the nodes were running on the same server
	//In order to avoid epoll loop going busy since one's own packets are received
	//2 loops are deployed
	for(;;){
		//Check what's the time now in order to determine the 30 second interval
		timePoint_ms interval = std::chrono::steady_clock::now() + std::chrono::seconds(MESSAGE_INTERVAL);
		send();
		while(true){
			//Determine time left till the end of the interval
			//which would be used for the epoll interface
			timePoint_ms curr = std::chrono::steady_clock::now();
			if(curr > interval){
				std::cout<<"End of interval... \n";
				break;
			}
			//std::cout<<"INTERVAL ENDS AT "<<curr<<"\n\n";
			std::chrono::steady_clock::duration dur = interval - curr;
			waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

			if(mhandler.recvEpoll(waitTime)){
				while(mhandler.recvData()){
					recvproto.ParseFromArray(&mhandler.databuf[0], mhandler.datalen);
					pkmgr->recvPacket(recvproto);
				}
				if(noOfPeers > 0){
					if(id ==  master_id){
						monitorNeighborHealth();
					}
					else{
						if((masterDown = monitorMaster())){
							break;
						}
					}
				}
				if(fsmstate != Cluster::NodeState::INIT){
						break;
				}
			}

		}
		if(masterDown){
			restartElections();
		}
		if(fsmstate == Cluster::NodeState::ELECTION){
			std::cout<<"Moving to election consensus \n";
			break;
		}
		if(fsmstate == Cluster::NodeState::FULL){
			std::cout<<"Moving to Full State \n";
			break;
		}
	}

}

void NodeHandler::elecConsensus(){
	int waitTime;
	for(;;){
		timePoint_ms interval = std::chrono::steady_clock::now() + std::chrono::seconds(MESSAGE_INTERVAL);
		send();
		while(true){
			timePoint_ms curr = std::chrono::steady_clock::now();
			std::chrono::steady_clock::duration dur = interval - curr;
			waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
			if(mhandler.recvEpoll(waitTime)){
				while(mhandler.recvData()){
					recvproto.ParseFromArray(&mhandler.databuf[0], mhandler.datalen);
					pkmgr->recvPacket(recvproto);
					if((recvproto.id() != id)){
						cnt++;
						//if(noOfPeers > QUORUM){
						if(id == master_id){
							masterNodeElecConsensus();
						}
						else{
							fullNodeElecConsensus();
						}
						//}
					}
				}
			}
			else{
				if(std::chrono::steady_clock::now() >= interval){
					break;
				}
			}
		}
	}
}

void NodeHandler::masterNodeElecConsensus(){
	if(recvproto.packettype() == static_cast<int>(Cluster::PacketType::ELEC_CONSENSUS)){
		if(recvproto.candidate() == id){
			consensusSet.insert(recvproto.id());
			//nList->set_id(recvproto.id());
			if(consensusSet == neighborSet){
				fsmstate = Cluster::NodeState::MASTER;
				sendproto.set_packettype(static_cast<int>(Cluster::PacketType::ELEC_COMPLETE));
				sendproto.set_master(id);
				send();
				while(true){
					send();
					std::cout<<"ELECTION IS COMPLETE. NODE WITH ID "<<id<<" IS THE MASTER NODE with "<<cnt<<std::endl;
					//networkMonitor();
					sleep(3);
				}
			}
		}
	}
}

void NodeHandler::fullNodeElecConsensus(){
	if(recvproto.packettype() == static_cast<int>(Cluster::PacketType::ELEC_COMPLETE)){
		if(recvproto.id() == master_id){
			fsmstate = Cluster::NodeState::FULL;
			sendproto.set_packettype(static_cast<int>(Cluster::PacketType::ELEC_COMPLETE));
			sendproto.set_master(id);
			while(true){
				send();
				std::cout<<"ELECTION IS COMPLETE. NODE WITH ID "<<master_id<<" IS THE MASTER NODE with "<<cnt<<std::endl;
				sleep(3);
			}
		}
	}
}

int NodeHandler::randfunc(){
	unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
	std::default_random_engine e(seed);
	return e()%3;
}

bool NodeHandler::addNeighbor(uint32_t neighbor_id, uint32_t neighbor_priority, timePoint_ms tp){
	auto neighborMessage = ntable.find(neighbor_id);
	if(neighborMessage != ntable.end()){
		std::cout<<"Neighbor found in the neighbor table \n";
		neighborMessage->second->tp = tp;
		return false;
	}
	else{
		neighborSet.insert(neighbor_id);
		std::shared_ptr<Neighbor> n = std::make_shared<Neighbor>();
		n->id = neighbor_id;
		n->priority = neighbor_priority;
	    n->tp = tp;
	    n->is_alive = true;
	    ntable[neighbor_id] = n;
	    return true;
	}
}

bool NodeHandler::setMasterID(uint32_t id, uint32_t priority){
	if(priority > master_priority){
		prevMaster = master_id;
		master_id = id;
		master_priority = priority;
		return true;
	}
	else if(priority == master_priority){
		if(id > master_id){
			prevMaster = master_id;
			master_id = id;
		    master_priority = priority;
		    return true;
		}
		return false;
	}
	return false;
}

void NodeHandler::monitorNeighborHealth(){
	timePoint_ms curr_time = std::chrono::steady_clock::now();
	std::chrono::steady_clock::duration timeBetweenMessages;
	int timeDiff;
	int noNeighbors = 0;
	auto iter = ntable.begin();
	while(noNeighbors < ntable.size()){
		timeBetweenMessages = curr_time - iter->second->tp;
		timeDiff = std::chrono::duration_cast<std::chrono::seconds>(timeBetweenMessages).count();
		std::cout<<"THE TIME BETWEEN THE MESSAGES FROM ID "<<iter->first<<" IS "<<timeDiff<<"\n";
		if(timeDiff > 60){
			auto deadNeighbor = mapptr->find(iter->first);
			iter = ntable.erase(iter);
			mapptr->erase(deadNeighbor);
			--noOfPeers;
		}
		else{
			noNeighbors++;
			iter++;
		}
		if(iter == ntable.end()){
			break;
		}
	}
	if(noOfPeers < QUOR){
		setDiscoverMode();
	}
}

void NodeHandler::setDiscoverMode(){
	fsmstate = Cluster::NodeState::INIT;
	sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::DISCOVER));
	pkmgr->changeState(Cluster::ElectionState::DISCOVER);
}

void NodeHandler::setElecConsensusMode(){
	fsmstate = Cluster::NodeState::ELECTION;
	sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::ELEC_CONSENSUS));
	pkmgr->changeState(Cluster::ElectionState::ELEC_CONSENSUS);
}

void NodeHandler::send(){
	sendproto.SerializeToString(&sendmessage);
	mhandler.sendData(sendmessage);
}

bool NodeHandler::receive(int timer_ms){
	if (mhandler.recvEpoll(timer_ms)){
		return true;
	}
	else{
		return false;
	}

}

void NodeHandler::restartElections(){
	master_id = id;
	noOfPeers = 0;
	master_priority = priority;
	pkmgr.reset();
	if(mapptr->size() > 0){
		mapptr->clear();
	}
	sendproto.set_packettype(static_cast<int>(Cluster::ElectionState::DISCOVER));
	sendproto.set_id(id);
	sendproto.set_priority(priority);
	sendproto.set_master(0);
	sendproto.set_backup(0);
	(*mapptr)[id] = priority;
	pkmgr = std::make_shared<Cluster::ElectionManager>(this);
	fsmstate = Cluster::NodeState::INIT;
	masterDown = false;
	ntable.clear();
}

bool NodeHandler::monitorMaster(){
	timePoint_ms current = std::chrono::steady_clock::now();
	if(ntable.size() > 0){
		std::chrono::steady_clock::duration dura =  current - (ntable[master_id]->tp);
		int missedTime = std::chrono::duration_cast<std::chrono::seconds>(dura).count();
		if(missedTime > 3){
			std::cout<<"The master node is down \n";
			std::cout<<"Starting over the elections \n";
			return true;
		}
		return false;
	}
}

}


