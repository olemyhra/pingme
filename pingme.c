#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define check_ebzero(x, y) if ((x) <= 0) {fprintf(stderr, (y)); exit(EXIT_FAILURE);}
#define check_bzero(x, y) if ((x) < 0) {fprintf(stderr, (y)); exit(EXIT_FAILURE);}
#define check_eone(x, y) if ((x) == 0) {fprintf(stderr, (y)); exit(EXIT_FAILURE);}

#define RECIEVE_BUFFER_SIZE 1024
#define RECV_TIMEOUT 5
#define TTL_TIME 56
#define MILLISECONDS 1000

struct options {
	uint8_t 			: 3;
	uint8_t no_option_selected	: 1;
	uint8_t display_raw_send	: 1;
	uint8_t display_raw_recieve	: 1;
	uint8_t ping_until_stop		: 1;
	uint8_t ping_range		: 1;
};

void show_usage();
int set_user_options(int argc, char **argv, struct options *user_selected_options);
void display_data(void *data, int size);

int main(int argc, char **argv) {

	struct options user_options = {0};
	
	set_user_options(argc, argv, &user_options);

	int end_device = 0;
	check_bzero(end_device = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP), 
		"pingme: unable to create socket!\n");

	struct sockaddr_in end_device_address = {0};
	end_device_address.sin_family = AF_INET;

	if (user_options.no_option_selected) {
		check_ebzero(inet_pton(AF_INET, argv[1],  &end_device_address.sin_addr.s_addr), 
			"pingme: invalid IP-address!\n");
	} else {
		check_ebzero(inet_pton(AF_INET, argv[2],  &end_device_address.sin_addr.s_addr), 
			"pingme: invalid IP-address!\n");
	}
	
	struct timeval recieve_timeout = {0};
	recieve_timeout.tv_sec = RECV_TIMEOUT;
	check_bzero(setsockopt(end_device, SOL_SOCKET, SO_RCVTIMEO, (void *) &recieve_timeout,  sizeof(recieve_timeout)),
		"pingme: unable to set socket recieve timeout value!");

	int ttl_time = TTL_TIME;
	check_bzero(setsockopt(end_device, IPPROTO_IP, IP_TTL, (void *) &ttl_time, sizeof(int)), 
		"pingme: unable to set IP TTL time!\n");

	struct icmphdr icmp_header = {0};
	icmp_header.type = ICMP_ECHO;
	icmp_header.code = 0;
	icmp_header.checksum = 0xf7a5;
	icmp_header.un.echo.id = 0;
	icmp_header.un.echo.sequence = 2130;

	uint8_t *recieve_buffer = (uint8_t *) calloc(RECIEVE_BUFFER_SIZE, sizeof(char));
 	do {
		int bytes_sent = 0;
		void *data = &icmp_header;
		struct timeval time_sent = {0};
		struct timeval time_recieved = {0};
	
		gettimeofday(&time_sent, NULL);
	
		bytes_sent = sendto(end_device, data, sizeof(icmp_header), 
			0, (struct sockaddr *) &end_device_address, sizeof(end_device_address));

		if(bytes_sent < 0) {
			perror("pingme: error: ");
			exit(EXIT_FAILURE);
		} else {
			if (user_options.display_raw_send) {
				printf("Tx: %d bytes (", bytes_sent);
				display_data(data, sizeof(icmp_header));
				printf(")\n");
			} else {
				printf("Sent %d bytes to %s:\n", bytes_sent, 
					(user_options.no_option_selected ? argv[1] : argv[2]));
			}
		}	

		int bytes_recieved = 0;
		struct sockaddr_in response_address = {0};
		socklen_t response_address_size = sizeof(response_address);
		
		bytes_recieved = recvfrom(end_device, recieve_buffer, (size_t) RECIEVE_BUFFER_SIZE,
			0, (struct sockaddr *) &response_address, &response_address_size);
	
		if (bytes_recieved > 0) {
			gettimeofday(&time_recieved, NULL);
			if (user_options.display_raw_recieve) {
				printf("Rx: %d bytes (", bytes_recieved);
				display_data((void *) recieve_buffer, bytes_recieved);
				printf(")\n");			
			} else {
				char response_address_string[INET_ADDRSTRLEN] = {0};
				inet_ntop(AF_INET, &(response_address.sin_addr), response_address_string, INET_ADDRSTRLEN);
				printf("Reply from %s, data:%d bytes, round trip time:%ld ms\n", response_address_string, 
					bytes_recieved, (time_recieved.tv_usec - time_sent.tv_usec) / MILLISECONDS);
			}
		} else {
			fprintf(stderr, "Timeout\n");
		}

		memset(recieve_buffer, 0, RECIEVE_BUFFER_SIZE);
		sleep(1);	

	} while (user_options.ping_until_stop);

	close(end_device);
	free(recieve_buffer);

	return 0;
}

void show_usage() {
	printf("Usage: pingme [options] <destination>\n\n");
	printf("Options:\n");
	printf("\t<destination>\tIP-address\n");
	printf("\t-a\tdisplay raw data sent to end device\n");
	printf("\t-b\tdisplay raw data recieved from end device\n");
	printf("\t-c\tping until stopped\n");
	printf("\t-d\tping a range of addresses\n");

}


int set_user_options(int argc, char **argv, struct options *user_selected_options) {
	int c = 0;	

	if (argc == 2) {
		user_selected_options->no_option_selected = 1;
		return 0;
	}
		

	while (--argc > 0 && (*++argv)[0] == '-') {
		while((c = *++argv[0])) {
			switch(c) {
			case 'a':
				user_selected_options->display_raw_send = 1;
				break;
			case 'b':
				user_selected_options->display_raw_recieve = 1;
				break;
			case 'c':
				user_selected_options->ping_until_stop = 1;
				break;
			case 'd':
				user_selected_options->ping_range = 1;
				break;
			default:
				printf("pingme: illegal option %c\n", c);
				exit(EXIT_FAILURE);
			}	
		}
	}
	if (argc != 1) {
		show_usage();
		exit(EXIT_FAILURE);
	}
		
	return 0;
}


void display_data(void *data, int size) {
	uint8_t *display_data = (uint8_t *) data;

	for (int i=0;i<size;i++)
		printf("0x%02X ", display_data[i]);

}
