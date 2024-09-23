/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "d1_udp.h"
#include <stddef.h>

#define MAX_BUFFER_SIZE 1024
#define D1HEADER_SIZE sizeof(struct D1Header)
uint16_t findChecksum(char *data, size_t sz);

int checkAllocationError(void *client){//A simple check to see if allocation was succesful, 
//gives 0 for no problem and -1 if the allocation was unsuccesfull
    if(!client){
        printf("Error: Could not allocate memory\n");
        return -1;
    }
    return 0;//Should maybe be positive integer instead?
}
D1Peer* d1_create_client( )//Creates and allocates memory for our client, then creates the socket for udp connection
{
    D1Peer *client = calloc(1,sizeof(D1Peer));
    if (checkAllocationError(client)){
        return NULL;
    }
    client -> socket = socket(AF_INET, SOCK_DGRAM, 0);
    return client;
}

D1Peer* d1_delete( D1Peer* peer )
{
    if(!peer){//Then the peer is already deleted
       return NULL; 
    }
    if(peer->socket<0){//Check if socket isnt closed already
        perror("socket is allerady closed!");
        free(peer);
        return NULL;
    }
    close(peer->socket);
    free(peer);//Closes the socket and frees the memory

    return NULL;
}

int d1_get_peer_info( struct D1Peer* peer, const char* peername, uint16_t server_port )
{
    if(checkAllocationError(peer) > 0){
        return 0;
    }
    peer->addr.sin_family = AF_INET;//Set the address family to IPv4
    peer->addr.sin_port = htons(server_port);
     if (inet_pton(AF_INET, peername, &(peer->addr.sin_addr)) == 1) { // Check if peername is an IP address
        printf("Peername is an IP address\n");
        return 1;
    }
    struct hostent* host_info = gethostbyname(peername);//If not an IP address, resolve the hostname
    if (host_info == NULL) {
        // Failed to resolve hostname
        perror("gethostbyname failed");
        return 0;
    }

    // Copy the resolved IP address from the host_info structure
    struct in_addr* host_addr = (struct in_addr*)host_info->h_addr;//Copy the resolved IP address from the host_info structure
    peer->addr.sin_addr = *host_addr; // Copy the resolved IP address from the host_info structure
    return 1;
}

int d1_recv_data( struct D1Peer* peer, char* buffer, size_t sz )
{   
    struct sockaddr_in sender_addr; 
    socklen_t sender_addr_len = sizeof(sender_addr);
    ssize_t bytes_received = recvfrom(peer->socket, buffer, sz, 0, (struct sockaddr *)&sender_addr, &sender_addr_len); // Receive data from the peer
    if (bytes_received == -1) {
        perror("recvfrom");
        return -1; 
    }
    D1Header* header = (D1Header *)  buffer; // Cast the first part of the buffer to a D1Header
    uint16_t seqno = 0;
    uint16_t header_sum = header->checksum;
    header->checksum=0;
    header->checksum = findChecksum(buffer,bytes_received); // Compute the checksum of the received packet
    header->size= ntohl (header->size);
    header->flags=ntohs(header->flags);
    if(header_sum != header->checksum || header->size !=bytes_received ){ // Check if the checksum and size are correct
        d1_send_ack(peer, !peer->next_seqno);
    }
    if (header->flags & FLAG_DATA) { 
        seqno = header->flags & SEQNO;// Call the function to send an ACK with the correct sequence number.
        d1_send_ack(peer, seqno);
        }
    memmove(buffer, buffer + sizeof(D1Header), bytes_received - sizeof(D1Header));
    bytes_received = bytes_received- sizeof(D1Header);
    return bytes_received; // Return number of bytes received
}


