#include <string.h>
#include "handlers.h"

json_t* error_handler(int desired_status, int* status) {
	*status = desired_status;
	return NULL;
}

static json_t* todos_create_handler(json_t* request_body, int* status) {
	json_t* jtitle = json_object_get(request_body, "title");
	const char* title = json_string_value(jtitle);
	if (!title) return error_handler(400, status);

	todo_t* todo = todorepo_todo_create(repo, title);
	json_decref(jtitle);

	*status = 201;
	return todo_to_json(todo);
}

static json_t* todos_list_handler(int* status) {
	json_t* todo_list = json_array();

	for (todo_t** todos = repo->todos;
	     todos < repo->todos + repo->todos_len;
	     todos++) {
		todo_t* todo = *todos;
		json_t* jtodo = todo_to_json(todo);
		json_array_append_new(todo_list, jtodo);
	}

	*status = 200;
	return todo_list;
}

static json_t* todos_archive_handler(int* status) {
	int archive_ids[1024];					// TODO: Use a proper list.
	int archive_pos = 0;

	for (todo_t** todos = repo->todos;
	     todos < repo->todos + repo->todos_len;
	     todos++) {
		todo_t* todo = *todos;
		if (todo->completed && archive_pos < 1024)
			archive_ids[archive_pos++] = todo->id;
	}

	for (int i = 0; i < archive_pos; i++)
		todorepo_todo_delete(repo, archive_ids[i]);	// TODO: Error handling.

	*status = 204;
	return NULL;
}

static json_t* todo_get_handler(int id, int* status) {
	todo_t* todo = todorepo_get_by_id(repo, id);
	if (!todo) return error_handler(404, status);

	*status = 200;
	return todo_to_json(todo);
}

static json_t* todo_update_handler(int id, json_t* request_body, int* status) {
	todo_t* todo = todorepo_get_by_id(repo, id);
	if (!todo) return error_handler(404, status);

	json_t* jtitle = json_object_get(request_body, "title");
	const char* title = json_string_value(jtitle);
	if (!title) return error_handler(400, status);

	json_t* jcompleted = json_object_get(request_body, "completed");
	bool completed = json_is_true(jcompleted);

	todo->completed = completed;
	todo_set_title(todo, title);

	json_decref(jtitle);
	json_decref(jcompleted);

	*status = 200;
	return todo_to_json(todo);
}

static json_t* todo_delete_handler(int id, int* status) {
	bool deleted = todorepo_todo_delete(repo, id);
	if (!deleted) return error_handler(404, status);

	*status = 204;
	return NULL;
}

json_t* todos_handler(const env_t* env, json_t* request_body, int* status) {
	// Use a partial string comparison to determine we have an id.
	// We know the string must start with /todos .
	if (env->script_name[sizeof("/todos") - 1]  == '\0' ||
	    env->script_name[sizeof("/todos/") - 1] == '\0') {
		if (env->request_method == RM_POST) {
			return todos_create_handler(request_body, status);
		} else if (env->request_method == RM_GET) {
			return todos_list_handler(status);
		} else if (env->request_method == RM_DELETE) {
			return todos_archive_handler(status);
		} else {
			return error_handler(400, status);
		}
	} else {
		// FIXME: Handle trailing slashes.
		const char* idpos = env->script_name + sizeof("/todos/") - 1;
		int id = atoi(idpos);				// 0 is an invalid todo id, so we can ignore errors.

		if (env->request_method == RM_GET) {
			return todo_get_handler(id, status);
		} else if (env->request_method == RM_PUT) {
			return todo_update_handler(id, request_body, status);
		} else if (env->request_method == RM_DELETE) {
			return todo_delete_handler(id, status);
		} else {
			return error_handler(500, status);
		}
	}
}
