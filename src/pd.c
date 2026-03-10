#ifndef PD_TEST_MODE
    #include <swilib.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pd.h"

#define TAG_SIZE 3
#define LEN_DIGITS 6
#define INFO_SIZE (LEN_DIGITS + TAG_SIZE)
#define MAX_PAYLOAD_SIZE (PD_MAX_GROUP_SIZE + PD_MAX_KEY_SIZE + 1)
#define MAX_LINE_SIZE (MAX_PAYLOAD_SIZE + PD_MAX_VALUE_SIZE + 1)

pd_node_t *pd_alloc_node(const char *group, const char *key) {
    pd_node_t *node = malloc(sizeof(pd_node_t));
    strcpy(node->group, group);
    strcpy(node->key, key);
    return node;
}

pd_node_t *pd_alloc_integer_node(const char *group, const char *key, int value) {
    pd_node_t *node = pd_alloc_node(group, key);
    node->type = PD_NODE_INT;
    if (value) {
        node->value.integer = value;
    }
    return node;
}

pd_node_t *pd_alloc_string_node(const char *group, const char *key, const char *value) {
    pd_node_t *node = pd_alloc_node(group, key);
    node->type = PD_NODE_STR;
    if (value) {
        strcpy(node->value.string, value);
    }
    return node;
}

pd_node_t *pd_parse_line(const char *p) {
    char *endptr = NULL;

    char length[8];
    char *t = strstr(p, ":T:");
    if (!t || (t - p) != LEN_DIGITS) {
        return NULL;
    }
    strncpy(length, p, LEN_DIGITS);
    length[LEN_DIGITS] = '\0';
    size_t len = strtoul(length, &endptr, 10);
    if (endptr == length || *endptr != '\0' || len <= INFO_SIZE) {
        return NULL;
    }
    len -= INFO_SIZE;
    if (len > MAX_LINE_SIZE) { // + ".="
        return NULL;
    }
    char content[MAX_LINE_SIZE + 1]; // + ".=" + '\0'
    strncpy(content, p + INFO_SIZE, len);
    content[len] = '\0';

    const char *equal = strchr(content, '=');
    if (!equal) {
        return NULL;
    }
    size_t payload_len = equal - content;
    if (payload_len < 1 || payload_len > MAX_PAYLOAD_SIZE || payload_len > len) { // + '.'
        return NULL;
    }
    char payload[MAX_PAYLOAD_SIZE + 1]; // + '.' + '\0'
    strncpy(payload, content, payload_len);
    payload[payload_len] = '\0';
    char group[PD_MAX_GROUP_SIZE + 1];
    char key[PD_MAX_KEY_SIZE + 1];
    const char *dot = strchr(payload, '.');
    if (dot) {
        const size_t group_len = dot - payload;
        const size_t key_len = payload_len - group_len - 1; // - '.'
        if (group_len < 1 || group_len > PD_MAX_GROUP_SIZE) {
            return NULL;
        }
        if (key_len < 1 || key_len > PD_MAX_KEY_SIZE) {
            return NULL;
        }
        strncpy(group, payload, group_len);
        group[group_len] = '\0';
        strncpy(key, dot + 1, key_len);
        key[key_len] = '\0';
    } else {
        if (payload_len > PD_MAX_KEY_SIZE) {
            return NULL;
        }
        group[0] = '\0';
        strcpy(key, payload);
    }
    size_t value_len = len - payload_len - 1;
    if (value_len == 0 || value_len > PD_MAX_VALUE_SIZE) {
        return NULL;
    }
    char value[PD_MAX_VALUE_SIZE + 1];
    strncpy(value, equal + 1, value_len);
    value[value_len] = '\0';

    pd_node_t *node = NULL;
    const int integer = (int)strtol(value, &endptr, 10);
    if (endptr == value || *endptr != '\0') { // is not integer!
        node = pd_alloc_string_node(group, key, value);
    } else {
        node = pd_alloc_integer_node(group, key, integer);
    }
    return node;
}

int pd_read_file(const char *file_name, PD_NODE ***nodes) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        return -1;
    }
#ifdef PD_TEST_MODE
    printf("Reading file: '%s'\n", file_name);
#endif
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
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
        if (*p == '\0') {
            break;
        }

        char *e = strchr(p, '\n');
        if (!e) {
            e = p + strlen(p);
        }
        line++;
        const size_t len = e - p;
        if (len > INFO_SIZE) {
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
                if (strlen(node->group)) {
                    printf("%s.", node->group);
                }
                printf("%s=", node->key);
                if (node->type == PD_NODE_INT) {
                    printf("%d", node->value.integer);
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
        if (*e == '\0') {
            break;
        }
        p = e + 1;
    }
    free(buffer);
    return 0;
}

int pd_write_file(const char *file_name, const pd_node_t **nodes) {
    FILE *file = fopen(file_name, "w");
    if (!file) {
        return -1;
    }
    for (int i = 0; nodes[i] != NULL; i++) {
        const pd_node_t *node = nodes[i];
        char line[MAX_LINE_SIZE + 2]; //
        size_t group_len = strlen(node->group);
        size_t key_len = strlen(node->key);
        char value[PD_MAX_VALUE_SIZE + 1];
        if (node->type == PD_NODE_INT) {
            sprintf(value, "%d", node->value.integer);
        } else {
            strcpy(value, node->value.string);
        }
        size_t value_len = strlen(value);

        size_t line_len = key_len + value_len + INFO_SIZE + 1;
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

void pd_free_nodes(pd_node_t **nodes) {
    if (nodes) {
        for (int i = 0; nodes[i] != NULL; i++) {
            free(nodes[i]);
        }
        free(nodes);
    }
}
