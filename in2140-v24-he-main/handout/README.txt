Introduction:
    This project is my implementation of a Directory Service as part of the IN2140 home exam. 
    The Directory Service is a key component of a distributed system, responsible for managing the network resources. 
    It provides a way for peers to locate resources on the network, making distributed computing possible.
    The communication in this project is based on the User Datagram Protocol (UDP), a simple, connectionless
    Internet protocol that allows for data transfer without the need for initial communication to establish a session. 
    This makes UDP ideal for situations where speed is a priority, such as in real-time applications or games.
    In this system, my "peer" communicates with a "black box" server. 
    The "black box" server is a component whose internal workings are not known to the peer. It sends and receives messages from the peer, acting as an intermediary between the peer and the network.
    This README will first provide a high-level overview of my implementation, explaining the key components and how they interact. 
    Following that, I will provide a detailed function list to give you a better understanding of the project's structure and functionality. 
    This will include descriptions of what each function does, the parameters it takes, and what it returns.

implementation:

    D1 implementation:
        This layer is pretty "simple" it finds the address, and makes it posssible to maintain the conncetion with your peer. It also sends and recives packets from said "connection".
        For my implementation of the D1 i started of with making a simple check for allocation fails. 
        Then i made a function to calculate the checksum which just iterates over the whole buffer and xors the 2 adjenct bits with each other. 
        I am not completly sure if thats how the task was descriped to be, but i interprtetd that it could be done this way. It also gives the same output checksums as the ones posted on the GitHub.
        Afterwards the rest of the D1 part was pretty straightforward, where I just used the explanation in the header file.
    
    D2 implementation:
        
        This is the upper layer of the program where we establish connections with the server by creating structs. To facilitate this process, we rely on the lower layer.
        The initial functions in D2 are straightforward; they involve sending or receiving messages and storing them in a buffer for later use.
        Given the volume of messages we anticipate receiving, our priority is to devise an efficient method for storing the information. This is where the LocalTreeStore struct comes in.
        One of the functions provides us with the exact number of nodes we should expect. Consequently, our LocalTreeStore struct features an array sized precisely to accommodate these nodes.
        When we now have room to store the nodes, we now have to make them from the packet through the d2_add_to_local_tree function. When we are done reading all the packets and hit last resposne, we are done reading.
        When we are done reading, we just have to print the nodes. I choose to do it the same way we have been shown in the server. With a DFS and one that just prints everything in order.

