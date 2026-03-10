#include <stdio.h>
#include <stdlib.h>
#include <pd.h>

int main() {
    pd_node_t **nodes = NULL;
    int error = pd_read_file("../files/fak.pd", &nodes);
    if (!error) {
        printf("File parsed successfully!\n");
    } else {
        printf("Error parsing file at line %d\n", error);
    }

    error = pd_write_file("../files/out.pd", (const pd_node_t**)nodes);
    if (!error) {
        printf("File written successfully!\n");
    } else {
        printf("Failed to write file!\n");
    }
    pd_free_nodes(nodes);
    return 0;
}
