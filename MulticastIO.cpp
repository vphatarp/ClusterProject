/*
 * MulticastIO.cpp
 *
 *  Created on: Dec 21, 2017
 *      Author: eebo
 */
#include "MulticastIO.h"

using namespace std;
namespace IO_Manager{


//Setting up the mcast socket
//End the program on failure
//Else print the success message
void MulticastIO::mcastSocket(){
	//Create the socket
	socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
	//Checking for errors
    if(socketDescriptor < 0){
    	perror("Error opening Datagram Socket: ");
        exit(1);
    }
    else{
        std::cout<< "Opened the datagram socket\n";
    }
}

void MulticastIO::setNonBlocking(){
	int flags; //Shall store the current value of the flags on the socket
	//Retrieving the values of the flags
	if ((flags = fcntl(socketDescriptor, F_GETFL, 0)) < 0){
		perror("Error getting flags: ");
	}
	//Setting the flag to Non Blocking
	flags |= O_NONBLOCK;
	//Checking for errors
	if(fcntl(socketDescriptor, F_SETFL, flags) < 0){
	    perror("Error setting flags for the socket: ");
	}
}

void MulticastIO::setBlocking(){
	int flags;
	if ((flags = fcntl(socketDescriptor, F_GETFL, 0)) < 0){
		perror("Error getting flags: ");
	}
	//Clearing the Non Blocking bit
	flags &= ~O_NONBLOCK;
	if(fcntl(socketDescriptor, F_SETFL, flags) < 0){
		perror("Error setting flags for the socket: ");
	}
}

//Since for the project multicast is going to be between nodes on the same machine
//All the nodes will be binding to the same address and hence the Reuse clause will be used
void MulticastIO::setReuse(){
        int reuse = 1;
        if(setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0){
                perror("Setting SO_REUSEADDR error");
                close(socketDescriptor);
                exit(1);
        }
        else{
                std::cout<<"Set reuse on the socket\n";
        }
}

void MulticastIO::createEpoll(){
	//Creating an epoll file descriptor and specifying events of interest
	epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (epollfd == -1){
		perror("Epoll Creation");
		exit(0);
	}
	eventdef.data.fd = socketDescriptor;
	eventdef.events = EPOLLIN | EPOLLET;

	//Add the socket to epoll
}

void MulticastIO::addEpoll(){
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketDescriptor, &eventdef) == -1){
		perror("Epoll CTL ADD");
		exit(0);
	}
	//Creating a buffer for storing the events coming in
	eventres = new epoll_event [4 * sizeof(eventdef)];
}

void MulticastIO::delEpoll(){
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, socketDescriptor, NULL) == -1){
		perror("Epoll CTL ADD");
		exit(0);
	}
}

void MulticastIO::disableLoopback(){
	// Disable loopback so you do not receive your own datagrams.
		char loopch = 0;
		if(setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
		{
			perror("Setting IP_MULTICAST_LOOP error");
			close(socketDescriptor);
			exit(1);
		}
		else{
			printf("Disabling the loopback...OK.\n");
		}
}

//The below function binds the listening port to the multicast address
//Also specifies the source address of the traffic it is interested in
void MulticastIO::bindListeningPort(std::string mcastaddr, std::string localaddr){
        if(socketbound == false){
        memset((char *) &listeningSocket, 0, sizeof(listeningSocket));
        listeningSocket.sin_family = AF_INET;
        listeningSocket.sin_port = htons(4321);
        listeningSocket.sin_addr.s_addr = INADDR_ANY;

        if(bind(socketDescriptor, (struct sockaddr*)&listeningSocket, sizeof(listeningSocket))){
                perror("Error binding the Listening Port");
                close(socketDescriptor);
                exit(1);
        }
        else{
                std::cout<<"Listening socket successfully set\n";
                socketbound = true;
        }
        }

        list_mcastaddr.imr_multiaddr.s_addr = inet_addr(mcastaddr.c_str());
        list_mcastaddr.imr_interface.s_addr = inet_addr(localaddr.c_str());
        if(setsockopt(socketDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&list_mcastaddr, sizeof(list_mcastaddr)) < 0){
                perror("Adding multicast group error");
                close(socketDescriptor);
                exit(1);
        }
        else{
                std::cout<<"Listening to the multicast group.\n";
        }
}

void MulticastIO::bindSendingPort(std::string mcastaddr, std::string exitaddr){
        memset((char *) &dest_mcastaddr, 0, sizeof(dest_mcastaddr));
        dest_mcastaddr.sin_family = AF_INET;
        dest_mcastaddr.sin_addr.s_addr = inet_addr(mcastaddr.c_str());
        dest_mcastaddr.sin_port = htons(4321);

        exitInterface.s_addr = inet_addr(exitaddr.c_str());
        if(setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *)&exitInterface, sizeof(exitInterface)) < 0){
          perror("Setting local interface error");
          exit(1);
        }
        else{
                std::cout<<"Local interface configured \n";
        }

}

bool MulticastIO::sendData(std::string message){
        if(sendto(socketDescriptor, message.c_str(), message.size()+1, 0, (struct sockaddr*)&dest_mcastaddr, sizeof(dest_mcastaddr)) < 0){
                perror("Sending datagram message error");
                return false;
        }
        else{
                std::cout<<"Sending datagram message...OK\n";
                return true;
        }
}

bool MulticastIO::recvData(){
	if(read(socketDescriptor, databuf, datalen) > 0){
		std::cout<<"Reading datagram message from client...OK\n";
        return true;
    }
	else if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
        return false;
    }
    else{
    	perror("Reading datagram message error\n");
        close(socketDescriptor);
        exit(1);
    }
}

bool MulticastIO::recvEpoll(int timer_ms){
	int noofevents;
	for(;;){
		noofevents = epoll_wait(epollfd, eventres, 4, timer_ms);
		if(noofevents > 0){
			for(int i=0;i<noofevents;i++){
				if(socketDescriptor == eventres[i].data.fd){
					return true;
				}
			}
		}
		else if(noofevents == 0){
			cout<<"Epoll timed out \n";
			return false;
		}
		else{
			if(errno == EINTR){
				continue;
			}
			else{
				exit(0);
			}
		}
	}
}


}


