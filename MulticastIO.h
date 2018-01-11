/*
 * MulticastIO.h
 *
 *  Created on: Dec 21, 2017
 *      Author: eebo
 */

#ifndef MULTICASTIO_H_
#define MULTICASTIO_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unordered_map>
#include <fcntl.h>
#include <error.h>
#include <sys/epoll.h>
#include<iostream>

namespace IO_Manager{

class MulticastIO{
	int epollfd;
    struct sockaddr_in listeningSocket;
    struct ip_mreq list_mcastaddr;
    struct in_addr exitInterface;
    struct sockaddr_in dest_mcastaddr;
    bool socketbound = false;
    struct epoll_event eventdef, *eventres;
public:
        int socketDescriptor;
        char databuf[1024];
        int datalen = sizeof(databuf);
public:
        void mcastSocket();
        void setNonBlocking();
        void setBlocking();
        void setReuse();
        void createEpoll();
        void addEpoll();
        void delEpoll();
        bool recvEpoll(int timer_ms);
        void disableLoopback();
        void bindSendingPort(std::string mcastaddr, std::string exitaddr);
        void bindListeningPort(std::string mcastaddr, std::string localaddr);
        bool sendData(std::string message);
        bool recvData();
};
}



#endif /* MULTICASTIO_H_ */
