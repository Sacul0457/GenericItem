#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "vector.h"
#include "hashmap.h"
#include "arena.h"
#include "complex_generic_item.h"


static inline char *
remove_whitespaces(char *string)
{
    assert(string != NULL);
    while (*string) {
        char curr_char = *string;
        if (curr_char == '\n' || curr_char == ' ' || curr_char == '\t' || curr_char == '\r') {
            string++;
        }
        else {
            break;
        }
    }
    return string;
}

static inline char *
parse_array(Arena *arena, char *string, GenericItem *ret);
static inline char *
parse_dict(Arena *arena, char *string, GenericItem *ret);

static inline char *
parse_string_value(Arena *arena, char *string, GenericItem *ret)
{
    assert(arena && string && ret);

    string++; // skip starting quote
    char *end_of_str = strchr(string, '"');
    if (end_of_str == NULL) {
        printf("SyntaxError: String (value) not closed\n");
        return false;
    }

    size_t str_length = (size_t)(end_of_str - string);
    char *buffer = arena_malloc(arena, str_length + 1);
    if (buffer == NULL) {
        return false;
    }

    memcpy(buffer, string, str_length);
    buffer[str_length] = '\0';
    *ret = convert_item(buffer);
    return end_of_str + 1;
}


static inline char *
parse_int_or_float_value(char *string, GenericItem *ret)
{
    assert(string && ret);
    char *end;

    double val = strtod(string, &end);

    if (*end == '\0') {
        printf("SyntaxError: Unexpected null-terminator\n");
        return NULL; // invalid input
    }

    double intpart;
    if (modf(val, &intpart) == 0.0) {
        *ret = convert_item((int)val);
    } else {
        *ret = convert_item(val);
    }

    return end;
}


static inline char *
parse_value(Arena *arena, char *string, GenericItem *ret)
{
    assert(arena && string && ret);
    switch (*string)
    {
        case '"': 
            return parse_string_value(arena, string, ret);
        case '{':
            return parse_dict(arena, string, ret);
        case '[':
            return parse_array(arena, string, ret);
        
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return parse_int_or_float_value(string, ret);

        case 't': {
            int length = 4;
            if (strncmp(string, "true", length) != 0) {
                return NULL;
            }
            *ret = convert_bool(true);
            return string + length;
        }
        case 'f': {
            int length = 5;
            if (strncmp(string, "false", length) != 0) {
                return NULL;
            }
            *ret = convert_bool(false);
            return string + length;
        }
        case 'n': {
            int length = 4;
            if (strncmp(string, "null", length) != 0) {
                return NULL;
            }
            *ret = convert_pointer(NULL);
            return string + length;
        }

        default:
            printf("SyntaxError: Expected (\", {, [ or integer), got '%c' instead\n", *string);
            return NULL;
    }
}


static inline char *
parse_dict(Arena *arena, char *string, GenericItem *ret)
{
    assert(arena && string && ret);
    string++; // skip '{'
    HashMap *dict = map_init(arena);
    if (dict == NULL) {
        return NULL;
    }

    // WE ARE ONLY ADVANCING THE LOCAL POINTER OF STRING
    while (true) {
        string = remove_whitespaces(string);
        if (*string == '}') {
            string++;
            break;
        }
        GenericItem key;
        string = parse_string_value(arena, string, &key);
        if (string == NULL) {
            try_free(arena, dict, sizeof(HashMap));
            return NULL;         
        }
        

        string = remove_whitespaces(string);
        if (*string != ':') {
            printf("SyntaxError: Expected ':', got '%c' instead\n", *string);
            return NULL;
        }
        string++; // skip ':'

        string = remove_whitespaces(string);
        if (*string == '\0') {
            printf("SyntaxError: Unexpected nul-terminator\n");
        }
        GenericItem value;
        string = parse_value(arena, string, &value);
        if (string == NULL) {
            try_free(arena, dict, sizeof(HashMap));
            return NULL;         
        }
        map_insert_item(arena, dict, key, value);

        string = remove_whitespaces(string);
        if (*string == ',') {
            string++;
            continue;
        }
        else if (*string == '}') {
            string++;
            break;
        }
        else {
            printf("SyntaxError: Expected '}' or ','\n");
            try_free(arena, dict, sizeof(HashMap));
            return NULL;
        }
    }

    *ret = convert_item(dict);
    return string;
}

static inline char *
parse_array(Arena *arena, char *string, GenericItem *ret)
{
    assert(arena && string && ret);
    string++; // skip '['
    Vector *array = vector_init(arena);
    if (array == NULL) {
        return NULL;
    }

    while (true) {
        string = remove_whitespaces(string);
        if (*string == ']') {
            string++;
            break;
        }
        GenericItem value;
        string = parse_value(arena, string, &value);
        if (string == NULL) {
            try_free(arena, array, sizeof(Vector));
            return NULL;
        }
        vector_append_item(arena, array, value);

        string = remove_whitespaces(string);
        if (*string == ',') {
            string++;
            continue;
        }
        else if (*string == ']') {
            string++;
            break;
        }
        else {
            printf("SyntaxError: Expected ']' or ','\n");
            return NULL;
        }
    }

    *ret = convert_item(array);
    return string;
}


static inline bool
json_str_to_item(Arena *arena, char *json_str, GenericItem *ret)
{
    if (json_str == NULL || ret == NULL) {
        return false;
    }

    json_str = remove_whitespaces(json_str);
    if (*json_str == '\0') {
        return false;
    }

    if (*json_str == '{') {
        if (!parse_dict(arena, json_str, ret)) {
            return false;
        }
    }
    else if (*json_str == '[') {
        if (!parse_array(arena, json_str, ret)) {
            return false;
        }
    }
    else {
        printf("Expected '{' or ']', got '%c' instead", *json_str);
        return false;
    }
    return true;
}


int main()
{   
    Arena *arena = arena_init();
    if (arena == NULL) {
        return -1;
    }

    FILE *file = fopen("examples/json_parser/example.json", "r");
    if (file == NULL) {
        goto cleanup;
    }

    fseek(file, 0, SEEK_END);
    size_t buffer_size = ftell(file);

    char *buffer = malloc(buffer_size + 1);
    if (buffer == NULL) {
        goto cleanup;
    }

    fseek(file, 0, SEEK_SET);
    size_t bytes_read= fread(buffer, 1, buffer_size, file);
    fclose(file);
    buffer[bytes_read] = '\0';

    GenericItem ret;
    json_str_to_item(arena, buffer, &ret);
    print_item(ret);

cleanup:
    arena_destroy(arena);
    return 0;

error:
    arena_destroy(arena);
    return -1;
}