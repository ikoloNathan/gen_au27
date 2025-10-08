/*
 * ao_udp.c
 *
 *  Created on: 18 Feb 2025
 *      Author: Nathan Ikolo
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <winsock2.h>
#elif defined (__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
typedef int socket_t;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif
#define CLOSESOCKET close
#define GET_LAST_ERROR() errno
#define THREAD_RETURN void*
#define THREAD_CALLCONV
#endif

#include <ao_udp.h>
#include <broker.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SERVER_PORT 8912
#define BUFFER_SIZE 1024

#ifdef _WIN32
WSADATA wsa;
static SOCKET Socket;
#elif defined(__linux__)

static socket_t Socket = (socket_t) INVALID_SOCKET;
#endif
struct sockaddr_in clientAddr;
int clientAddrLen = sizeof(clientAddr);
char buffer[BUFFER_SIZE];

static void dispatch(base_obj_t *const me, const message_frame_t *frame);
#ifdef _WIN32
static unsigned __stdcall udp_recv_task(void *param);
#elif defined (__linux__)
static void* udp_recv_task(void *param);
#endif

udp_obj_t* udp_ctor(broker_t *broker, char *name) {
	static udp_obj_t *me;
	if (me == NULL) {
//		topic_config_t configs[1] = {
//				{ .type = MASK,
//				  .topic = BUILD_CAN_ID(0x01,0x00,system_id.value & 0x1F,MESSAGE_MODE_INTERNAL,0x02,0),
//				  .start = CAN_ID_SRC_ADDR(system_id.value & 0x1F)
//				}
//		};
		me = (udp_obj_t*) malloc(sizeof(udp_obj_t));
		memset(me, 0, sizeof(udp_obj_t));
		INIT_BASE(me, broker, name, system_id, NULL);
#ifdef _WIN32
		WSADATA wsa;
		SOCKET serverSocket;
#endif
		struct sockaddr_in serverAddr;

//		broker_subscribe(broker, configs, 1, (base_obj_t*) &me->super);
#ifdef _WIN32
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
		}

		if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
			printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
			WSACleanup();
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
		serverAddr.sin_port = htons(SERVER_PORT);

		if (bind(serverSocket, (struct sockaddr*) &serverAddr,
				sizeof(serverAddr)) == SOCKET_ERROR) {
			printf("Bind failed. Error Code: %d\n", WSAGetLastError());
			closesocket(serverSocket);
			WSACleanup();
		}
		Socket = serverSocket;
		printf("UDP Server listening on port %d...\n", SERVER_PORT);

		me->thread = (HANDLE)_beginthreadex(NULL, 0, udp_recv_task, broker, 0, NULL);
#elif defined (__linux__)
		/* Create UDP socket */
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr.sin_port = htons(SERVER_PORT);
		socket_t serverSocket = (socket_t) socket(AF_INET, SOCK_DGRAM, 0);
		if (serverSocket == INVALID_SOCKET) {
			fprintf(stderr, "Socket creation failed. Error: %d\n", GET_LAST_ERROR());
			free(me);
			return NULL;
		}
	    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
	        fprintf(stderr, "Bind failed. Error: %d\n", GET_LAST_ERROR());
	        CLOSESOCKET(serverSocket);
	#ifdef WIN32
	        WSACleanup();
	#endif
	        free(me);
	        return NULL;
	    }
		Socket = serverSocket;
		printf("UDP Server listening on port %d...\n", SERVER_PORT);
		if (pthread_create(&me->thread, NULL, udp_recv_task, broker) != 0) {
			fprintf(stderr, "pthread_create failed: %s\n", strerror(errno));
			CLOSESOCKET(Socket);
			Socket = (socket_t) INVALID_SOCKET;
			free(me);
			return NULL;
		}
#endif
	}
	return me;
}

#ifdef __WIN32
unsigned __stdcall udp_recv_task(void *param) {
	broker_t *broker = (broker_t*) param;
	while (1) {
		int recvLen = recvfrom(Socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &clientAddr, &clientAddrLen);

		message_frame_t frame = { 0 };
		uint8_t frame_size = recvLen > sizeof(message_frame_t) ? sizeof(message_frame_t) : recvLen;

		memcpy(&frame, buffer, frame_size);

		memset(buffer, 0, BUFFER_SIZE);
		if (broker != NULL) {
			broker_post(broker, frame, frame.priority != 3);
		}
	}
	return 0;
}
#elif defined (__linux__)
void* udp_recv_task(void *param) {
	broker_t *broker = (broker_t*) param;

	while (1) {
		int recvLen = (int) recvfrom(Socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &clientAddr, &clientAddrLen);
		if (recvLen <= 0) {
			/* ignore transient errors; a production impl may log and break */
			continue;
		}
		message_frame_t frame = (message_frame_t ) { 0 };
		size_t frame_size = (size_t) recvLen > sizeof(message_frame_t) ? sizeof(message_frame_t) : (size_t) recvLen;
		memcpy(&frame, buffer, frame_size);

		memset(buffer, 0, BUFFER_SIZE);
		if (broker != NULL) {
			broker_post(broker, frame, SECONDARY_QUEUE);
		}
	}

	return NULL;
}
#endif

void dispatch(base_obj_t *const me, const message_frame_t *frame) {

	sendto(Socket, (const char*) frame, sizeof(message_frame_t), 0, (struct sockaddr*) &clientAddr, clientAddrLen);

}

#ifdef __cplusplus
}
#endif
