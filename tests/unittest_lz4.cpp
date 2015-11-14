#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <lz4.h>

TEST(Lz4, Simple)
{
	int v;

	v = APE_LZ4_versionNumber();
	
	EXPECT_EQ(v, 171);
}
/*
@TODO: ALL
*/

