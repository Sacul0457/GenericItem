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
Want to see this in action? Here's a little JSON-Parser that I made using `GenericItem`.

First, run this in the root directory which compiles the example:
```sh
make json_parser
```

Then execute it:
```sh
.\json_parser
```

**See more [`examples`](examples/)!**
