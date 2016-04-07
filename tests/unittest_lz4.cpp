/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

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

