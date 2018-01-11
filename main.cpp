/*
 * main.cpp
 *
 *  Created on: Sep 29, 2017
 *      Author: eebo
 */





#include <sys/epoll.h>
#include "MulticastIO.h"
#include "packetstruct.pb.h"
#include "ElectionManager.h"
#include "FSM.h"
#include "NodeHandler.h"
#include "PacketManager.h"



int main(int argc, char** argv){
	if(argc < 3){
		std::cout<< "The correct usage is ./server (id) (priority) \n";
	}
	//Setting the ID and the priority of the node
	Cluster::NodeHandler nhandler(atoi(argv[1]), atoi(argv[2]));

	//Starting the loop for carrying out elections
	nhandler.startElection();
	//if(nhandler.fsmstate == Cluster::NodeState::FULL){
	//	while(true){
	//		std::cout<<"ELECTION IS COMPLETE. NODE WITH ID "<<nhandler.master_id<<" IS THE MASTER NODE with "<<std::endl;
	//		sleep(3);
	//	}
	//}
	//nhandler.elecConsensus();
}
