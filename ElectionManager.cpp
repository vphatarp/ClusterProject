/*
 * ElectionManager.cpp
 *
 *  Created on: Dec 30, 2017
 *      Author: root
 */

#include "ElectionManager.h"
#include "NodeHandler.h"
#include <string.h>
#define QUOR 9
typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_ms;

namespace Cluster{

void ElectionManager::changeState(Cluster::ElectionState elecState){
	electionState = elecState;
}

void ElectionManager::recvPacket(ipc::Packet protpacket){
	//std::cout<<"MESSAGE RECEIVED FROM ID "<<protpacket.id()<<"\n";
	if(protpacket.id() != nodeHandler->id){
	message = protpacket;
	//std::cout<<"MESSAGE ID AFTER SETTING "<<message.id()<<"\n";
	switch(static_cast<Cluster::PacketType>(message.packettype())){
	case Cluster::PacketType::DISCOVER:
		recvDiscover();
		break;
    case Cluster::PacketType::ELEC_CONSENSUS:
		recvElectionConsensus();
		break;
    case Cluster::PacketType::ELEC_COMPLETE:
		recvElectionComplete();
		break;
    default:
    	char str[32];
    	sprintf(str,"%d", protpacket.id());
	}
	}
	//else{
	//	std::cout<<"RECEIVING MY OWN MESSAGES \n\n\n\n\n\n\n\n\n\n\n";
	//}
}


void ElectionManager::recvDiscover(){
	std::cout<<"THE CURRENT MESSAGE IS BEING RECEIVED FROM NODE WITH ID "<<message.id()<<"\n\n\n";
	static bool flag_discover = false;
	if(electionState == Cluster::ElectionState::DISCOVER){
		timePoint_ms tp = std::chrono::steady_clock::now();

		//If this particular node is currently the master
		//This also covers the situation wherein a node initialized and just came up
		if(nodeHandler->id == nodeHandler->master_id){

			//Check if the the message is from a known neighbor
			if(nodeHandler->addNeighbor(message.id(), message.priority(), tp)){
				nodeHandler->noOfPeers++;
				std::cout<<"The number of peers has now become "<<nodeHandler->noOfPeers<<"and "<<message.knownneighbors_size()<<"\n\n";

				//Adding a new neighbor can also mean, a master is being added
				//Which would mean, it already has a known neighbor list
				//This list is only used to keep track of the number of nodes

				if(nodeHandler->setMasterID(message.id(), message.priority())){
					nodeHandler->ntable.clear();
					nodeHandler->addNeighbor(message.id(), message.priority(), tp);
					nodeHandler->noOfPeers = 0;
					google::protobuf::Map<uint32_t, uint32_t>* ptr = message.mutable_knownneighbors();
					if(ptr->count(nodeHandler->id) == 0){
						if(message.knownneighbors_size()+nodeHandler->noOfPeers - 1 > QUOR){
							std::cout<<"Quorum has reached \n";
							nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
							nodeHandler->sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::ELEC_CONSENSUS));
							nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
							flag_discover = true;
						}
					}
					else{
						if(message.knownneighbors_size() > QUOR){
							std::cout<<"Quorum has reached \n";
							nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
							nodeHandler->sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::ELEC_CONSENSUS));
							nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
							flag_discover = true;
						}
					}
					nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
					nodeHandler->send();
				}
				else{
					(*nodeHandler->mapptr)[message.id()] = message.priority();
					std::cout<<"THIS IS WHERE I WILL REACH FOR ALL THE NODES WITH A LOWER ID THAN MYSELF "<<message.id()<<"\n\n";
					//If the code reaches this section, it means a new neighbor with an ID lower than the current master
					//Is trying to join the cluster
					//This message could then be from the previous Master node
					//Getting a Download from the previous master node
					timePoint_ms tp = std::chrono::steady_clock::now();
					if(message.knownneighbors_size() > 0){
						google::protobuf::Map<uint32_t, uint32_t>* ptr = message.mutable_knownneighbors();
						for(auto it = ptr->begin();it != ptr->end(); ++it){
							if(nodeHandler->addNeighbor(it->first, it->second, tp)){
								nodeHandler->noOfPeers++;
							}
							(*nodeHandler->mapptr)[it->first] = it->second;
						}
						if(nodeHandler->noOfPeers > QUOR){
							nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
							nodeHandler->sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::ELEC_CONSENSUS));
							nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
						}
					}
					nodeHandler->send();
				}
			}

			//If the message is from a known neighbor
			//And this node is still the master
			else{
				//This is the situation when a node with a lower credentials is sending its keepalives
				//Since ADD NEIGHBOR function also takes care of updating the timepoint of the received keepalive
				//This section also does not need to handle any logic as such
			}
		}

		//If the node is not currently the master node
		//This section of code is written just to minimized the number of messages being exchanged between the nodes
		else{
			if(message.id() == nodeHandler->master_id){
				nodeHandler->addNeighbor(message.id(), message.priority(), tp); //Updating the timepoint of the keepalive from the master node
				std::cout<<"THE SECOND TIME THIS IS WHERE "<<nodeHandler->id<<" WILL COME WHEN I RECEIVE A MESSAGE FROM "<<nodeHandler->master_id<<"\n\n";

				if(message.knownneighbors_size() > 0){
										google::protobuf::Map<uint32_t, uint32_t>* pt = message.mutable_knownneighbors();
										int count=0;
										for(auto it=pt->begin();it != pt->end();++it){
											std::cout<<"THE "<<count<< "NODE IS "<<it->first<<"\n";
											count++;
										}
									}
				if(!checkifinList(message.mutable_knownneighbors())){
					if(message.knownneighbors_size() > QUOR){
						nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
						nodeHandler->sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::ELEC_CONSENSUS));
						nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
					}
					nodeHandler->send();
				}
				else{
					if(nodeHandler->mapptr->size()){
						nodeHandler->mapptr->clear();
					}
				}
			}
			else{
				if(nodeHandler->setMasterID(message.id(), message.priority())){
					//Which means we have encountered a new neighbor
					//First we check if the new neighbor has the potential to become the new master
					if(nodeHandler->addNeighbor(message.id(), message.priority(), tp)){
						if(message.knownneighbors_size() > QUOR){
							nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
							nodeHandler->sendproto.set_packettype(static_cast<uint32_t>(Cluster::PacketType::ELEC_CONSENSUS));
							nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
						}
					}
					else{
						//We should never reach this section of the code
						//This message is from the Highest ID node which is known but wasn't the master
						//If a particular node knows about a Higher ID node, it would immediately make it the master node
					}
				}
				else{
					//Since we are trying to keep track of only the master node
					//If the above statement comes out true
					//which would mean a new neighbor which has a lower ID has just joined
					//or it is a keepalive from an existing node sent out to the master node
					//Hence we ignore this message and let master handle the keep alives from the other neighbors
				}

			}

		}
	}
	else if(electionState == Cluster::ElectionState::ELEC_CONSENSUS){
		timePoint_ms tp = std::chrono::steady_clock::now();
		//If a Discover packettype message is received
		//But the node currently is in Elec Consensus state
		//It means, there are enough number of nodes to form Quorum
		//And hence this message should be from a node that has just entered the cluster
		//Again we handle this packet based on whether this particular node is a master or just a peer node
		if(nodeHandler->id == nodeHandler->master_id){
			//Check if the new node has higher credentials than the current Master Node
			//If yes, since yet a consensus has not been reached, we should accept the new node as Master
			if(nodeHandler->setMasterID(message.id(), message.priority())){
				//Send a message immediately to the new Node so that it sync up fast
				nodeHandler->sendproto.set_candidate(message.id());
				nodeHandler->send();
			}
			else{
				//Since the current Master node retains its position
				//Just add the new Node
				if(nodeHandler->addNeighbor(message.id(), message.priority(), tp)){
					++nodeHandler->noOfPeers;
				}
			}
		}
		else{
			//If the node is not the master
			if(message.id() == nodeHandler->master_id){
				//This statement is to record the keepalives coming from the master node
				nodeHandler->addNeighbor(message.id(), message.priority(), tp);
				nodeHandler->sendproto.set_candidate(message.id());
			}
			else{
				//If the message is not from the master node
				if(nodeHandler->setMasterID(message.id(), message.priority())){
					nodeHandler->addNeighbor(message.id(), message.priority(), tp);
				}
				else{
					//If it is from a node that has lower credentials than the current master node
					//Ignore the message and do nothing
					//Since only the master node keeps track of all the peers
				}
			}
		}
	}
	else if(electionState == Cluster::ElectionState::ELEC_COMPLETE){
		if(nodeHandler->id == nodeHandler->master_id){
			nodeHandler->send();
		}
	}

}

