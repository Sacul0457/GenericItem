#include "complex_generic_item.h"
#include "vector.h"
#include "hashmap.h"
#include "set.h"
#include "linked_list.h"
#include "queue.h"
#include "db_linked_list.h"


// ================================
//     COMPARE VALUE FUNCTIONS
// ================================

static inline int
compare_int_value(const GenericItem item, const GenericItem other)
{
    if (item.tp != other.tp) {
        return 0;
    }
    return item.data.integer == other.data.integer;
}

static inline int
compare_bool_value(const GenericItem item, const GenericItem other)
{   
    if (item.tp != other.tp) {
        return 0;
    }
    return item.data.boolean == other.data.boolean;
}

static inline int
compare_char_value(const GenericItem item, const GenericItem other)
{
    if (item.tp != other.tp) {
        return 0;
    }
    return item.data.character == other.data.character;
}

static inline int
compare_float_value(const GenericItem item, const GenericItem other)
{
    if (item.tp != other.tp) {
        return 0;
    }
    return item.data.ft == other.data.ft;
}

static inline int
compare_double_value(const GenericItem item, const GenericItem other)
{
    if (item.tp != other.tp) {
        return 0;
    }
    return item.data.db == other.data.db;
}

static inline int
compare_string_value(const GenericItem item, const GenericItem other)
{   
    if (item.tp != other.tp) {
        return 0;
    }
    // strcmp returns 0 when true
    return (strcmp(item.data.string, other.data.string) == 0);
}

static inline int
compare_pointer_value(const GenericItem item, const GenericItem other)
{   
    if (item.tp != other.tp) {
        return 0;
    }

    return item.data.pointer == other.data.pointer;
}


int
item_compare(const GenericItem item, const GenericItem other)
{
    switch (item.tp) {
        case INTEGER:
            return compare_int_value(item, other);

        case BOOLEAN_TYPE:
            return compare_bool_value(item, other);

        case CHARACTER:
            return compare_char_value(item, other);

        case FLOAT_TYPE:
            return compare_float_value(item, other);

        case DOUBLE_TYPE:
            return compare_double_value(item, other);

        case STRING:
            return compare_string_value(item, other);

        case POINTER:
            return compare_pointer_value(item, other);

        case VECTOR:
            return vector_compare(item.data.vector, other.data.vector);

        case MAP:
            return map_compare(item.data.map, other.data.map);

        case SET:
            return set_compare(item.data.set, other.data.set);

        case LINKED_LIST:
            return linked_list_compare(item.data.l_list, other.data.l_list);

        case QUEUE:
            return queue_compare(item.data.queue, other.data.queue);

        case DB_LINKED_LIST:
            return db_list_compare(item.data.db_list, other.data.db_list);
  
        default:
            return -1;
    }
}


void
print_item(const GenericItem item)
{
    switch (item.tp)
    {
        case INTEGER:
            printf("%d", item.data.integer);
            return;

        case BOOLEAN_TYPE:
            printf("%s", item.data.boolean ? "true" : "false");
            return;

        case CHARACTER:
            printf("%c", item.data.character);
            return;

        case FLOAT_TYPE:
            printf("%f", item.data.ft);
            return;

        case DOUBLE_TYPE:
            printf("%lf", item.data.db);
            return;
        
        case STRING:
            printf("\"%s\"", item.data.string);
            return;

        case POINTER:
            if (item.data.pointer == NULL) {
                printf("NULL");
            }
            else {
                printf("%p", item.data.pointer);
            }
            return;

        case VECTOR:
            vector_print(item.data.vector);
            return;

        case MAP:
            map_print(item.data.map);
            return;
            
        case SET:
            set_print(item.data.set);
            return;

        case LINKED_LIST:
            linked_list_print(item.data.l_list);
            return;

        case QUEUE:
            queue_print(item.data.queue);
            return;

        case DB_LINKED_LIST:
            dlinked_list_print(item.data.db_list);
            return;

        default:
            return;
    }
}