#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pd.h"

#define PD_TAG_LEN 3
#define PD_DIGITS_LEN 6
#define PD_INFO_LEN (PD_DIGITS_LEN + PD_TAG_LEN)
#define PD_MAX_FULL_KEY_LEN (PD_MAX_GROUP_SIZE + PD_MAX_KEY_SIZE + 1) // + '.'
#define PD_MAX_LINE_LEN (PD_MAX_FULL_KEY_LEN + PD_MAX_VALUE_SIZE + 1) // + '='
#define PD_TAG_MARK ":T:"

pd_node_t *pd_alloc_node(const char *group, const char *key) {
    if (!key || !*key) {
        return NULL;
    }

    pd_node_t *node = malloc(sizeof(pd_node_t));
    if (group && *group) {
        strncpy(node->group, group, PD_MAX_GROUP_SIZE);
        node->group[PD_MAX_GROUP_SIZE] = '\0';
    } else {
        node->group[0] = '\0';
    }
    strncpy(node->key, key, PD_MAX_KEY_SIZE);
    node->key[PD_MAX_KEY_SIZE] = '\0';
    return node;
}

pd_node_t *pd_alloc_integer_node(const char *group, const char *key, long value) {
    pd_node_t *node = pd_alloc_node(group, key);
    if (node) {
        node->type = PD_NODE_INT;
        node->value.integer = value;
    }
    return node;
}

pd_node_t *pd_alloc_string_node(const char *group, const char *key, const char *value) {
    pd_node_t *node = pd_alloc_node(group, key);
    if (node) {
        node->type = PD_NODE_STR;
        if (value) {
            strncpy(node->value.string, value, PD_MAX_VALUE_SIZE);
            node->value.string[PD_MAX_VALUE_SIZE] = '\0';
        } else {
            node->value.string[0] = '\0';
        }
    }
    return node;
}

static pd_node_t *pd_parse_line(const char *p) {
    char *endptr = NULL;

    char length[8];
    char *t = strstr(p, PD_TAG_MARK);
    if (!t || (t - p) != PD_DIGITS_LEN) {
        return NULL;
    }
    strncpy(length, p, PD_DIGITS_LEN);
    length[PD_DIGITS_LEN] = '\0';
    size_t len = strtoul(length, &endptr, 10);
    if (endptr == length || *endptr != '\0' || len <= PD_INFO_LEN) {
        return NULL;
    }
    len -= PD_INFO_LEN;
    if (len > PD_MAX_LINE_LEN) {
        return NULL;
    }
    char content[PD_MAX_LINE_LEN + 1];
    strncpy(content, p + PD_INFO_LEN, len);
    content[len] = '\0';

    const char *equal = strchr(content, '=');
    if (!equal) {
        return NULL;
    }
    const size_t full_key_len = equal - content;
    if (full_key_len < 1 || full_key_len > PD_MAX_FULL_KEY_LEN || full_key_len > len) { // + '.'
        return NULL;
    }
    char full_key[PD_MAX_FULL_KEY_LEN + 1];
    strncpy(full_key, content, full_key_len);
    full_key[full_key_len] = '\0';
    char group[PD_MAX_GROUP_SIZE + 1];
    char key[PD_MAX_KEY_SIZE + 1];
    const char *dot = strchr(full_key, '.');
    if (dot) {
        const size_t group_len = dot - full_key;
        const size_t key_len = full_key_len - group_len - 1; // - '.'
        if (group_len < 1 || group_len > PD_MAX_GROUP_SIZE) {
            return NULL;
        }
        if (key_len < 1 || key_len > PD_MAX_KEY_SIZE) {
            return NULL;
        }
        strncpy(group, full_key, group_len);
        group[group_len] = '\0';
        strncpy(key, dot + 1, key_len);
        key[key_len] = '\0';
    } else {
        if (full_key_len > PD_MAX_KEY_SIZE) {
            return NULL;
        }
        group[0] = '\0';
        strcpy(key, full_key);
    }
    size_t value_len = len - full_key_len - 1;
    if (value_len > PD_MAX_VALUE_SIZE) {
        return NULL;
    }
    char value[PD_MAX_VALUE_SIZE + 1];
    strncpy(value, equal + 1, value_len);
    value[value_len] = '\0';

    pd_node_t *node = NULL;
    const long integer = strtol(value, &endptr, 10);
    if (endptr == value || *endptr != '\0') { // is not integer!
        node = pd_alloc_string_node(group, key, value);
    } else {
        node = pd_alloc_integer_node(group, key, integer);
    }
    return node;
}

int pd_read_file(const char *path, pd_node_t ***nodes) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }
#ifdef PD_TEST_MODE
    printf("Reading file: '%s'\n", path);
#endif
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (!size) {
        *nodes = realloc(*nodes, sizeof(pd_node_t *));
        (*nodes)[0] = NULL;
        fclose(file);
        return 0;
    }
#ifdef PD_TEST_MODE
    printf("Buffer: %lu bytes\n", size);
