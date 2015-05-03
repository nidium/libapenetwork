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
	ape_array_t * array;

	array = NULL;
	array = ape_array_new(0);
	EXPECT_TRUE(array == NULL);
	
	ape_array_destroy(array);
}

TEST(Array, SimpleAdd)
{
	ape_array_t * array;
	ape_array_item_t *item;
	buffer *value;

	value = NULL;
	array = NULL;
	item = NULL;
	array = ape_array_new(1);
	EXPECT_TRUE(array != NULL);
	
	value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
	EXPECT_TRUE(value == NULL);
	item = ape_array_add(array, ARRAY_TEST_KEY_1, ARRAY_TEST_TEXT_1);
	EXPECT_TRUE(strcmp((char*)item->key->data, ARRAY_TEST_KEY_1) == 0);
	EXPECT_TRUE(strcmp((char*)item->pool.ptr.buf->data, ARRAY_TEST_TEXT_1) == 0);

	item = ape_array_add(array, ARRAY_TEST_KEY_2, ARRAY_TEST_TEXT_2);
	value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
	EXPECT_TRUE(strcmp((char*)value->data, ARRAY_TEST_TEXT_1) == 0);

	value = ape_array_lookup(array, (char *)item->key->data, item->key->used);
	EXPECT_TRUE(strcmp((char*)value->data, ARRAY_TEST_TEXT_2) == 0);

	ape_array_delete(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
	value = ape_array_lookup(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
	EXPECT_TRUE(value == NULL);
	
	ape_array_destroy(array);
}

TEST(Array, AddPtr)
{
	ape_array_t * array;
	buffer * buffer; //#, *b;
	ape_array_item_t *item;
	void *value;

	array = ape_array_new(10);
	buffer = buffer_new(strlen(ARRAY_TEST_KEY_1));
	buffer_append_string(buffer, ARRAY_TEST_KEY_1);

	ape_array_add_ptr(array, buffer, (void *) &ARRAY_TEST_TEXT_1);
	value = ape_array_lookup_data(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
	EXPECT_TRUE(strcmp((char*)value, ARRAY_TEST_TEXT_1) == 0);
	
	item = ape_array_lookup_item(array, ARRAY_TEST_KEY_1, strlen(ARRAY_TEST_KEY_1));
	EXPECT_TRUE(strcmp((char*)item->key->data, ARRAY_TEST_KEY_1) == 0);
	
	ape_array_destroy(array);
}
/*TODO
ape_array_lookup_cstr
ape_array_add_b
ape_array_add_ptr
ape_array_add_n
ape_array_add_camelkey_n
APE_A_FOREACH
*/
#undef ARRAY_TEST_TEXT_1
#undef ARRAY_TEST_TEXT_2
#undef ARRAY_TEST_KEY_1
#undef ARRAY_TEST_KEY_2
