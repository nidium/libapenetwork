/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_array.h>

#define ARRAY_TEST_TEXT_1 "Nidium"
#define ARRAY_TEST_TEXT_2 "A new breed of browser"

#define ARRAY_TEST_KEY_1 "K1"
#define ARRAY_TEST_KEY_2 "K2"

TEST(Array, Simple)
{
    ape_array_t *array;
    size_t size;

    array = NULL;

#if 0
    should NULL on 0
    size = 0;
    array = ape_array_new(size);
    EXPECT_TRUE(array == NULL);
#endif

    size  = 1;
    array = ape_array_new(size);
    EXPECT_TRUE(array != NULL);
    // EXPECT_EQ(array->pool.size, size);
    ape_array_destroy(array);
}

TEST(Array, SimpleAdd)
{
    ape_array_t *array;
    ape_array_item_t *item;
    buffer *value;

    value = NULL;
    array = NULL;
    item  = NULL;
    array = ape_array_new(1);
    EXPECT_TRUE(array != NULL);

    // should not find anything in a empty array
    value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(value == NULL);

    // should add someting
    item = ape_array_add(array, ARRAY_TEST_KEY_1, ARRAY_TEST_TEXT_1);
    EXPECT_TRUE(strcmp((char *)item->key->data, ARRAY_TEST_KEY_1) == 0);
    EXPECT_TRUE(strcmp((char *)item->pool.ptr.buf->data, ARRAY_TEST_TEXT_1)
                == 0);

    // should add another thing
    item  = ape_array_add(array, ARRAY_TEST_KEY_2, ARRAY_TEST_TEXT_2);
    value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(strcmp((char *)value->data, ARRAY_TEST_TEXT_1) == 0);

    // should find somenthing
    value = ape_array_lookup(array, (char *)item->key->data, item->key->used);
    EXPECT_TRUE(strcmp((char *)value->data, ARRAY_TEST_TEXT_2) == 0);

    // should delete something and not find it anymore
    ape_array_delete(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
    value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(value == NULL);

    // should add someting again (with_n)
    item = ape_array_add_n(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1),
                           ARRAY_TEST_TEXT_1, strlen(ARRAY_TEST_TEXT_1));
    EXPECT_TRUE(strcmp((char *)item->key->data, ARRAY_TEST_KEY_1) == 0);
    value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(strcmp((char *)value->data, ARRAY_TEST_TEXT_1) == 0);


    ape_array_destroy(array);
}

TEST(Array, cstr)
{
    ape_array_t *array;
    buffer *value;
    ape_array_item_t *item;

    array = ape_array_new(10);

    value = NULL;
    // find it if it is not thery yet?
    value = ape_array_lookup_cstr(array, ARRAY_TEST_KEY_1,
                                  strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(value == NULL);

    // add it and find it
    item = ape_array_add_camelkey_n(array, ARRAY_TEST_KEY_1,
                                    strlen(ARRAY_TEST_KEY_1), ARRAY_TEST_TEXT_1,
                                    strlen(ARRAY_TEST_TEXT_1));
    //@FIXME: this should not be needed!
    buffer_append_char(item->key, '\0');
    value = ape_array_lookup_cstr(array, (char *)item->key->data,
                                  strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(strcmp((char *)value->data, ARRAY_TEST_TEXT_1) == 0);

#if 0 
    //should not find it if it is not there anymore
    ape_array_delete(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
    value = ape_array_lookup_cstr(array, (char*)item->key->data, strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(value == NULL);
#endif

    ape_array_destroy(array);
}

TEST(Array, AddPtr)
{
    ape_array_t *array;
    buffer *buffer; //#, *b;
    ape_array_item_t *item;
    void *value;

    array  = ape_array_new(10);
    buffer = buffer_new(strlen(ARRAY_TEST_KEY_1));
    buffer_append_string(buffer, ARRAY_TEST_KEY_1);

    // should store a pointer
    ape_array_add_ptr(array, buffer, (void *)&ARRAY_TEST_TEXT_1);
    value = ape_array_lookup_data(array, ARRAY_TEST_KEY_1,
                                  strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(strcmp((char *)value, ARRAY_TEST_TEXT_1) == 0);

    // should find the pointer
    item = ape_array_lookup_item(array, ARRAY_TEST_KEY_1,
                                 strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(strcmp((char *)item->key->data, ARRAY_TEST_KEY_1) == 0);

    ape_array_destroy(array);
}

TEST(Array, AddB)
{
    ape_array_t *array;
    buffer *buffer, *b;
    ape_array_item_t *item;

    array  = ape_array_new(10);
    buffer = buffer_new(strlen(ARRAY_TEST_KEY_1));
    buffer_append_string(buffer, ARRAY_TEST_KEY_1);
    b = buffer_new(strlen(ARRAY_TEST_TEXT_1));
    buffer_append_string(b, ARRAY_TEST_TEXT_1);

    // should store a buffer
    item = ape_array_add_b(array, buffer, b);
    EXPECT_TRUE(item != NULL);

    // should find the pointer
    item = ape_array_lookup_item(array, ARRAY_TEST_KEY_1,
                                 strlen(ARRAY_TEST_KEY_1));
    EXPECT_TRUE(strcmp((char *)item->key->data, ARRAY_TEST_KEY_1) == 0);

    ape_array_destroy(array);
}


/*TODO
APE_A_FOREACH
*/

#undef ARRAY_TEST_TEXT_1
#undef ARRAY_TEST_TEXT_2
#undef ARRAY_TEST_KEY_1
#undef ARRAY_TEST_KEY_2
