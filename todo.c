#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "todo.h"

todorepo_t* repo = NULL;

todo_t* todo_create(int id, const char* title) {
	assert(title);

	todo_t* todo = (todo_t*) malloc(sizeof(todo_t));
	todo->id = id;
	todo->title = strdup(title);
	todo->completed = false;
	return todo;
}

void todo_set_title(todo_t* todo, const char* title) {
	if (strcmp(todo->title, title)) {
		free(todo->title);
		todo->title = strdup(title);
	}
}

void todo_set_completed(todo_t* todo, bool completed) {
	todo->completed = completed;
}

void todo_destroy(todo_t* todo) {
	assert(todo);

	if (todo->title) free(todo->title);
	free(todo);
}

todorepo_t* todorepo_create() {
	todorepo_t* repo = (todorepo_t*) malloc(sizeof(todorepo_t));
	repo->todos_capacity = 1024;
	repo->todos_len = 0;
	repo->curr_id = 1;
	repo->todos = (todo_t**) calloc(repo->todos_capacity, sizeof(todo_t*));
	return repo;
}

todo_t* todorepo_todo_create(todorepo_t* repo, const char* title) {
	int id = repo->curr_id++;
	todo_t* todo = todo_create(id, title);

	assert(repo->todos_len < repo->todos_capacity);
	repo->todos[repo->todos_len++] = todo;

	// FIXME: Expand list as needed.

	return todo;
}

todo_t* todorepo_get_by_id(todorepo_t* repo, int id) {
	for (todo_t** todos = repo->todos;
	     todos < repo->todos + repo->todos_len;
	     todos++) {
		todo_t* todo = *todos;
		if (todo->id == id) {
			return todo;
		}
	}
	return NULL;
}

bool todorepo_todo_delete(todorepo_t* repo, int id) {
	for (todo_t** todos = repo->todos;
	     todos < repo->todos + repo->todos_len;
	     todos++) {
		todo_t* todo = *todos;
		if (todo->id == id) {
			todo_destroy(todo);
			repo->todos_len--;

			// Swap with last todo.  If current todo is last, should be a NOP.
			if (repo->todos_len > 0) {
				todo_t* other = repo->todos[repo->todos_len];
				*todos = other;
				repo->todos[repo->todos_len] = NULL;
			}

			return true;
		}
	}
	return false;
}

void todorepo_destroy(todorepo_t* repo) {
	assert(repo->todos_capacity <= repo->todos_len);
	for (todo_t** todos = repo->todos;
	     todos < repo->todos + repo->todos_len;
	     todos++)
		free(*todos);
	free(repo);
}

json_t* todo_to_json(todo_t* todo) {
	assert(todo);
	json_t* jtodo = json_object();
	json_object_set_new(jtodo, "id", json_integer(todo->id));
	json_object_set_new(jtodo, "title", json_string(todo->title));
	json_object_set_new(jtodo, "completed", json_boolean(todo->completed));
	return jtodo;
}
