#include <stdlib.h>

#define CJ_IMPLEMENTATION
#include "cj.h"

typedef struct {
    const char* name;
    int age;
}Person;

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

int main(void) {
    CJ* cj = cj_new(stdout, (CJ_write_t) fprintf);

    Person people[] = {
        { "Joe", 12 },
        { "Urmom", 122 },
        { "John", 22 },
    };

    dump_people(cj, 3, people);

    cj_delete(cj);
    return 0;
}
