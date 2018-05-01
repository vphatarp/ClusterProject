# ClusterProject
A group of nodes having the same state information. For this particular project, Files are what would be maintained by each node. The Cluster has its own election mechanism and enables dynamic failover scenario.

Neighbor States:
1) Init:- The state a Node will be in, before quorum has reached.
2) Election:- Once quorum is reached, the node will take part in the election process.
3) Full:- Any node that is aware of an elected Master Node, but itself is not a Master node.
4) Backup:- Based on a certain criteria a Backup Node will be designated.
5) Master:- This node has been elected as a the node that would co-ordinate the state transfer and also inform all the other nodes of the health of neighbors.

Election States:
1) DISCOVER: The nodes havent reached Quorum yet.
2) ELEC_CONSENSUS: A master node has been identified, but yet to receive a message back from master. This is to ensure consistency.
3) ELEC_COMPLETE: Election has completed and the Master node has acknowledged all the other nodes that it would be taking up the role.

Election Mechanism:
In order to reduce the number of redundant messages, each node keeps a list of other nodes that they have currently heard from. 


