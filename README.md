# ClusterProject
A group of nodes having the same state information. For this particular project, Files are what would be maintained by each node. The Cluster has its own election mechanism and enables dynamic failover scenario.

Neighbor States:
1) Init:- The state a Node will be in, before quorum has reached.
2) Election:- Once quorum is reached, the node will take part in the election process.
3) Full:- Any node that is aware of an elected Master Node, but itself is not a Master node.
4) Backup:- Based on a certain criteria a Backup Node will be designated.
5) Master:- This node has been elected as a the node that would co-ordinate the state transfer and also inform all the other nodes of the health of neighbors.

# Election States:
1) DISCOVER: The nodes havent reached Quorum yet.
2) ELEC_CONSENSUS: A master node has been identified, but yet to receive a message back from master. This is to ensure consistency.
3) ELEC_COMPLETE: Election has completed and the Master node has acknowledged all the other nodes that it would be taking up the role.

# Election Mechanism:
There are 2 mechanisms via which the nodes exchange hello packets with each other.
Periodically: Hello packets are sent every 3 seconds, between the nodes.
Triggered: There are cases where a node sends a packet before the periodic interval in order to expedite the election process. For example: a new node that has joined the cluster. The current Master node or a candidate node will immediately respond back with a packet in the right Election state to bring it up to sync.
Assume the scenario wherein Node1 , Node2, Node3 enter a cluster one by one.(Quorum is 2) 
1) Node1 has sent out the hello packet based on the periodic interval, but hasnt heard back from any of the other nodes. Hence Node1 will be in INIT state and the Election state would be discovery.
2) Node2 enters the cluster and immediately based on its periodic interval sends out a hello packet. In response to the packet Node1 sends a triggered hello. Node2 and Node1, both are aware of each other's existence in the cluster, but since Quorum has still not been reached, The nodes remain in INIT state and Election state is Discovery. Since Node2 has a larger ID, it becomes the candidate for becoming the master node and has a list of all the nodes that it currently heard from.(This information is only held by candidate or master nodes)
3) When Node3 enters the cluster, Node2 sends out the packet with a list of all the nodes that are currently in the cluster. Since Node3 has a larger ID, it now becomes a candidate master, gets a download of all the nodes that are currently in the cluster from Node2. Since the nodes have reached Quorum, the nodes move from INIT to ELECTION state. The Election state for these nodes change from DISCOVER to ELEC_CONSENSUS. 
4) In ELEC_CONSENSUS state, the non-master nodes have to send out a message confirming that the master node is Node3. Election is considered complete:
   a) Only if all the nodes in the cluster have sent out an ELEC_CONSENSUS packet indicating that Node3 is the master node.
   b) And the master node has send them a packet back indicating that it has taken up the role, by sending out an ELEC_COMPLETE packet.

# Failovers(failover to backup node still needs to be worked on):
1) All the non-master nodes keep track of only the health of the Master node.
2) The master node keeps track of the health of all the non-master nodes.
Once a failover has been detected, all the nodes shall enter back into DISCOVER mode and start the election process again.
Everytime a non-master node dies, the Master node checks if the number of nodes have fallen below Quorum and accordingly either decides to stay in the same state or enter into ELEC_CONSENSUS state.


# Node Entering the Cluster after Election is complete
1) Pre-emption is not enabled. Hence a node that has entered after election is complete, will be sent a packet from the Master node and will sync up its state with the Master node and move to ELEC_COMPLETE and FULL state.

# Node in DISCOVER mode entering a Cluster which is currently in ELEC_CONSENSUS
1) If the ID of the node is higher, all the nodes will select this particular node as the master node.
2) If the ID of the node is lower, then the Master node will include it in the list of neighbors.


