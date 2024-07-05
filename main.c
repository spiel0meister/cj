#include <stdlib.h>

#define CJ_IMPLEMENTATION
#include "cj.h"

typedef struct {
    const char* name;
    int age;
}Person;

typedef struct Node {
    struct Node* next;
    int value;
}Node;

void dump_people(CJ* cj, size_t n, Person people[n]) {
    cj_begin_array(cj);

    for (size_t i = 0; i < n; i++) {
        cj_begin_object(cj);
        cj_key(cj, "name");
        cj_string(cj, people[i].name);

        cj_key(cj, "age");
        cj_number(cj, people[i].age);

        cj_end_object(cj);
    }

    cj_end_array(cj);
}

void dump_nodes(CJ* cj, Node* root) {
    if (root == NULL) cj_null(cj);
    else {
        cj_begin_object(cj);

        cj_key(cj, "value");
        cj_number(cj, root->value);

        cj_key(cj, "next");
        dump_nodes(cj, root->next);

        cj_end_object(cj);
    }
}

Node* random_nodes(size_t n) {
    if (n == 0) return NULL;
    Node* node = malloc(sizeof(*node));
    node->value = n;
    node->next = random_nodes(n - 1);
    return node;
}

int main(void) {
    CJ* cj = cj_new(stdout, (CJ_write_t) fprintf);

#if 0
    Person people[] = {
        { "Joe\nMama", 12 },
        { "Urmom", 122 },
        { "John", 22 },
        { "Jill", 52 },
    };

    dump_people(cj, 4, people);
#else
    Node* root = random_nodes(10);
    dump_nodes(cj, root);
#endif

    cj_delete(cj);
    return 0;
}
