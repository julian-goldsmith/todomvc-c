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
	PGconn* conn = PQconnectdb("");

	if (PQstatus(conn) == CONNECTION_BAD) {
		fprintf(stderr, "Failed to open connection.\n");
		PQfinish(conn);
		exit(1);				// FIXME: Do this better.
	}

	todorepo_t* repo = (todorepo_t*) malloc(sizeof(todorepo_t));
	repo->conn = conn;

	return repo;
}

todo_t* parse_todo_internal(PGresult* res, int id_num, int title_num, int completed_num) {
	int id = atoi(PQgetvalue(res, 0, id_num));
	const char* title = PQgetvalue(res, 0, title_num);
	const char* str_completed = PQgetvalue(res, 0, completed_num);
	bool completed = str_completed[0] == 't';

	todo_t* todo = todo_create(id, title);
	todo->completed = completed;
	return todo;
}

todo_t* parse_todo(PGresult* res) {
	int id_num = PQfnumber(res, "id");
	int title_num = PQfnumber(res, "title");
	int completed_num = PQfnumber(res, "completed");

	if (id_num < 0 || title_num < 0 || completed_num < 0) {
		return NULL;
	}

	return parse_todo_internal(res, id_num, title_num, completed_num);
}

todo_t* todorepo_create_todo(todorepo_t* repo, const char* title) {
	const char* stmt = "insert into todos(title, completed) "
		"values($1, false) returning id, title, completed";
	PGresult* res = PQexecParams(repo->conn, stmt, 1, NULL,
		(const char* const[]) { title }, NULL, NULL, 0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK ||
	    PQntuples(res) < 1) {
		PQclear(res);
		return NULL;
	}

	todo_t* todo = parse_todo(res);
	PQclear(res);
	return todo;
}

todo_t* todorepo_update_todo(todorepo_t* repo, todo_t* todo) {
	char id_str[10] = { 0 };
	snprintf(id_str, sizeof(id_str), "%i", todo->id);

	const char* completed_str = todo->completed ? "true" : "false";

	const char* stmt = "update todos "
		"set title=$2, completed=$3 "
		"where id = $1 "
		"returning id, title, completed";
	PGresult* res = PQexecParams(repo->conn, stmt, 3, NULL,
		(const char* const[]) { id_str, todo->title, completed_str },
		NULL, NULL, 0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK ||
	    PQntuples(res) < 1) {
		PQclear(res);
		return NULL;
	}

	todo_t* todo_new = parse_todo(res);
	PQclear(res);
	return todo_new;
}

todo_t* todorepo_get_todo_by_id(todorepo_t* repo, int id) {
	char id_param[32];
	sprintf(id_param, "%i", id);

	PGresult* res = PQexecParams(
		repo->conn, "select id, title, completed from todos where id=$1", 
		1, NULL, (const char* const[]) { id_param }, NULL, NULL, 0);


	if (PQresultStatus(res) != PGRES_TUPLES_OK ||
	    PQntuples(res) < 1) {
		PQclear(res);
		return NULL;
	}

	todo_t* todo = parse_todo(res);
	PQclear(res);
	return todo;
}

todo_t* todorepo_get_all_todos(todorepo_t* repo, size_t *num_todos) {
	assert(repo != NULL);
	assert(repo->conn != NULL);
	assert(num_todos != NULL);

	*num_todos = -1;

	PGresult *res = PQexecParams(
		repo->conn, "select id, title, completed from todos", 
		0, NULL, NULL, NULL, NULL, 0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		*num_todos = -1;
		return NULL;
	}

	int id_num = PQfnumber(res, "id");
	int title_num = PQfnumber(res, "title");
	int completed_num = PQfnumber(res, "completed");

	if (id_num < 0 || title_num < 0 || completed_num < 0) {
		PQclear(res);
		*num_todos = -1;
		return NULL;
	}

	*num_todos = PQntuples(res);
	todo_t *todos = (todo_t*) calloc(*num_todos, sizeof(todo_t));

	for (int i = 0; i < *num_todos; i++) {
		todo_t* todo = todos + i;

		todo->id = atoi(PQgetvalue(res, i, id_num));
		todo->title = strdup(PQgetvalue(res, i, title_num));

		const char* str_completed = PQgetvalue(res, i, completed_num);
		todo->completed = str_completed[0] == 't';
	}

	return todos;
}

bool todorepo_delete_todo(todorepo_t* repo, int id) {
	char id_str[10] = { 0 };
	snprintf(id_str, sizeof(id_str), "%i", id);

	const char* stmt = "delete from todos where id = $1";
	PGresult* res = PQexecParams(repo->conn, stmt, 1, NULL,
		(const char* const[]) { id_str },
		NULL, NULL, 0);

	ExecStatusType status = PQresultStatus(res);
	PQclear(res);
	return status == PGRES_COMMAND_OK;
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
