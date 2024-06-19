#include <stdlib.h>

#define CJ_IMPLEMENTATION
#include "cj.h"

typedef struct Node Node;
struct Node {
    int value;
    Node* left;
    Node* right;
};

Node* random_tree(size_t depth) {
    if (depth == 0) return NULL;
    Node* node = malloc(sizeof(*node));
    node->value = rand() % depth;
    node->left = random_tree(depth - 1);
    node->right = random_tree(depth - 1);
    return node;
}

void dump_tree(CJ* cj, Node* node) {
    if (node != NULL) {
        cj_begin_object(cj);

        cj_key(cj, "value");
        cj_number(cj, node->value);

        cj_key(cj, "left");
        dump_tree(cj, node->left);

        cj_key(cj, "right");
        dump_tree(cj, node->right);

        cj_end_object(cj);
    } else {
        cj_null(cj);
    }
}

int main(void) {
    CJ* cj = cj_new(stdout);

    Node* tree = random_tree(5);

    dump_tree(cj, tree);

    cj_delete(cj);

    return 0;
}