void ElectionManager::recvElectionConsensus(){
	timePoint_ms tp = std::chrono::steady_clock::now();
	uint32_t currMaster = message.candidate();
	//If the node processing the message is currently in Discover mode
	if(electionState == Cluster::ElectionState::DISCOVER){
		//Process messages only from the current candidate
		if(message.id() == message.candidate()){
			if(nodeHandler->setMasterID(message.candidate(), (*message.mutable_knownneighbors())[message.candidate()])){

			}
			else{
				//This node shall become the new master
				//Downloading the peer information from the current master
				nodeHandler->addNeighbor(message.id(), message.priority(), tp);
				nodeHandler->setElecConsensusMode();
				nodeHandler->sendproto.set_candidate(nodeHandler->id);
				if(message.knownneighbors_size() > 0){
					google::protobuf::Map<uint32_t, uint32_t>* ptr = message.mutable_knownneighbors();
					for(auto it = ptr->begin();it != ptr->end(); ++it){
						if(nodeHandler->addNeighbor(it->first, it->second, tp)){
							nodeHandler->noOfPeers++;
						}
						(*nodeHandler->mapptr)[it->first] = it->second;
					}
				}
				nodeHandler->send();
			}
		}
		else{
			//Only process packets from the current master
			//This section of code would mean, this message is from another peer not currently the master
			//Hence most likely it will be a keepalive since it has already participated in the election
		}

	}
	else if(electionState == Cluster::ElectionState::ELEC_CONSENSUS){

	}
	else if(electionState == Cluster::ElectionState::ELEC_COMPLETE){

	}
}

