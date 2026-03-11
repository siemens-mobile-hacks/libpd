#pragma once

#define PD_MAX_GROUP_SIZE 64
#define PD_MAX_KEY_SIZE 64
#define PD_MAX_VALUE_SIZE 128

typedef enum {
    PD_NODE_INT,
    PD_NODE_STR,
} PD_NODE_TYPE;

typedef PD_NODE_TYPE pd_node_type_t;

typedef struct {
    char group[PD_MAX_GROUP_SIZE + 1];
    char key[PD_MAX_KEY_SIZE + 1];
    pd_node_type_t type;
    union {
        long integer;
        char string[PD_MAX_VALUE_SIZE + 1];
    } value;
} PD_NODE;

typedef PD_NODE pd_node_t;

pd_node_t *pd_alloc_integer_node(const char *group, const char *key, long value);
pd_node_t *pd_alloc_string_node(const char *group, const char *key, const char *value);
int pd_read_file(const char *file_name, pd_node_t ***nodes);
int pd_write_file(const char *file_name, const pd_node_t **nodes);
void pd_free_node(pd_node_t *node);
void pd_free_nodes(pd_node_t **nodes);

int pd_get_max_group_size();
int pd_get_max_key_size();
int pd_get_max_value_size();