Function List:

    D1 FUNCTION LIST:

        findChecksum:
            Description: Computes the checksum of the provided data buffer.
            Usage: Call this function to calculate the checksum of a data buffer. 
            Parameters: 
                data: Pointer to the data buffer.
                sz: Size of the data buffer.
            Return Value: Returns the computed checksum as a uint16_t value.

        d1_create_client:
            Description: Creates and allocates memory for a D1 peer client, and creates a UDP socket for communication.
            Usage: Call this function to initialize a D1 peer client.
            Parameters: None
            Return Value: Returns a pointer to the newly created D1 peer client.

        d1_delete:
            Description: Deletes a D1 peer client and frees associated memory.
            Usage: Call this function to clean up and delete a D1 peer client.
            Parameters: Pointer to the D1 peer client to be deleted.
            Return Value: NULL on success, or the same pointer passed as the parameter if an error occurs.

        d1_get_peer_info:
            Description: Retrieves peer information based on the provided peer name and server port.
            Usage: Call this function to populate the peer structure with peer information.
            Parameters: Pointer to the D1 peer structure, peer name (IP address or hostname), and server port.
            Return Value: Returns 1 on success, 0 on failure.

        d1_recv_data:
            Description: Receives data from a peer, handles packet headers, and computes checksums.
            Usage: Call this function to receive data from a peer.
            Parameters: Pointer to the D1 peer structure, buffer to store received data, and size of the buffer.
            Return Value: Number of bytes received on success, -1 on failure.

        d1_wait_ack:
            Description: Waits for an acknowledgment from the peer after sending data, and handles retransmissions if necessary.
            Usage: Call this function after sending data to wait for the acknowledgment.
            Parameters: Pointer to the D1 peer structure, buffer containing the data sent, and size of the buffer.
            Return Value: 1 on acknowledgment received, -1 on failure.

        checkAllocationError:
            Description: Checks if memory allocation was successful and prints an error message if allocation fails.
            Usage: Helper function used to check memory allocation status.
            Parameters: Pointer to the allocated memory.
            Return Value: Returns 0 if allocation was successful, -1 if allocation failed.
            
        d1_send_data:
            Description: Constructs a data packet with a header, computes checksum, sends it to the specified peer, and waits for an acknowledgment.
            Usage: Call this function to send data to a peer using the D1 protocol.
            Parameters:
                peer: Pointer to the D1 peer structure representing the destination peer.
                buffer: Pointer to the buffer containing the data to be sent.
                sz: Size of the data buffer.
            Return Value: Returns the number of bytes sent on success, or -1 on failure.

        d1_send_ack:
            Description: Constructs an acknowledgment packet with a header and sends it to the specified peer.
            Usage: Call this function to send an acknowledgment to a peer.
            Parameters:
                peer: Pointer to the D1 peer structure representing the destination peer.
                seqno: Sequence number indicating the acknowledgment status (1 for acknowledgment, 0 otherwise).
            Return Value: None.

    D2 FUNCTION LISTS:

        d2_client_create:
            Description: Creates a new D2 client with the specified server name and port. It internally creates a D1 peer to handle communication with the server.
            Usage: Call this function to create a new D2 client.
            Parameters:
                server_name: The name or IP address of the server.
                server_port: The port number on which the server is listening.
            Return Value: Returns a pointer to the newly created D2 client on success, or NULL on failure.

        d2_client_delete:
            Description: Deletes a D2 client and its associated D1 peer. Frees memory allocated for the client and its peer.
            Usage: Call this function to delete a D2 client when it is no longer needed.
            Parameters:
                client: Pointer to the D2 client structure to be deleted.
            Return Value: Always returns NULL.

        d2_send_request:
            Description: Sends a request packet to the server with the specified request ID.
            Usage: Call this function to send a request to the server.
            Parameters:
                client: Pointer to the D2 client structure.
                id: The ID of the request.
            Return Value: Returns 1 on success, -1 on failure.

        d2_recv_response_size:
            Description: Receives the response size packet from the server, which contains the size of the response data.
            Usage: Call this function to receive the size of the response data from the server.
            Parameters:
                client: Pointer to the D2 client structure.
            Return Value: Returns the size of the response data on success, -1 on failure.

        d2_recv_response:
            Description: Receives response packets from the server.
            Usage: Call this function to receive response packets from the server.
            Parameters:
                client: Pointer to the D2 client structure.
                buffer: Pointer to the buffer to store the response data.
                sz: Size of the buffer.
            Return Value: Returns the number of bytes received on success, -1 on failure.

        d2_alloc_local_tree:
            Description: Allocates memory for a local tree store with the specified number of nodes.
            Usage: Call this function to allocate memory for a local tree store.
            Parameters:
                num_nodes: The number of nodes in the local tree.
            Return Value: Returns a pointer to the allocated LocalTreeStore structure on success, or NULL on failure.

        d2_free_local_tree:
            Description: Frees the memory allocated for a local tree store.
            Usage: Call this function to free the memory allocated for a local tree store.
            Parameters:
                nodes: Pointer to the LocalTreeStore structure to be freed.
            Note: This function first frees the memory allocated for the nodes in the local tree store before freeing the memory for the local tree store itself.
            Return Value: None.

        d2_add_to_local_tree:
            Description: Adds nodes to the local tree structure based on the data in the buffer.
            Usage: Call this function to populate the local tree structure with nodes.
            Parameters:
                nodes_out: Pointer to the LocalTreeStore structure where nodes will be added.
                node_idx: Index of the node where the addition will start.
                buffer: Pointer to the buffer containing the node data.
                buflen: Length of the buffer.
            Return Value: Returns the index of the next node in the local tree structure.

        d2_print_tree:

            Description: Prints the contents of the local tree structure in a depth-first manner.
            Usage: Call this function to print the local tree structure.
            Parameters:
                nodes_out: Pointer to the LocalTreeStore structure to be printed.
            Note: This function utilizes a recursive depth-first search (DFS) approach to print the tree.
            Return Value: None.

        dfs_print_tree:
            Description: Recursively prints the contents of a local tree structure in a depth-first manner, starting from the specified node index.
            Usage: Call this function to print the local tree structure in a depth-first manner.
            Parameters:
                nodes_out: Pointer to the LocalTreeStore structure containing the tree to be printed.
                node_index: Index of the starting node in the tree traversal.
                depth: Current depth of the tree traversal (used for indentation).
            Note: This function is called recursively to print each node and its children.
            Return Value: None.