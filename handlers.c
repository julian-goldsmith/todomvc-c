#include "handlers.h"

json_t* error_handler(const char* description, int desired_status, int* status) {
	const char* error_key = "error";
	json_t* value = json_string(description);
	json_t* object = json_object();
	json_object_set_new(object, error_key, value);
	*status = desired_status;
	return object;
}

static json_t* todos_post_handler(json_t* request_body, int* status) {
	json_t* jtitle = json_object_get(request_body, "title");
	const char* title = json_string_value(jtitle);
	if (!title) return error_handler("Missing title.", 400, status);

	todo_t* todo = todorepo_todo_create(repo, title);
	json_decref(jtitle);

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

json_t* todos_handler(const env_t* env, json_t* request_body, int* status) {
	// Use a partial string comparison to determine we have an id.
	// We know the string must start with /todos .
	if (env->script_name[sizeof("/todos") - 1]  == '\0' ||
	    env->script_name[sizeof("/todos/") - 1] == '\0') {
		if (env->request_method == RM_POST) {
			return todos_post_handler(request_body, status);
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
