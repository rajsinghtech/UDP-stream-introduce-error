#include <stdio.h> 
#include <string.h>    
#include <sys/socket.h>    
#include <stdlib.h>
#include "ccitt16.h"
#include "utilities.h"
#include "introduceerror.h"


void primary(int sockfd, double ber) {
	int read_size;
    char msg[100], srv_reply[150];
    int sequence[WINDOW_SIZE];
    struct packet_buffer pkt_buf[WINDOW_SIZE];

    //keep communicating with server
    while(1)
    {
        unsigned char send_base = 0;
        unsigned char last_sent = 0;
        unsigned char msg_idx = 0;
        unsigned char continueNum = 0;
        int read_data = 1;
        int re_sends = 0;

        memset(msg,0,100); // Clear contents of string
        msg_idx = 0;
        
        printf("Enter message : ");
		fgets(msg, 100 , stdin);
		
		while( read_data ) {
		
		    //printf(" Right hand side of condition %d\n", (strlen(msg) - 2 + continueNum));
        
            // Send some data to the receiver
		    // msg - the message to be sent
		    // strlen(msg) - length of the message
		    // 0 - Options and are not necessary here
		    // If return value < 0, an error occured
            // if( send(sockfd , msg, strlen(msg), 0) < 0)
            //    perror("Send failed");
            
            //send the first batch of items in the window
            while(last_sent <= send_base + WINDOW_SIZE && (msg[msg_idx] != 0)) {
                //printPacket(msg);
		        unsigned char data[PACKET_SIZE];
		        unsigned char buf[DATA_LENGTH + 1] = {'\0'};
		        int i, j;
		        
		        for (i = 0; i < DATA_LENGTH; i++) {
		            buf[i] = msg[msg_idx++];
		        }

		        //struct packet_buffer send_packet;
		        buildPacket(data, DATA_PACKET, buf, last_sent);
		        strcpy(pkt_buf[ last_sent - send_base ].packet, data);
		        
                printf("Sending new packet(%d)\n\n", last_sent);
                printPacket(pkt_buf[ last_sent - send_base ].packet);
		        IntroduceError(data, ber);
               
		        if(send(sockfd, data, sizeof(data), 0) < 0)
			        perror("Send failed\n");
			        
		        last_sent++;
            
            }
            printf("Last send %d, send base %d,\n", last_sent, send_base);
            //TODO: not stopping in time in testing. continues past the length of the message          
        
            //continue to send data if there is more to send in the window
            read_data = last_sent < send_base;
            if ( read_data && ((read_size = recv(sockfd , srv_reply , PACKET_SIZE , 0)) > 0) ){
            
                int crc = calculate_CCITT16(srv_reply, PACKET_SIZE, CHECK_CRC);
            
                if( (srv_reply[0] == ACK_PACKET) && (crc == CRC_CHECK_SUCCESSFUL) && contains(sequence, WINDOW_SIZE, srv_reply[1] ) && send_base != srv_reply[1] ){ // We are expecting ACK for the recieved packet, packet is not corrupted, and not a duplicate ACK
                    shiftWindow(sequence, WINDOW_SIZE, srv_reply[1] - send_base);
                    shiftBuf(pkt_buf, WINDOW_SIZE, srv_reply[1] - send_base);
                    send_base = srv_reply[1];
                    
                    printf("Recieved ACK(%d), shifting send base to %d\n\n", srv_reply[1], send_base);
                }
                
                //handle a negative aknowlegement
                if ( (srv_reply[0] == NAK_PACKET) || (crc == CRC_CHECK_SUCCESSFUL) ||  ( !contains(sequence, WINDOW_SIZE, srv_reply[1] ) ) ) { // Corrupted Data, explicit NAK, or duplicate ACK
                    int i, j, k;
                    
                    for (i = 0; i < last_sent - send_base; i++) {
                        re_sends++;
                        printf("Duplicate or NAK packet!\n");

                        printPacket(msg);
	                    unsigned char data[PACKET_SIZE];
	                    unsigned char buf[DATA_LENGTH + 1] = {'\0'};	                    
	                    strcpy(data, pkt_buf[i].packet);
	                    
	                    int k;
	                    for( k = 0; k < DATA_LENGTH + 1; k++){
	                        buf[k] = pkt_buf[i].packet[DATA_OFFSET + k];
	                    }
	                    
	                    buildPacket(data, pkt_buf[i].packet[0], buf, pkt_buf[i].packet[1]);
                        printf("Recieved NAK, re-sending packet(%d)\n\n", pkt_buf[i].packet[1]);
                        IntroduceError(data, ber);
                        if(send(sockfd, data, sizeof(data), 0) < 0)
			                perror("Re-Send failed for packet\n");
                    }
                } 
            } else if(read_size == -1) {
                perror("recv failed");
            }
        }
        printf("total number of re_sends - %d\n\n", re_sends);
        re_sends = 0;
        continueNum += last_sent - 1;
        msg_idx++;
    }
}