#endif
    char *buffer = malloc(size + 1);
    buffer[size] = '\0';
    if (fread(buffer, 1, size, file) != size) {
        free(buffer);
        fclose(file);
        return -2;
    };
    fclose(file);

    int line = 0, node_id = 0;
    char *p = buffer;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) {
            if (*p == '\n') {
                line++;
            }
            p++;
        }
        if (!*p) {
            break;
        }

        char *e = strchr(p, '\n');
        if (!e) {
            e = p + strlen(p);
        }
        line++;
        const size_t len = e - p;
        if (len > PD_INFO_LEN) {
            const char saved = *e;
            *e = '\0';
            pd_node_t *node = pd_parse_line(p);
            *e = saved;
            if (node) {
                pd_node_t **new_nodes = realloc(*nodes, sizeof(pd_node_t *) * (node_id + 2));
                if (!new_nodes) {
                    free(node);
                    pd_free_nodes(*nodes);
                    *nodes = NULL;
                    free(buffer);
                    return -0xA;
                }
                *nodes = new_nodes;
                (*nodes)[node_id++] = node;
                (*nodes)[node_id] = NULL;
#ifdef PD_TEST_MODE
                printf("Reading line %d: ", line);
                if (*(node->group)) {
                    printf("%s.", node->group);
                }
                printf("%s=", node->key);
                if (node->type == PD_NODE_INT) {
                    printf("%ld", node->value.integer);
                } else {
                    printf("%s", node->value.string);
                }
                printf("\n");
#endif
            } else {
                goto ERROR;
            }
        } else {
            ERROR:
                pd_free_nodes(*nodes);
                *nodes = NULL;
                free(buffer);
                return line;
        }
        if (!*e) {
            break;
        }
        p = e + 1;
    }
    free(buffer);
    return 0;
}

#ifndef PD_TEST_MODE
int pd_read_file_ws(const WSHDR *path, pd_node_t ***nodes) {
    char p[256 + 1];
    ws_2str(path, p, 256);
    return pd_read_file(p, nodes);
}
#endif

int pd_write_file(const char *path, const pd_node_t **nodes) {
    FILE *file = fopen(path, "w");
    if (!file) {
        return -1;
    }
    for (int i = 0; nodes[i] != NULL; i++) {
        const pd_node_t *node = nodes[i];
        char line[PD_MAX_LINE_LEN + 2]; //
        const size_t group_len = strlen(node->group);
        const size_t key_len = strlen(node->key);
        char value[PD_MAX_VALUE_SIZE + 1];
        if (node->type == PD_NODE_INT) {
            sprintf(value, "%ld", node->value.integer);
        } else {
            strcpy(value, node->value.string);
        }
        const size_t value_len = strlen(value);

        size_t line_len = key_len + value_len + PD_INFO_LEN + 1;
        if (group_len) {
            line_len += group_len + 1;
        }
        sprintf(line, "%06d:T:", (int)line_len);
        if (group_len) {
            strcat(line, node->group);
            strcat(line, ".");
        }
        strcat(line, node->key);
        strcat(line, "=");
        strcat(line, value);
#ifndef PD_TEST_MODE
        strcat(line, "\r");
        strcat(line, "\n");
        line_len += 2;
#else
        strcat(line, "\n");
        line_len += 1;
#endif
        if (fwrite(line, 1, line_len, file) != line_len) {
            fclose(file);
            return -2;
        }
#ifdef PD_TEST_MODE
        printf("Writing line %d: %s", i + 1, line);
#endif
    }
    fclose(file);
    return 0;
}

#ifndef PD_TEST_MODE
int pd_write_file_ws(const WSHDR *path, const pd_node_t **nodes) {
    char p[256 + 1];
    ws_2str(path, p, 256);
    return pd_write_file(p, nodes);
}
#endif

size_t pd_get_max_group_size() {
    return PD_MAX_GROUP_SIZE;
}

size_t pd_get_max_key_size() {
    return PD_MAX_KEY_SIZE;
}

size_t pd_get_max_value_size() {
    return PD_MAX_VALUE_SIZE;
}

size_t pd_get_size(pd_node_t **nodes) {
    int i = 0;
    while (nodes[i] != NULL) {
        i++;
    };
    return i;
}

void pd_add_node(pd_node_t ***nodes, pd_node_t *node) {
    if (!node) {
        return;
    }
    const size_t size = pd_get_size(*nodes);
    pd_node_t **new_nodes = realloc(*nodes, sizeof(pd_node_t *) * (size + 2));
    if (!new_nodes) {
        return;
    }
    new_nodes[size] = node;
    new_nodes[size + 1] = NULL;
    *nodes = new_nodes;
}

void pd_delete_node(pd_node_t ***nodes, size_t id) {
    pd_node_t **n = *nodes;
    const size_t size = pd_get_size(n);
    if (id >= size) {
        return;
    }
    pd_free_node(n[id]);
    for (size_t i = id; i < size; i++) {
        n[i] = n[i + 1];
    }
    pd_node_t **new_nodes = realloc(*nodes, sizeof(pd_node_t *) * (size));
    if (new_nodes) {
        *nodes = new_nodes;
    }
}

void pd_free_node(pd_node_t *node) {
    if (node) {
        free(node);
    }
}

void pd_free_nodes(pd_node_t **nodes) {
    if (nodes) {
        for (int i = 0; nodes[i] != NULL; i++) {
            free(nodes[i]);
        }
        free(nodes);
    }
}
