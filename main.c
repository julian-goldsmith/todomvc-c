#include <string.h>
#include <assert.h>
#include <fcgiapp.h>
#include <jansson.h>
#include "env.h"

static int dump_json(const char* buffer, size_t size, void* data) {
	FCGX_Request* request = (FCGX_Request*) data;
	FCGX_PutStr(buffer, size, request->out);
	return 0;
}

static json_t* dumpenv_handler(const char** envp) {
	char key[256];
	json_t* items = json_array();

	for (const char** var = envp; *var != NULL; var++) {
		// Parse key.
		const char* key_start = *var;
		const char* key_end = strchr(key_start, '=');
		size_t key_len = key_end - key_start;

		if (!key_end || key_len >= sizeof(key))
			continue;

		// Ensure we have a null-terminated key for json_object_set_new.
		memcpy(key, key_start, key_len);
		key[key_len] = '\0';

		// Parse value.
		const char* value_start = key_end + 1;
		size_t value_len = strlen(*var) - key_len - 1;
		json_t* value = json_stringn(value_start, value_len);

		// Write key/value to JSON objects.  References are taken by _new methods.
		json_t* object = json_object();
		json_object_set_new(object, key, value);
		json_array_append_new(items, object);
	}

	return items;
}

static json_t* error_handler(const char* description) {
	const char* error_key = "error";
	json_t* value = json_string(description);
	json_t* object = json_object();
	json_object_set_new(object, error_key, value);
	return object;
}

static void* worker(void* param) {
	FCGX_Request request;
	int err, sockfd;

	sockfd = FCGX_OpenSocket("/tmp/fastcgi/rest.sock", 16);
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

		FCGX_FPrintF(request.out, "Content-Type: application/json\r\n\r\n");

		json_t* response;

		if (!strcmp(env->script_name, "/dumpenv"))
			response = dumpenv_handler((const char**) request.envp);
		else
			response = error_handler("Unknown request.");

		// Dump JSON value using dump_json, and free items.
		json_dump_callback(response, dump_json, &request, JSON_INDENT(4));
		json_decref(response);

		// Print trailing newline, and close out request.
		FCGX_FPrintF(request.out, "\r\n");
		FCGX_Finish_r(&request);
	}

	FCGX_Free(&request, 1);

	return NULL;
}

int main(int argc, char** argv) {
	if (FCGX_Init()) {
		printf("Unable to init FCGI.\n");
		return 1;
	}

	// TODO: Spawn threads.
	worker(NULL);

	FCGX_ShutdownPending();

	return 0;
}
