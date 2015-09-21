#include "netlib.h"

/**
 * Set up everything we need to listed to the specified UDP port
 *
 * @return Socket file descriptor
 */
int listen_to_udp(char *portnum)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, portnum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(EXIT_FAILURE);
	}

	// Bind to the first address we can get
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			             p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		fcntl(sockfd, F_SETFL, O_NONBLOCK);

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(servinfo);

	return sockfd;
}

/**
 * Get UDP packet and put it into buf
 *
 * @return Number of bytes read
 */
int get_udp_packet(int sockfd, void *buf, int bufsiz,
                   struct sockaddr_storage *remote_addr)
{
	socklen_t addr_len;
	int numbytes;

	addr_len = sizeof(struct sockaddr_storage);
	
	if ((numbytes = recvfrom(sockfd, buf, bufsiz, 0,
	    (struct sockaddr *)remote_addr, &addr_len)) == -1) {

		// Could be that there was just no data on nonblocking sock?
		if (errno != (EAGAIN | EWOULDBLOCK)) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}

	}

	return numbytes;

}

/**
 * Helper to get the (printable) internet address
 *
 * @return Pointer to printable string
 */
void *get_in_addr(struct sockaddr_storage *sas)
{
	struct sockaddr *sa;
	sa = (struct sockaddr *)sas;

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

