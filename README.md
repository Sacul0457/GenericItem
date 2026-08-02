# GenericItem
A data structure that can hold many dynamic types - Made in C!

## About
`GenericItem` is a data structure that can hold `ints`, `char` and even custom data structures!

Note that intrisics/SIMD are used and only machines with `AVX2` are supported at the moment.


## Data Structures/Types
These are the built-int types that `GenericItem` can hold:
    - `int`
    - `bool`
    - `char`
    - `char *`
    - `float`
    - `double`
    - `void *`

Custom Data Structures (found in `src`):
    - `Vector *`
    - `HashMap *`
    - `Set *`
    - `LinkedList *`
    - `Queue *`
    - `DLinkedList *`
    - `BST *`


### Example
Want to see this in action?

Here's a little JSON-Parser that I made using `GenericItem`.

Run this in the root directory which compiles the example:
```sh
make json_parser
```

Then execute it:
```sh
.\json_parser
```

See more at `examples`!