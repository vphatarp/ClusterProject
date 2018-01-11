/*
 * ErrorHandler.cpp
 *
 *  Created on: Dec 30, 2017
 *      Author: root
 */


class ErrorHandler{
public:
	void operator () (int err, char* info){
		switch(err){
		case Cluster::Error::INCORRECT_PACKET_TYPE:
			std::cout<<"Incoming Packet of an unrecognized type from node "<<info<<"\n\n";
		}
	}
};

