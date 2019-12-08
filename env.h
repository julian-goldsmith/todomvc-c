#ifndef ENV_H
#define ENV_H

#include <sys/types.h>

typedef enum {
	RM_INVALID,
	RM_GET,
	RM_POST,
	RM_PUT,
	RM_DELETE
} request_method_t;

typedef struct env_s {
	char* query_string;
	char* script_name;
	request_method_t request_method;
	char* content_type;
	size_t content_length;
} env_t;

env_t* env_parse(const char** envp);
void env_destroy(env_t* env);

#endif
