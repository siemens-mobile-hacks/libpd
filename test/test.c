#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pd.h>

pd_node_t **parse_file(const char *file_name) {
    pd_node_t **nodes = NULL;

    char path[128];
    sprintf(path, "../files/%s", file_name);
    int error = pd_read_file(path, &nodes);
    if (!error) {
        printf("File %s parsed successfully!\n", file_name);
    } else {
        printf("Error parsing %s file at line %d\n", file_name, error);
    }
    return nodes;
}

int main() {
    pd_node_t **nodes = NULL;
    nodes = parse_file("empty.pd");
    pd_free_nodes(nodes);
    nodes = parse_file("fak.pd");
    pd_free_nodes(nodes);
    nodes = parse_file("apidc_setup.pd");
    assert(pd_get_size(nodes) == 13);
    pd_free_nodes(nodes);
    nodes = parse_file("syncmlds.pd");
    assert(pd_get_size(nodes) == 15);
    pd_free_nodes(nodes);
    nodes = parse_file("userprofiles.pd");
    if (nodes) {
        int error = pd_write_file("../files/out.pd", (const pd_node_t**)nodes);
        if (!error) {
            printf("File written successfully!\n");
        } else {
            printf("Failed to write file!\n");
        }
        pd_free_nodes(nodes);
    }
    return 0;
}
