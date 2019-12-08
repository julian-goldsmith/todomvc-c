#ifndef HANDLERS_H
#define HANDLERS_H

#include "env.h"
#include "todo.h"

json_t* error_handler(const char* description, int desired_status, int* status);
json_t* todos_handler(const env_t* env, json_t* request_body, int* status);

#endif