int d1_wait_ack( D1Peer* peer, char* buffer, size_t sz )
{
    /* This is meant as a helper function for d1_send_data.
     * When D1 data has send a packet, this one should wait for the suitable ACK.
     * If the arriving ACK is wrong, it resends the packet and waits again.
     *
     * Implementation is optional.
     */
   
    struct timeval timeout;
    timeout.tv_sec = 1;  // 5 seconds
    timeout.tv_usec = 0;
    struct sockaddr_in peer_addr;
    socklen_t peer_addrlen = sizeof(peer_addr);
    int ack_received = 0;
    if (setsockopt(peer->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) { // Set the timeout for the socket to wait 1 second
        perror("setsockopt failed");
        return -1;  // Error setting socket option
    }
    while (!ack_received) {
        // Receive ACK from the peer
       ssize_t bytes_received = recvfrom(peer->socket, buffer, sz, 0,(struct sockaddr *)&peer_addr, &peer_addrlen);
       D1Header * ack = (D1Header *) buffer;
       int ackflag = ntohs(ack->flags) & ACKNO;
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue; // Retry receiving
        }
        else if (peer->next_seqno == ackflag) { //If the sequence number of the ACK matches the next_seqno value in the D1Peer stucture
            ack_received = 1;
            peer->next_seqno = !peer->next_seqno;  // Toggle the next sequence number, when the ACK is received and successfull
            return 1;
        }
         else {
            //Is probably better to stop after x tries so that the program does not get stuck in an infinite loop if the ACK is never received
           d1_send_data(peer, buffer + sizeof(D1Header), sz - sizeof(D1Header));//Have to remove the header before trying to send the data again. Because we dont want to send the header 
           continue;//If the sequence number of the ACK does not match the next_seqno value in the D1Peer stucture, resend the packet and wait again.
        }
    }
    return -1;
}

int d1_send_data(D1Peer *peer, char *buffer, size_t sz) {
    D1Header* header = calloc(1, sizeof(D1Header));
    if(checkAllocationError(buffer) > 0){
        perror("buffer is empty");
        return -1;
    }
    if (!header) {
        perror("calloc failed");
        return -1;
    }
    
    size_t totSz = sizeof(D1Header) + sz;
    char* packet_buffer = (char *)malloc(totSz); // Allocate memory for the packet buffer with the size of the header and the data
    if (!packet_buffer) {
        free(header);
        perror("malloc failed");
        return -1;
    }
    header->flags = 0;
    // Construct the header and copy it to the packet buffer
    header->flags |= FLAG_DATA;
    if(peer->next_seqno){
        header->flags |= SEQNO;
    }
    header->checksum = 0;
    header ->flags = htons(header->flags);
    header->size = htonl(totSz);
    memcpy(packet_buffer, header, sizeof(D1Header));//Copy the header to the packet buffer
    memcpy(packet_buffer + sizeof(D1Header), buffer, sz);//then copy the data to the packet buffer from the index of the header
    // Compute checksum and assign it to the appropriate place in the packet buffer
    header->checksum = (findChecksum(packet_buffer, totSz));
    memcpy(packet_buffer + offsetof(D1Header, checksum), &(header->checksum), sizeof(header->checksum));//Copy the checksum to the packet buffer from the index of the header 

    if (totSz > MAX_BUFFER_SIZE) { //Check if the buffer is too large
        printf("Error: Buffer is too large\n");
        free(packet_buffer);
        free(header);
        return -1;
    }
    int bytes = sendto(peer->socket, packet_buffer, totSz, 0, (struct sockaddr *)&(peer->addr), sizeof(peer->addr)); // Send the packet to the peer
    if (bytes < 0) {
        perror("sendto failed");
        return -1;
    }
    int ack_status = d1_wait_ack(peer, packet_buffer, totSz); // Wait for an ACK from the peer
    if (ack_status < 0) {
        perror("d1_wait_ack failed");
        return -1;
    }

    free(packet_buffer);
    free(header);

    return bytes;
}

uint16_t findChecksum(char *data, size_t sz) { // Compute the checksum of the data
    uint16_t checksum = 0;
    for (size_t i = 0; i < sz; i += 2) { // Take two bytes at a time and xor them
        uint16_t mini = 0;
        if (i + 1 < sz) {
            mini = (data[i] & 0xff) | ((data[i + 1] & 0xff) << 8);//xor the two bytes
        } else {
            mini = data[i] & 0xff; // If there is only one byte left, xor it with 0
        }
        checksum ^= mini; 
    }
    return checksum;
}
void d1_send_ack(struct D1Peer* peer, int seqno)
{
    size_t sz = sizeof(D1Header);
    D1Header* header = calloc(1, sizeof(D1Header));
    if(checkAllocationError(header)){
        return;
    }
    uint16_t flags = FLAG_ACK; // Set the ACK flag
    if (seqno) {
        flags |= ACKNO ;// Set the sequence number bit if seqno is 1
    }
    header->flags = htons(flags);
    header->size = htonl(sz);//Put the size and flags in network byte order before sending
    header->checksum=findChecksum((char *)header ,sz);
    sendto(peer->socket, header, sz, 0, (struct sockaddr *)&(peer->addr), sizeof(peer->addr));
    // Dree the allocated header
    free(header);
}

