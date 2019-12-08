#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "env.h"

env_t* env_parse(const char** envp) {
	char key[256];
	env_t* env = (env_t*) calloc(1, sizeof(env_t));

	assert(envp);

	for (; *envp; envp++) {
		// Parse key.
		const char* key_start = *envp;
		const char* key_end = strchr(key_start, '=');
		size_t key_len = key_end - key_start;

		if (!key_end || key_len > 256) continue;

		// Ensure we have a null-terminated key for strcmp.
		memcpy(key, key_start, key_len);
		key[key_len] = '\0';

		// Parse value.
		const char* value_start = key_end + 1;
		size_t value_len = strlen(*envp) - key_len - 1;
		char* value = (char*) malloc(value_len + 1);
		memcpy(value, value_start, value_len);
		value[value_len] = '\0';

		// NOTE: This could be sped up by comparing key_len to constant string lengths.
		//       Also, by only allocating a new value when needed.
		if (!strcmp(key, "QUERY_STRING"))
			env->query_string = value;
		else if (!strcmp(key, "SCRIPT_NAME"))
			env->script_name = value;
		else if (!strcmp(key, "CONTENT_TYPE"))
			env->content_type = value;
		else {
			// These keys don't need to keep value.
			if (!strcmp(key, "REQUEST_METHOD")) {
				if (!strcmp(value, "GET"))
					env->request_method = RM_GET;
				else if (!strcmp(value, "POST"))
					env->request_method = RM_POST;
				else if (!strcmp(value, "PUT"))
					env->request_method = RM_PUT;
				else if (!strcmp(value, "DELETE"))
					env->request_method = RM_DELETE;
				else
					env->request_method = RM_INVALID;
			} else if (!strcmp(key, "CONTENT_LENGTH")) {
				env->content_length = atol(value);
			}

			free(value);
		}
	}

	return env;
}

void env_destroy(env_t* env) {
	if (!env) return;

	if (env->query_string) free(env->query_string);
	if (env->script_name) free(env->script_name);
	if (env->content_type) free(env->content_type);
	free(env);
}

