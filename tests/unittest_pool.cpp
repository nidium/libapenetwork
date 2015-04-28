#include <string.h>

#include <gtest/gtest.h>

#include <ape_pool.h>

TEST(Pool, Simple)
{
	ape_pool_t * pool;

	pool = NULL;

	pool = ape_new_pool( 16, 2 );
	EXPECT_TRUE(pool != NULL);
	EXPECT_TRUE(pool->next != NULL);
	EXPECT_TRUE(pool->next->next == NULL);
	EXPECT_TRUE(pool->prev == NULL);
	EXPECT_TRUE(pool == pool->next->prev);

	ape_destroy_pool( pool );
}

TEST(Pool, Looped)
{
#define SIZE_START 16
#define SIZE_LIMIT 512
#define SIZE_STEP 16 
#define N_START 1
#define N_LIMIT 128
#define N_STEP 16 
	ape_pool_t * pool;
	size_t size, n;

	pool = NULL;
	pool = ape_new_pool(0, 1);
	EXPECT_TRUE(pool != NULL);
	ape_destroy_pool(pool);

	pool = NULL;
	pool = ape_new_pool(SIZE_START, 0);
	EXPECT_TRUE(pool == NULL);

	for( n = 1; n < N_START; n += N_STEP ) {
		for(size = 1; size < SIZE_START; size += SIZE_STEP) {
			pool = NULL;
			pool = ape_new_pool(size, n);
			EXPECT_TRUE(pool != NULL);
			ape_destroy_pool(pool);
		}
	}

	for( n = N_START; n < N_LIMIT; n += N_STEP ) {
		for(size = SIZE_START; size < SIZE_LIMIT; size += SIZE_STEP) {
			pool = NULL;
			pool = ape_new_pool(size, n);
			EXPECT_TRUE(pool != NULL);
			ape_destroy_pool(pool);
		}
	}
#undef SIZE_START
#undef SIZE_LIMIT
#undef SIZE_STEP
#undef N_START
#undef N_LIMIT
#undef N_STEP
}

struct dummy_t {
	ape_pool_t pool;
	int a;
	char * b;
};
#if 0
static struct dummy_t * Dummy_New( int a, const char * b)
{
	struct dummy_t * dummy;
	
	dummy = (struct dummy_t*) malloc(sizeof(*dummy));
	dummy->a = a;
	dummy->b = strdup(b);

	return dummy;
}
static void Dummy_Delete(struct dummy_t *dummy)
{
	dummy->a = 0;
	free(dummy->b); dummy->b = NULL;
	free(dummy); dummy = NULL;
}


#endif

static void Dummy_Init( struct dummy_t * dummy)
{
	if (dummy) {
		dummy->a = 0;
		dummy->b = NULL;
	}
}

static void Dummy_Set( struct dummy_t * dummy, int a, const char * b)
{
	if (dummy) {
		dummy->a = a;
		if (dummy->b) {
			free(dummy->b); dummy->b = NULL;
		}
		dummy->b = strdup(b);
	}
}

static void Dummy_Clear( struct dummy_t * dummy)
{
	if (dummy) {
		dummy->a = 0;
		free(dummy->b);dummy->b = NULL;
	}
}

static void Dummy_Cleaner( ape_pool_t * pool, void * ctx)
{
	struct dummy_t *dummy;

	ctx = NULL;

	dummy = (struct dummy_t*) pool;
	Dummy_Clear(dummy);
}

TEST(Pool, allocate)
{
	ape_pool_t * pool;

	pool = ape_new_pool(sizeof(struct dummy_t), 2);
	Dummy_Init((struct dummy_t*) &pool->ptr.data);
	Dummy_Init((struct dummy_t*) &pool->next->ptr.data);
	Dummy_Set((struct dummy_t*) &pool->ptr.data, 1, "Nidium");
	Dummy_Set((struct dummy_t*) &pool->next->ptr.data, 2, "A new breed of browser");
	Dummy_Clear((struct dummy_t*) &pool->ptr.data);
	Dummy_Clear((struct dummy_t*) &pool->next->ptr.data);
	
	ape_destroy_pool(pool);
}

TEST(Pool, allocateAndClean)
{
	ape_pool_t * pool;

	pool = ape_new_pool(sizeof(struct dummy_t), 2);
	Dummy_Init((struct dummy_t*) &pool->ptr.data);
	Dummy_Init((struct dummy_t*) &pool->next->ptr.data);
	Dummy_Set((struct dummy_t*) &pool->ptr.data, 1, "Nidium");
	Dummy_Set((struct dummy_t*) &pool->next->ptr.data, 2, "A new breed of browser");

	ape_destroy_pool_ordered( pool, Dummy_Cleaner, NULL);
}

/*
//  head to queue
//  head to current
//  grow pool
//  push
//  rewind
//  destroy poollist ordered
//  foreach
//  foreach_reverse

TEST(Pool, SimplePoolList)
{
	ape_pool_list_t *list;
	int n;

	for (n = 1; n < 2; n++) {
		list = NULL;
		list = ape_new_pool_list(0, n);
		EXPECT_TRUE( list != NULL);

		ape_init_pool_list(list, 0, n);
		EXPECT_TRUE(list != NULL);
		EXPECT_TRUE(list->head != NULL);
		EXPECT_TRUE(list->head == list->current);
		EXPECT_TRUE(list->head->prev == NULL);
		EXPECT_TRUE(list->queue->next == NULL);

		ape_destroy_pool_list(list);
	}
}
*/
