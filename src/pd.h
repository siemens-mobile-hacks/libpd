#pragma once

#define PD_MAX_GROUP_SIZE 32
#define PD_MAX_KEY_SIZE 32
#define PD_MAX_VALUE_SIZE 32

typedef enum {
    PD_NODE_INT,
    PD_NODE_STR,
} PD_NODE_TYPE;

typedef PD_NODE_TYPE pd_node_type_t;

typedef struct {
    char group[PD_MAX_GROUP_SIZE];
    char key[PD_MAX_KEY_SIZE];
    pd_node_type_t type;
    union {
        int integer;
        char string[PD_MAX_VALUE_SIZE];
    } value;
} PD_NODE;

typedef PD_NODE pd_node_t;

int pd_read_file(const char *file_name, pd_node_t ***nodes);
int pd_write_file(const char *file_name, const pd_node_t **nodes);
void pd_free_nodes(pd_node_t **nodes);
