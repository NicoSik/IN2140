/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "d2_lookup.h"
int checkAllocationError(void *client);
D2Client* d2_client_create( const char* server_name, uint16_t server_port )
{
    D2Client *client = (D2Client *)malloc(sizeof(D2Client));
    if(checkAllocationError(client)){
        return NULL;
    }
    
    client->peer = d1_create_client();
    if(checkAllocationError(client->peer)){
        free(client);
        return NULL;
    }
    if (!d1_get_peer_info(client->peer, server_name, server_port)) {
        free(client->peer); // Clean up on failure
        free(client);
        return NULL;
    }    
    return client;
}

D2Client* d2_client_delete( D2Client* client )
{
    if(!client){
        return NULL;
    }
    if(!client->peer){
        free(client);
        return NULL;
    }
    d1_delete(client->peer);
    free(client);
    return NULL;
}

int d2_send_request( D2Client* client, uint32_t id )
{
    PacketRequest* header = calloc(1,sizeof(PacketRequest)); //Allocate memory for the header
    if(checkAllocationError(header)){
        return 0;
    }
    header->id = id;
    header->type |= TYPE_REQUEST; //Set the type to request
    header->type = htons(header->type);
    header->id = htonl(header->id);
    int ret = d1_send_data(client->peer, (char*)header, sizeof(PacketRequest)); //Send the header from the client of our peer
    if(ret < 0){  //If the data was not sent, free the memory and return -1
        free(header);
        return -1;
    }
    free(header);
    return 1;
}

int d2_recv_response_size( D2Client* client )
{
    char buffer[1024];
    int ret = d1_recv_data(client->peer, buffer, 1024);
    if(ret < 0){
        return -1;
    }
    PacketHeader* header = (PacketHeader *)  buffer;
    if(ntohs(header->type) != TYPE_RESPONSE_SIZE){//Check if the type of the packet is correct
        perror("Wrong type of packet received, packet should be of type TYPE_RESPONSE_SIZE");
        return -1;
    }

    PacketResponseSize* response = (PacketResponseSize*)buffer; //Cast buffer to PacketResponseSize
    if(checkAllocationError(response)){
        return -1;
    }
    response->size = ntohs(response->size); //Change the size and type to host byte order 
    response->type = ntohs(response->type);
    return response->size;
}

int d2_recv_response( D2Client* client, char* buffer, size_t sz )
{
    if(sz< 1024){ //We have to check if the size of the buffer is 1024, which is the maximum size of the buffer 
        printf("Size of buffer should be 1024, bytes try sending again!");
        return-1;
    }
    int ret = d1_recv_data(client->peer, buffer, sz);
    if(ret < 0){
        return -1;
    }
    PacketHeader* header = (PacketHeader *)  buffer;
    if(ntohs(header->type) != TYPE_RESPONSE && ntohs(header->type) != TYPE_LAST_RESPONSE){ //Check if the type of the packet is correct 
        perror("Wrong type of packet received, packet should be of type TYPE_RESPONSE or TYPE_LAST_RESPONSE");
        return -1;
    }
    return ret;
}

LocalTreeStore* d2_alloc_local_tree( int num_nodes )
{
    LocalTreeStore *local_tree = calloc(1,sizeof(LocalTreeStore));
    if(checkAllocationError(local_tree)){
        return NULL;
    }
    local_tree->number_of_nodes = num_nodes; 
        local_tree->node = calloc(num_nodes, sizeof(NetNode));  //Callocates memory for the nodes    
       if(local_tree->node == NULL){ //If the memory was not allocated, free the memory and return NULL
          free(local_tree);
          local_tree = NULL;
       }

    return local_tree;
}


void  d2_free_local_tree( LocalTreeStore* nodes )
{
    if(!checkAllocationError(nodes)){
        if(!checkAllocationError(nodes->node)) {
           free(nodes->node);//frees the list with the nodes
           }
        free(nodes);//then frees the struct
    }
}

int d2_add_to_local_tree( LocalTreeStore* nodes_out, int node_idx, char* buffer, int buflen )
{
    uint16_t type = ntohs(*(uint16_t *)&buffer[0]);//Checks to see if type is last response or not    
    const char *buffer_start = buffer;
        for (size_t i = 0; i < 5; ++i) { // I have 5 since one buffer can contain up to 5 NetNodes
        // We cant directily cast the buffer to NetNode since the netnode size is kind of dynamic, with the children array
        // So we have to manually extract the data from the buffer
        uint32_t id = ntohl(*(uint32_t *)&buffer[0]);
        uint32_t value = ntohl(*(uint32_t *)&buffer[4]);
        uint32_t num_children = ntohl(*(uint32_t *)&buffer[8]);
        buffer += 12; // Move pointer past id, value, and num_children
        if (num_children > 0) {
            // Make sure there's enough data for all child_id entries
            if ((buffer - buffer_start) + (4 * num_children) > buflen) {
                break; // Not enough data left for all child_id entries
            }
            for (uint32_t j = 0; j < num_children; ++j) {
                uint32_t child_id = ntohl(*(uint32_t *)&buffer[j * 4]); //Gets the 
            nodes_out->node[node_idx].child_id[j]=child_id;     }
            buffer += 4 * num_children; // Move pointer past the child_id array when done reading the children ids
        }
        // Since i allerday allocated the tree and actually constructed the nodes i can just assign the values to the nodes
        nodes_out->node[node_idx].id = id;
        nodes_out->node[node_idx].value = value;
        nodes_out->node[node_idx].num_children= num_children;
        node_idx++;
        // Check if this is the last node (end of payload or TYPE_LAST_RESPONSE)
        if (type == TYPE_LAST_RESPONSE || (buffer - buffer_start) >= buflen) {
            break;
        }
    }
    return node_idx;


}
//Our recursive function to print the tree where we print the the nodes from an DFS perspective. Meaning that we print the the 1 child of the node, and then the child of that child and so on. 
void dfs_print_tree(LocalTreeStore* nodes_out, int node_index,int depth) {
   for (int i = 0; i < depth; i++) {
        printf("--"); // Print indentation for each level of depth
    }
    printf("id %d value %d children %d ", nodes_out->node[node_index].id, nodes_out->node[node_index].value, nodes_out->node[node_index].num_children);
    // Recursively print children with increased indentation
    for (uint32_t j = 0; j < nodes_out->node[node_index].num_children; j++) {
    // loop contents
        printf("\n"); 
        dfs_print_tree(nodes_out, nodes_out->node[node_index].child_id[j], depth + 1); // Recursively print children
    }
}

void d2_print_tree( LocalTreeStore* nodes_out )
{
    dfs_print_tree(nodes_out, 0,0);
 for (int i = 0; i < nodes_out->number_of_nodes; i++) {
        // Check if the current node has children
            printf("\nNode %d val %d -> ", nodes_out->node[i].id,nodes_out->node[i].value);
            int num_children = nodes_out->node[i].num_children;
            // If the current node has no children, skip to the next node
            if(nodes_out->node[i].num_children == 0){
                continue;
            }
            // Print children except the last one
            for (int j = 0; j < num_children - 1; j++) {
                printf("%d, ", nodes_out->node[i].child_id[j]);
            }
            // If there are more than one child, print "and"
            if (num_children > 1) {
                printf("and ");
            }
            // Print the last child
            printf("%d", nodes_out->node[i].child_id[num_children - 1]);
        
     }
    printf("\n");
}

