#include <string.h>
#include "handlers.h"

static int todos_create_handler(json_t* request_body, json_t** response_body) {
	json_t* jtitle = json_object_get(request_body, "title");
	const char* title = json_string_value(jtitle);
	if (!title) return 400;

	todo_t* todo = todorepo_todo_create(repo, title);
	*response_body = todo_to_json(todo);
	return 201;
}

static int todos_list_handler(json_t** response_body) {
	json_t* todo_list = json_array();

	for (todo_t** todos = repo->todos;
	     todos < repo->todos + repo->todos_len;
	     todos++) {
		todo_t* todo = *todos;
		json_t* jtodo = todo_to_json(todo);
		json_array_append_new(todo_list, jtodo);
	}

	*response_body = todo_list;
	return 200;
}

static int todos_archive_handler() {
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

	return 204;
}

static int todo_get_handler(int id, json_t** response_body) {
	todo_t* todo = todorepo_get_by_id(repo, id);
	if (!todo) return 404;

	*response_body = todo_to_json(todo);
	return 200;
}

static int todo_update_handler(int id, json_t* request_body, json_t** response_body) {
	todo_t* todo = todorepo_get_by_id(repo, id);
	if (!todo) return 404;

	json_t* jtitle = json_object_get(request_body, "title");
	const char* title = json_string_value(jtitle);
	if (!title) return 400;

	json_t* jcompleted = json_object_get(request_body, "completed");
	bool completed = json_is_true(jcompleted);

	todo->completed = completed;
	todo_set_title(todo, title);

	*response_body = todo_to_json(todo);
	return 200;
}

static int todo_delete_handler(int id) {
	return todorepo_todo_delete(repo, id) ? 204 : 404;
}

int todos_handler(const env_t* env, json_t* request_body, json_t** response_body) {
	// Use a partial string comparison to determine we have an id.
	// We know the string must start with /todos .
	if (env->script_name[sizeof("/todos") - 1]  == '\0' ||
	    env->script_name[sizeof("/todos/") - 1] == '\0') {
		if (env->request_method == RM_POST) {
			return todos_create_handler(request_body, response_body);
		} else if (env->request_method == RM_GET) {
			return todos_list_handler(response_body);
		} else if (env->request_method == RM_DELETE) {
			return todos_archive_handler();
		} else {
			return 400;
		}
	} else {
		// FIXME: Handle trailing slashes.
		const char* idpos = env->script_name + sizeof("/todos/") - 1;
		int id = atoi(idpos);				// 0 is an invalid todo id, so we can ignore errors.

		if (env->request_method == RM_GET) {
			return todo_get_handler(id, response_body);
		} else if (env->request_method == RM_PUT) {
			return todo_update_handler(id, request_body, response_body);
		} else if (env->request_method == RM_DELETE) {
			return todo_delete_handler(id);
		} else {
			return 500;
		}
	}
}
