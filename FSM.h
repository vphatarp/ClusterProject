/*
 * FSM.h
 *
 *  Created on: Dec 30, 2017
 *      Author: root
 */

#ifndef FSM_H_
#define FSM_H_

namespace Cluster{


enum class PacketType{
	DISCOVER=1,
	ELEC_CONSENSUS,
	ELEC_COMPLETE,
	MOD_MASTER
};


enum class ElectionState{
	DISCOVER =1,
	ELEC_CONSENSUS,
	ELEC_COMPLETE
};

enum class NodeState{
	INIT =1,
	ELECTION,
	FULL,
	MASTER,
	BACKUP
};

enum class Error{
	INCORRECT_PACKET_TYPE = 1,
};
}




#endif /* FSM_H_ */
