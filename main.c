#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <fcgiapp.h>
#include <jansson.h>
#include "env.h"
#include "todo.h"

static todorepo_t* repo = NULL;

static int dump_json(const char* buffer, size_t size, void* data) {
	FCGX_Request* request = (FCGX_Request*) data;
	FCGX_PutStr(buffer, size, request->out);
	return 0;
}

static size_t read_json(void* buffer, size_t buflen, void* data) {
	FCGX_Request* request = (FCGX_Request*) data;
	return FCGX_GetStr((char*) buffer, buflen, request->in);
}

static json_t* error_handler(const char* description, int desired_status, int* status) {
	const char* error_key = "error";
	json_t* value = json_string(description);
	json_t* object = json_object();
	json_object_set_new(object, error_key, value);
	*status = desired_status;
	return object;
}

static json_t* todos_post_handler(FCGX_Request* request, int* status) {
	json_error_t jerror;
	json_t* value = json_load_callback(read_json, request, 0, &jerror);
	if (!value) return error_handler("Unable to parse request.", 400, status);

	json_t* jtitle = json_object_get(value, "title");
	const char* title = json_string_value(jtitle);
	if (!title) return error_handler("Missing title.", 400, status);

	todo_t* todo = todorepo_todo_create(repo, title);
	json_decref(jtitle);
	json_decref(value);

	*status = 201;
	return todo_to_json(todo);
}

static json_t* todos_list_handler(int* status) {
	json_t* todo_list = json_array();

	for (todo_t** todos = repo->todos;
	     *todos && todos < repo->todos + repo->todos_capacity;
	     todos++) {
		todo_t* todo = *todos;
		json_t* jtodo = todo_to_json(todo);
		json_array_append_new(todo_list, jtodo);
	}

	*status = 200;
	return todo_list;
}

static json_t* todo_get_handler(int id, int* status) {
	todo_t* todo = todorepo_get_by_id(repo, id);
	if (!todo) return error_handler("Unknown item.", 404, status);

	*status = 200;
	return todo_to_json(todo);
}

static json_t* todos_handler(const env_t* env, FCGX_Request* request, int* status) {
	// Use a partial string comparison to determine we have an id.
	// We know the string must start with /todos .
	if (env->script_name[sizeof("/todos")]  == '\0' ||
	    env->script_name[sizeof("/todos/")] == '\0') {
		if (env->request_method == RM_POST) {
			return todos_post_handler(request, status);
		} else if (env->request_method == RM_GET) {
			return todos_list_handler(status);
		} else {
			return error_handler("Invalid request.", 400, status);
		}
	} else {
		const char* idpos = env->script_name + sizeof("/todos/") - 1;
		int id = atoi(idpos);				// 0 is an invalid todo id, so we can ignore errors.

		if (env->request_method == RM_GET) {
			return todo_get_handler(id, status);
		} else {
			return error_handler("Unimplemented.", 500, status);
		}
	}
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

		json_t* response;
		int status = 500;

		// Route request.
		if (!strncmp(env->script_name, "/todos", sizeof("/todos") - 1))		// Partial string comparison.
			response = todos_handler(env, &request, &status);
		else
			response = error_handler("Unknown request.", 404, &status);

		// Write headers.
		FCGX_FPrintF(request.out, "Content-Type: application/json; charset=utf-8\r\n");
		FCGX_FPrintF(request.out, "Status: %i\r\n", status);
		FCGX_FPrintF(request.out, "\r\n");

		// Dump JSON value using dump_json, and free response.
		json_dump_callback(response, dump_json, &request, JSON_INDENT(4));
		json_decref(response);

		// Print trailing newline, and close out request.
		FCGX_FPrintF(request.out, "\r\n");
		FCGX_Finish_r(&request);
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
