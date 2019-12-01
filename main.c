#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcgiapp.h>

static void* worker(void* param)
{
	FCGX_Request request;
	int err, sockfd;

	sockfd = FCGX_OpenSocket("/tmp/fastcgi/rest.sock", 16);
	if (sockfd < 0)
	{
		fprintf(stderr, "Unable to open socket.\n");
		exit(1);
	}

	err = FCGX_InitRequest(&request, sockfd, 0);
	if (err)
	{
		fprintf(stderr, "Failed to init request: %i\n", err);
		exit(1);
	}

	while (1)
	{
		err = FCGX_Accept_r(&request);
		if (err)
		{
			fprintf(stderr, "Failed to accept request: %i\n", err);
			continue;
		}

		FCGX_FPrintF(request.out, "Content-Type: text/plain\r\n\r\ntest\r\n");

		// Dump environment.
		for (char** var = request.envp; *var != NULL; var++)
		{
			FCGX_FPrintF(request.out, "%s\n", *var);
		}
		
		FCGX_Finish_r(&request);
	}

	FCGX_Free(&request, 1);

	return NULL;
}

int main(int argc, char** argv)
{
	if (FCGX_Init())
	{
		printf("Unable to init FCGI.\n");
		return 1;
	}

	// TODO: Spawn threads.
	worker(NULL);

	return 0;
}
