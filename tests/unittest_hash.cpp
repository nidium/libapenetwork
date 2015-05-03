#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_hash.h>

#define KEY1 "1234567890123456"
#define KEY2 "2345678901234567"

unsigned long _ape_seed = 31415961;

TEST(Hash, MurmurHash)
{
	unsigned int hash;
	
	hash = MurmurHash2(KEY1, strlen(KEY1), _ape_seed);
	EXPECT_EQ(hash, 2247823904);
	hash = MurmurHash2(KEY2, strlen(KEY2), _ape_seed);
	EXPECT_EQ(hash, 282487095);
}
TEST(Hash, Hashstr)
{
	uint32_t hash;
	
	hash = ape_hash_str(KEY1, strlen(KEY1), 12);
	EXPECT_EQ(hash, 8);
	hash = ape_hash_str(KEY2, strlen(KEY2), 12);
	EXPECT_EQ(hash, 3);
}


TEST(Hash, HashTableSimple)
{
	uint32_t value;
	ape_htable_t *table = NULL;

	table = hashtbl_init(APE_HASH_INT);
	EXPECT_TRUE( table != NULL );

	hashtbl_append_val32(table, KEY1, strlen(KEY1), 123456);
	value = hashtbl_seek_val32(table, KEY1, strlen(KEY1));
	EXPECT_EQ(value, 123456);

	hashtbl_append_val32(table, KEY2, strlen(KEY2), 234567);
	value = hashtbl_seek_val32(table, KEY2, strlen(KEY2));
	EXPECT_EQ(value, 234567);

	hashtbl_erase(table, KEY2, strlen(KEY2));
	value = hashtbl_seek_val32(table, KEY2, strlen(KEY2));
	EXPECT_EQ(value, 0);

	hashtbl_erase(table, KEY1, strlen(KEY1));
	value = hashtbl_seek_val32(table, KEY1, strlen(KEY1));
	EXPECT_EQ(value, 0);

	hashtbl_free(table);
}

struct dummy_t{
	int a;
};

struct dummy_t * Dummy_New( int a )
{
	struct dummy_t * dummy;
	dummy = (struct dummy_t*) malloc( sizeof( * dummy) ); 
	dummy->a = a;
	return dummy;
}

void Dummy_Delete( ape_htable_item_t * item)
{
	struct dummy_t * dummy;

	dummy = ( struct dummy_t *) item->content.addrs;

	free(dummy);
}

TEST(Hash, HashTableKey64)
{
	struct dummy_t * value;
	ape_htable_t *table = NULL;

	table = hashtbl_init(APE_HASH_STR);
	EXPECT_TRUE( table != NULL );

	hashtbl_set_cleaner(table, Dummy_Delete);

	value = Dummy_New( 123456 );
	hashtbl_append64(table, 123, value);
	value = (struct dummy_t*) hashtbl_seek64(table, 123);
	EXPECT_EQ(value->a, 123456);

	value = Dummy_New( 234567);
	hashtbl_append64(table, 122, value);
	value = (struct dummy_t*) hashtbl_seek64(table, 122);
	EXPECT_EQ(value->a, 234567);


	hashtbl_erase64(table, 123);
	value = (struct dummy_t*) hashtbl_seek64(table, 123);
	EXPECT_TRUE(value == NULL );

	hashtbl_erase64(table, 122);
	value = (struct dummy_t*) hashtbl_seek64(table, 122);
	EXPECT_TRUE(value == NULL );

	hashtbl_free(table);
}


TEST(Hash, HashTableStr)
{
	struct dummy_t * value;
	ape_htable_t *table = NULL;

	table = hashtbl_init_with_size(APE_HASH_STR, 1);
	EXPECT_TRUE( table != NULL );

	hashtbl_set_cleaner(table, Dummy_Delete);

	value = Dummy_New( 123456 );
	hashtbl_append(table, KEY1, strlen(KEY1), value);
	value = (struct dummy_t*) hashtbl_seek(table, KEY1, strlen(KEY1));
	EXPECT_EQ(value->a, 123456);

	value = Dummy_New( 234567);
	hashtbl_append(table, KEY2, strlen(KEY2), value);
	value = (struct dummy_t*) hashtbl_seek(table, KEY2, strlen(KEY2));
	EXPECT_EQ(value->a, 234567);


	hashtbl_erase(table, KEY1, strlen(KEY1));
	value = (struct dummy_t*) hashtbl_seek(table, KEY1, strlen(KEY1));
	EXPECT_TRUE(value == NULL );

	hashtbl_erase(table, KEY2, strlen(KEY2));
	value = (struct dummy_t*) hashtbl_seek(table, KEY2, strlen(KEY2));
	EXPECT_TRUE(value == NULL );

	hashtbl_free(table);
}

#undef KEY1
#undef KEY2

