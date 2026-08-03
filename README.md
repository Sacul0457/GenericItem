# About
`GenericItem` is a data structure that can hold `int`, `char`... and even custom data structures!
This project was mainly made for my learning but it's actually very useful.

> [!IMPORTANT]
> Intrisics/SIMD are used and only machines with `AVX2` are supported at the moment. (Although most x86-64 machines should)


## Data Structures/Types
These are the built-int types that `GenericItem` can hold:
- `int`
- `bool`
- `char`
- `char *`
- `float`
- `double`
- `void *`

Custom Data Structures (found in [`src`](src/) & [`include`](include/)):
- `Vector *`
- `HashMap *`
- `Set *`
- `LinkedList *`
- `Queue *`
- `DLinkedList *`
- `BST *`


## Example
Here's a little example of how you can use it
```c
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "complex_generic_item.h"
#include "arena.h"
#include "vector.h"
#include "hashmap.h"

int main() {
    Arena *arena = arena_init();
    if (arena == NULL) {
        return -1;
    }

    Vector *vector = vector_init(arena);
    if (vector == NULL) {
        goto cleanup;
    }

    vector_append_item(arena, vector, convert_item(10));
    vector_append_item(arena, vector, convert_item("Sacul"));

    HashMap *map = map_init(arena);
    if (map == NULL) {
        goto cleanup;
    }
    map_insert_item(arena, map, convert_item("spider"), convert_item("man"));
    vector_append_item(arena, vector, convert_item(map));

    print_item(convert_item(vector)); // [10, "Sacul", {"spider": "man", }, ]
cleanup:
    arena_destroy(arena);
}
```

Want a more complex use case? Here's a little JSON-Parser that I made using `GenericItem`!

First, run this in the root directory which compiles the example:
```sh
make json_parser
```

Then execute it:
```sh
.\json_parser
```

**See more [`examples`](examples/)!**
