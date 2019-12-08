#include <errno.h>
#include <sys/stat.h>
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

	sockfd = FCGX_OpenSocket("/tmp/fastcgi/rest.sock", 32);
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
		err = FCGX_Accept_r(&request);
		if (err) {
			fprintf(stderr, "Failed to accept request: %i\n", err);
			continue;
		}

		env_t* env = env_parse((const char**) request.envp);
		assert(env && env->script_name);

		json_t* request_body = NULL;
		json_t* response;
		int status = 500;

		if (env->request_method == RM_POST || env->request_method == RM_PUT) {
			json_error_t jerror;
			request_body = json_load_callback(read_json, request.in, 0, &jerror);
			if (!request_body) return error_handler("Unable to parse request.", 400, &status);
		}

		// Route request.
		if (!strncmp(env->script_name, "/todos", sizeof("/todos") - 1))		// Partial string comparison.
			response = todos_handler(env, request_body, &status);
		else
			response = error_handler("Unknown request.", 404, &status);

		// Write headers.
		FCGX_FPrintF(request.out, "Content-Type: application/json; charset=utf-8\r\n");
		FCGX_FPrintF(request.out, "Status: %i\r\n", status);
		FCGX_FPrintF(request.out, "\r\n");

		// Dump JSON value using write_json, and free response.
		json_dump_callback(response, write_json, request.out, JSON_INDENT(4));
		json_decref(response);

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
	struct stat statbuf;
	int staterr = stat("/tmp/fastcgi", &statbuf);
	if (staterr == -EPERM)
		mkdir("/tmp/fastcgi", S_IRWXU | S_IRWXG);

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
