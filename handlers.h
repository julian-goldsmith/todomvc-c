#ifndef HANDLERS_H
#define HANDLERS_H

#include "env.h"
#include "todo.h"

int todos_handler(const env_t* env, json_t* request_body, json_t** response_body);

#endif
