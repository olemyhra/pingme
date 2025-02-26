#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#define check_ebzero(x, y) if ((x) <= 0) {fprintf(stderr, (y)); exit(EXIT_FAILURE);}
#define check_bzero(x, y) if ((x) < 0) {fprintf(stderr, (y)); exit(EXIT_FAILURE);}
#define check_eone(x, y) if ((x) == 0) {fprintf(stderr, (y)); exit(EXIT_FAILURE);}

#define RECIEVE_BUFFER_SIZE 1024
#define RECV_TIMEOUT 5
#define TTL_TIME 56

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
		"Unable to create socket!\n");

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
		"Unable to set socket recieve timeout value!");

	int ttl_time = TTL_TIME;
	check_bzero(setsockopt(end_device, IPPROTO_IP, IP_TTL, (void *) &ttl_time, sizeof(int)), 
		"Unable to set IP TTL time!\n");

	struct icmphdr icmp = {0};
	icmp.type = ICMP_ECHO;
	icmp.code = 0;
	icmp.checksum = 0xf7a5;
	icmp.un.echo.id = 0;
	icmp.un.echo.sequence = 2130;

	
	int bytes_sent = 0;
	void *data = &icmp;
	bytes_sent = sendto(end_device, data, sizeof(icmp), 
		0, (struct sockaddr *) &end_device_address, sizeof(end_device_address));

	if(bytes_sent < 0) {
		perror("Error: ");
		exit(EXIT_FAILURE);
	} else {
		if (user_options.display_raw_send) {
			printf("TX: %d bytes (", bytes_sent);
			display_data(data, sizeof(icmp));
			printf(")\n");
		} else {
			printf("TX: %d bytes\n", bytes_sent);
		}
	}	

	int bytes_recieved = 0;
	uint8_t *recieve_buffer = (uint8_t *) calloc(RECIEVE_BUFFER_SIZE, sizeof(char));
	struct sockaddr_in response_address = {0};
	socklen_t response_address_size = sizeof(response_address);
	
	bytes_recieved = recvfrom(end_device, recieve_buffer, (size_t) RECIEVE_BUFFER_SIZE,
		0, (struct sockaddr *) &response_address, &response_address_size);
	if (bytes_recieved > 0) {
		if (user_options.display_raw_recieve) {
			printf("RX: %d bytes (", bytes_recieved);
			display_data((void *) recieve_buffer, bytes_recieved);
			printf(")\n");			
		} else {
			printf("RX: %d bytes\n", bytes_recieved);
		}
	} else {
		fprintf(stderr, "Timeout...exiting!\n");
		exit(EXIT_SUCCESS);
	}

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
