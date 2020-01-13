#include <string.h>
#include <assert.h>
#include <fcgiapp.h>
#include "handlers.h"

static int write_json(const char* buffer, size_t size, void* data) {
	FCGX_Stream* out = (FCGX_Stream*) data;
	FCGX_PutStr(buffer, size, out);
	return 0;
}

static size_t read_json(void* buffer, size_t buflen, void* data) {
	FCGX_Stream* in = (FCGX_Stream*) data;
	return FCGX_GetStr((char*) buffer, buflen, in);
}

static void* worker(void* param) {
	FCGX_Request request;
	int err, sockfd;

	// TODO: IPv6?
	sockfd = FCGX_OpenSocket(":5050", 32);
	if (sockfd < 0) {
		fprintf(stderr, "Unable to open socket.\n");
		exit(1);
	}

	err = FCGX_InitRequest(&request, sockfd, 0);
	if (err) {
		fprintf(stderr, "Failed to init request: %i\n", err);
		exit(1);
	}

	while (1) {
		json_t* request_body = NULL;
		json_t* response_body = NULL;
		int status = 500;

		err = FCGX_Accept_r(&request);
		if (err) {
			fprintf(stderr, "Failed to accept request: %i\n", err);
			continue;
		}

		env_t* env = env_parse((const char**) request.envp);
		assert(env && env->script_name);

		// If we should have a request body, try to parse it.
		if (env->request_method == RM_POST || env->request_method == RM_PUT) {
			json_error_t jerror;
			request_body = json_load_callback(read_json, request.in, 0, &jerror);
			if (!request_body) status = 400;				// FIXME: Handle properly.
		}

		// Route request.
		if (!strncmp(env->script_name, "/todos", sizeof("/todos") - 1))		// Partial string comparison.
			status = todos_handler(env, request_body, &response_body);
		else
			status = 404;

		// Write headers.
		if (response_body) FCGX_FPrintF(request.out, "Content-Type: application/json; charset=utf-8\r\n");
		FCGX_FPrintF(request.out, "Status: %i\r\n", status);
		FCGX_FPrintF(request.out, "\r\n");

		// Dump JSON value using write_json, and free response.
		if (response_body) {
			json_dump_callback(response_body, write_json, request.out, JSON_INDENT(4));
			json_decref(response_body);
		}

		// Print trailing newline, and close out request.
		FCGX_FPrintF(request.out, "\r\n");
		FCGX_Finish_r(&request);

		// Release resources from request.  Request body must be released after 
		// response is written, because we keep references to strings in it.
		if (request_body) json_decref(request_body);
		env_destroy(env);
	}

	FCGX_Free(&request, 1);

	return NULL;
}

int main(int argc, char** argv) {
	if (FCGX_Init()) {
		printf("Unable to init FCGI.\n");
		return 1;
	}

	repo = todorepo_create();

	worker(NULL);

	FCGX_ShutdownPending();
	todorepo_destroy(repo);

	return 0;
}
