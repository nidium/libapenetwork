#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_blowfish.h>

TEST(Blowfish, Simple)
{
	struct APEBlowfish ctx;
	int key_len = 6;
	uint8_t key = 123;

	APE_blowfish_init(&ctx, &key, key_len);
}

/*
@TODO: APE_blowfish_init
@TODO: APE_blowfish_crypt_ecb
*/

