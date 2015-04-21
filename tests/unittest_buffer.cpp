#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <ape_buffer.h>


TEST(Buffer, Simple)
{
	buffer * buf = NULL;
	
	buf = buffer_new( 8 );
	EXPECT_TRUE(buf != NULL);
	EXPECT_EQ(buf->used, 0);
	EXPECT_EQ(buf->size, 8);

	buffer_delete( buf );
	EXPECT_EQ(buf->used, 0);
	buffer_destroy( buf );

}

