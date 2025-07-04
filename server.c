#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 100
#define NP_SEND "./server_to_client"
#define NP_RECEIVE "./client_to_server"

int main(void) {
	char receive_msg[BUFFER_SIZE], send_msg[BUFFER_SIZE];
	int receive_fd, send_fd;
	int value;

	/*---------------------------------------*/
	/* TODO 3 : make pipes and init fds      */
	/* (1) make NP_RECEIVE pipe              */
	/* (2) make NP_SEND pipe                 */
	/* (3) init receive_fd and send_fd       */

	//make client_to_server pipe
	if (mkfifo(NP_RECEIVE, 0666) == -1) return -1;

	//make server_to_client pipe
	if (mkfifo(NP_SEND, 0666) == -1) return -1;

	if((receive_fd = open(NP_RECEIVE, O_RDONLY)) == -1) return -1;

	if((send_fd = open(NP_SEND, O_WRONLY)) == -1) return -1;
	/* TODO 3 : END                          */
	/*---------------------------------------*/

	while (1) {
		/*---------------------------------------*/
		/* TODO 4 : read msg                     */
		
		
		ssize_t n = read(receive_fd, receive_msg, BUFFER_SIZE);

		if (n == -1) return -1;
		else if (n == 0) break;

		/* TODO 4 : END                          */
		/*---------------------------------------*/


		printf("server : receive %s\n", receive_msg);

		value = atoi(receive_msg);

		sprintf(send_msg, "%d", value*value);
		printf("server : send %s\n", send_msg);

		/*---------------------------------------*/
		/* TODO 5 : write msg                    */
		if (write(send_fd, send_msg, strlen(send_msg) + 1) == -1) return -1;

		/* TODO 5 : END                          */
		/*---------------------------------------*/
	}
	return 0;
}