/*
void ElectionManager::recvDiscover(){
	timePoint_ms tp = std::chrono::steady_clock::now();
	if(nodeHandler->addNeighbor(message.id(), message.priority(), tp)){
		nodeHandler->noOfPeers++;
	}
	if(electionState == Cluster::ElectionState::DISCOVER){
		if(nodeHandler->noOfPeers >= QUORUM){
			std::cout<<"Reached Quorum....starting elections \n";
			electionState = Cluster::ElectionState::ELEC_CONSENSUS;
			nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
			nodeHandler->sendproto.set_packettype(static_cast<int>(Cluster::PacketType::ELEC_CONSENSUS));
			if(nodeHandler->setMasterID(message.id(), message.priority())){
				nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
				nodeHandler->send();
			}
		}
		else{
			if(nodeHandler->setMasterID(message.id(), message.priority())){
				nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
				nodeHandler->send();
			}
			else{
				if(nodeHandler->id == nodeHandler->master_id){
					if(message.consentedlist_size() == 0){
						nodeHandler->send();
					}
					else{
						bool idFound = false;
						for(int i=0; i< message.consentedlist_size();i++){
							const ipc::Packet::neighbor& n = message.consentedlist(i);
							if(n.id() == nodeHandler->id){
								idFound = true;
								break;
							}
						}
						if(idFound == false){
							nodeHandler->send();
						}
					}
				}
			}
			std::cout<<"No Quorum yet \n";
		}
	}
	else if(electionState == Cluster::ElectionState::ELEC_CONSENSUS){
		if(nodeHandler->setMasterID(message.id(), message.priority())){
			nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
			nodeHandler->send();
		}
	}
	else if(electionState == Cluster::ElectionState::ELEC_COMPLETE){
		if(nodeHandler->fsmstate == Cluster::NodeState::MASTER){
			nodeHandler->send();
		}
	}
}

void ElectionManager::recvElectionConsensus(){
	static int consensusSent =0;
	timePoint_ms tp = std::chrono::steady_clock::now();
	if(nodeHandler->addNeighbor(message.id(), message.priority(), tp)){
		nodeHandler->noOfPeers++;
	}
	if(electionState == Cluster::ElectionState::DISCOVER){
		if(nodeHandler->setMasterID(message.candidate(), message.priority())){
			electionState = Cluster::ElectionState::ELEC_CONSENSUS;
			nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
			nodeHandler->sendproto.set_packettype(static_cast<int>(Cluster::PacketType::ELEC_CONSENSUS));
			nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
			nodeHandler->send();
		}
		else{
			if(nodeHandler->noOfPeers >= QUORUM){
				electionState = Cluster::ElectionState::ELEC_CONSENSUS;
				nodeHandler->fsmstate = Cluster::NodeState::ELECTION;
				nodeHandler->sendproto.set_packettype(static_cast<int>(Cluster::PacketType::ELEC_CONSENSUS));
				nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
				nodeHandler->send();
			}
		}
	}
	else if(electionState == Cluster::ElectionState::ELEC_CONSENSUS){
		if(nodeHandler->setMasterID(message.candidate(), message.priority())){
			nodeHandler->sendproto.set_candidate(nodeHandler->master_id);
			nodeHandler->send();
		}
		else if(message.id() == nodeHandler->master_id){
			if(consensusSent == 0){
			nodeHandler->send();
			consensusSent = 1;
			}
		}
		else{
			if(nodeHandler->id == nodeHandler->master_id){
				nodeHandler->send();
			}
		}
	}
	else if(electionState == Cluster::ElectionState::ELEC_COMPLETE){
		if(nodeHandler->fsmstate == Cluster::NodeState::MASTER){
					nodeHandler->send();
		}
	}

}
*/

void ElectionManager::recvElectionComplete(){
	electionState = Cluster::ElectionState::ELEC_COMPLETE;
	nodeHandler->fsmstate = Cluster::NodeState::FULL;
	nodeHandler->master_id = message.master();
}

bool ElectionManager::checkifinList(google::protobuf::Map<uint32_t, uint32_t>* ptr){
	if(ptr->size() == 0){
		return false;
	}
	else{
		auto itera = ptr->find(nodeHandler->id);
		if( itera == ptr->end()){
			return false;
		}
		else{
			return true;
		}
	}
}

}





