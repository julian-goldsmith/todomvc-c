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
	// FIXME: Implement this;
	if (strcmp(todo->title, title)) {
		free(todo->title);
		todo->title = strdup(title);
	}
}

void todo_set_completed(todo_t* todo, bool completed) {
	// FIXME: Implement this;
	todo->completed = completed;
}

void todo_destroy(todo_t* todo) {
	assert(todo);

	if (todo->title) free(todo->title);
	free(todo);
}

todorepo_t* todorepo_create() {
	PGconn* conn = PQconnectdb("");

	if (PQstatus(conn) == CONNECTION_BAD) {
		fprintf(stderr, "Failed to open connection.\n");
		PQfinish(conn);
		exit(1);				// FIXME: Do this better.
	}

	int ver = PQserverVersion(conn);
	fprintf(stderr, "Server version: %i.\n", ver);

	todorepo_t* repo = (todorepo_t*) malloc(sizeof(todorepo_t));
	repo->conn = conn;

	return repo;
}

todo_t* todorepo_todo_create(todorepo_t* repo, const char* title) {
	todo_t* todo = todo_create(0, title);

	// FIXME: Implement this;

	return todo;
}

todo_t* todorepo_get_by_id(todorepo_t* repo, int id) {
	// FIXME: Use actual parameters.
	char stmt[1024];
	sprintf(stmt, "select * from todos where id=%i", id);
	PGresult* res = PQexec(repo->conn, stmt);

	if (PQresultStatus(res) != PGRES_TUPLES_OK ||
	    PQntuples(res) < 1) {
		PQclear(res);
		return NULL;
	}

	// FIXME: Do this in a reasonable way.
	id = atoi(PQgetvalue(res, 0, 0));
	char* title = PQgetvalue(res, 0, 1);
	char* str_completed = PQgetvalue(res, 0, 2);
	bool completed = str_completed[0] == 't';

	todo_t* todo = todo_create(id, title);
	todo->completed = completed;
	PQclear(res);
	return todo;
}

bool todorepo_todo_delete(todorepo_t* repo, int id) {
	// FIXME: Implement this;
	/*
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
	*/
	return false;
}

void todorepo_destroy(todorepo_t* repo) {
	PQfinish(repo->conn);
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
