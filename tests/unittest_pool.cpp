#include <string.h>

#include <gtest/gtest.h>

#include <ape_pool.h>
/*
struct dummy_t {
	int a;
	char * b;
};

static struct dummy_t * Dummy_New( int a, const char * b)
{
	struct dummy_t * dummy;
	
	dummy = (struct dummy_t*) malloc(sizeof(*dummy));
	dummy->a = a;
	dummy->b = strdup(b);

	return dummy;
}

static void Dummy_Delete( struct dummy_t *dummy)
{
	dummy->a = 0;
	free(dummy->b); dummy->b = NULL;
}
*/
TEST(Pool, Simple)
{
	ape_pool_t * pool;

	pool = NULL;

	pool = ape_new_pool( 5, 20 );
	EXPECT_TRUE(pool != NULL);
	ape_destroy_pool( pool );
}
TEST(Pool, Looped)
{
#define SIZE_LIMIT 513
#define N_LIMIT 260
	ape_pool_t * pool;
	size_t size, n;

	for( n = 0; n < 33; n++ ); {
		for( size = 0; size < 33; size++ ); {
			pool = NULL;
			pool = ape_new_pool( size, n );
			EXPECT_TRUE(pool != NULL);
			ape_destroy_pool( pool );
		}
	}
#undef SIZE_LIMIT
#undef N_LIMIT
}
