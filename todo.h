#ifndef TODO_H
#define TODO_H

#include <jansson.h>
#include <stdbool.h>

typedef struct todo_s {
	int id;
	char* title;
	bool completed;
} todo_t;

typedef struct todorepo_s {
	todo_t** todos;
	size_t todos_capacity;
	size_t todos_len;
	int curr_id;
} todorepo_t;

todo_t* todo_create(int id, const char* title);
void todo_set_title(todo_t* todo, const char* title);
void todo_set_completed(todo_t* todo, bool completed);
json_t* todo_to_json(todo_t* todo);
void todo_destroy(todo_t* todo);

todorepo_t* todorepo_create();
todo_t* todorepo_todo_create(todorepo_t* repo, const char* title);
todo_t* todorepo_get_by_id(todorepo_t* repo, int id);
bool todorepo_todo_delete(todorepo_t* repo, int id);
void todorepo_destroy(todorepo_t* repo);

extern todorepo_t* repo;

#endif
