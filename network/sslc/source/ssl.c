#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <3ds.h>

#include "builtin_rootca_der.h"

static char http_netreq[] = "GET /testpage HTTP/1.1\r\nUser-Agent: 3ds-examples_sslc\r\nConnection: close\r\nHost: yls8.mtheall.com\r\n\r\n";

char readbuf[0x400];

void network_request(char *hostname)
{
	Result ret=0;

	struct addrinfo hints;
	struct addrinfo *resaddr = NULL, *resaddr_cur;
	int sockfd;

	sslcContext sslc_context;
	u32 RootCertChain_contexthandle=0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1)
	{
		printf("Failed to create the socket.\n");
		return;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	printf("Resolving hostname...\n");

	if(getaddrinfo(hostname, "443", &hints, &resaddr)!=0)
	{
		printf("getaddrinfo() failed.\n");
		closesocket(sockfd);
		return;
	}

	printf("Connecting to the server...\n");

	for(resaddr_cur = resaddr; resaddr_cur!=NULL; resaddr_cur = resaddr_cur->ai_next)
	{
		if(connect(sockfd, resaddr_cur->ai_addr, resaddr_cur->ai_addrlen)==0)break;
	}

	freeaddrinfo(resaddr);

	if(resaddr_cur==NULL)
	{
		printf("Failed to connect.\n");
		closesocket(sockfd);
		return;
	}

	printf("Running sslc setup...\n");

	ret = sslcCreateRootCertChain(&RootCertChain_contexthandle);
	if(R_FAILED(ret))
	{
		printf("sslcCreateRootCertChain() failed: 0x%08x.\n", (unsigned int)ret);
		closesocket(sockfd);
		return;
	}

	ret = sslcAddTrustedRootCA(RootCertChain_contexthandle, (u8*)builtin_rootca_der, builtin_rootca_der_size, NULL);
	if(R_FAILED(ret))
	{
		printf("sslcAddTrustedRootCA() failed: 0x%08x.\n", (unsigned int)ret);
		closesocket(sockfd);
		sslcDestroyRootCertChain(RootCertChain_contexthandle);
		return;
	}

	ret = sslcCreateContext(&sslc_context, sockfd, SSLCOPT_Default, hostname);
	if(R_FAILED(ret))
	{
		printf("sslcCreateContext() failed: 0x%08x.\n", (unsigned int)ret);
		closesocket(sockfd);
		sslcDestroyRootCertChain(RootCertChain_contexthandle);
		return;
	}

	ret = sslcContextSetRootCertChain(&sslc_context, RootCertChain_contexthandle);
	if(R_FAILED(ret))
	{
		printf("sslcContextSetRootCertChain() failed: 0x%08x.\n", (unsigned int)ret);
		sslcDestroyContext(&sslc_context);
		sslcDestroyRootCertChain(RootCertChain_contexthandle);
		closesocket(sockfd);
		return;
	}

	printf("Starting the TLS connection...\n");

	ret = sslcStartConnection(&sslc_context, NULL, NULL);
	if(R_FAILED(ret))
	{
		printf("sslcStartConnection() failed: 0x%08x.\n", (unsigned int)ret);
		sslcDestroyContext(&sslc_context);
		sslcDestroyRootCertChain(RootCertChain_contexthandle);
		closesocket(sockfd);
		return;
	}

	printf("Sending request...\n");

	ret = sslcWrite(&sslc_context, (u8*)http_netreq, strlen(http_netreq));
	if(R_FAILED(ret))
	{
		printf("sslcWrite() failed: 0x%08x.\n", (unsigned int)ret);
		sslcDestroyContext(&sslc_context);
		sslcDestroyRootCertChain(RootCertChain_contexthandle);
		closesocket(sockfd);
		return;
	}

	printf("Total sent size: 0x%x.\n", (unsigned int)ret);

	memset(readbuf, 0, sizeof(readbuf));

	ret = sslcRead(&sslc_context, readbuf, sizeof(readbuf)-1, false);
	if(R_FAILED(ret))
	{
		printf("sslcWrite() failed: 0x%08x.\n", (unsigned int)ret);
		sslcDestroyContext(&sslc_context);
		sslcDestroyRootCertChain(RootCertChain_contexthandle);
		closesocket(sockfd);
		return;
	}

	printf("Total received size: 0x%x.\n", (unsigned int)ret);

	printf("Reply:\n%s\n", readbuf);

	sslcDestroyContext(&sslc_context);
	sslcDestroyRootCertChain(RootCertChain_contexthandle);

	closesocket(sockfd);
}

int main()
{
	Result ret=0;
	u32 *soc_sharedmem, soc_sharedmem_size = 0x100000;

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("libctru sslc demo.\n");

	soc_sharedmem = memalign(0x1000, soc_sharedmem_size);
	if(soc_sharedmem==NULL)
	{
		printf("Failed to allocate SOC sharedmem.\n");
	}
	else
	{
		ret = socInit(soc_sharedmem, soc_sharedmem_size);

		if(R_FAILED(ret))
		{
			printf("socInit failed: 0x%08x.\n", (unsigned int)ret);
		}
		else
		{
			ret = sslcInit(0);
			if(R_FAILED(ret))
			{
				printf("sslcInit failed: 0x%08x.\n", (unsigned int)ret);
			}
			else
			{
				network_request("yls8.mtheall.com");
				sslcExit();
			}

			socExit();
		}
	}

	printf("Press START to exit.\n");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	return 0;
}
