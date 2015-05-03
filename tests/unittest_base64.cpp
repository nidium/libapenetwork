#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_base64.h>

#define BASE64_BUFFER_SIZE 512
#define ORG "main(t){for(t=0;;t++)putchar(t*(((t>>12)|(t>>8))&(63&(t>>4))));}"
#define GRO "bWFpbih0KXtmb3IodD0wOzt0KyspcHV0Y2hhcih0KigoKHQ+PjEyKXwodD4+OCkpJig2MyYodD4+NCkpKSk7fQ=="
#define GRO_SAFE "bWFpbih0KXtmb3IodD0wOzt0KyspcHV0Y2hhcih0KigoKHQ-PjEyKXwodD4-OCkpJig2MyYodD4-NCkpKSk7fQ" 

TEST(Base64, EncodeDecode)
{
	char * org, * gro, * encoded;
	char dummy[BASE64_BUFFER_SIZE];
	char * pdummy;
	int size;

	pdummy = &dummy[0];
	memset( pdummy, '\0', BASE64_BUFFER_SIZE);
	org = strdup(ORG);
	gro = strdup(GRO);

	encoded = base64_encode((unsigned char *)org, strlen(org));
	EXPECT_TRUE(strcmp(encoded, gro) == 0);

	size = base64_decode((unsigned char *)pdummy, encoded, strlen(gro));
	EXPECT_EQ(size, strlen(org));
	EXPECT_TRUE(strcmp(pdummy, org) == 0);

	free(encoded);
	free(org);
	free(gro);
}

TEST(Base64, EncodeDecodeSafe)
{
	char * org, * gro, * encoded;
	char dummy[BASE64_BUFFER_SIZE];
	char * pdummy;
	int size;

	pdummy = &dummy[0];
	memset( pdummy, '\0', BASE64_BUFFER_SIZE);
	org = strdup(ORG);
	gro = strdup(GRO_SAFE);
	encoded = base64_encode_safe((unsigned char *)org, strlen(org));
	EXPECT_TRUE(strcmp(encoded, gro) == 0);

	size = base64_decode((unsigned char *)pdummy, encoded, strlen(gro));
	EXPECT_EQ(size, -1);
	EXPECT_TRUE(strncmp(pdummy, org, 35) == 0);

	free(encoded);
	free(org);
	free(gro);
}


TEST(Base64, EncodeDecodeBin)
{
	char * org, * gro, * encoded;
	char dummy[BASE64_BUFFER_SIZE];
	char * pdummy;
	int size;

	pdummy = &dummy[0];
	memset( pdummy, '\0', BASE64_BUFFER_SIZE);
	org = strdup(ORG);
	gro = strdup(GRO);

	encoded = (char*) malloc(BASE64_BUFFER_SIZE);
	base64_encode_b((unsigned char *)org, encoded, strlen(org));
	EXPECT_TRUE(strcmp(encoded, gro) == 0);

	size = base64_decode((unsigned char *)pdummy, encoded, strlen(gro));
	EXPECT_EQ(size, strlen(org));
	EXPECT_TRUE(strcmp(pdummy, org) == 0);

	free(encoded);
	free(org);
	free(gro);
}

TEST(Base64, EncodeDecodeBinSafe)
{
	char * org, * gro, * encoded;
	char dummy[BASE64_BUFFER_SIZE];
	char * pdummy;
	int size;

	pdummy = &dummy[0];
	memset( pdummy, '\0', BASE64_BUFFER_SIZE);
	org = strdup(ORG);
	gro = strdup(GRO);

	encoded = (char*) malloc(BASE64_BUFFER_SIZE);
	memset( encoded, '\n', BASE64_BUFFER_SIZE);
	base64_encode_b_safe((unsigned char *)org, encoded, strlen(org), '\0');
	EXPECT_TRUE(strcmp(encoded, gro) == 0);

	size = base64_decode((unsigned char *)pdummy, encoded, strlen(gro));
	EXPECT_EQ(size, strlen(org));
	EXPECT_TRUE(strcmp(pdummy, org) == 0);

	free(encoded);
	free(org);
	free(gro);
}

#undef ORG
#undef GRO
#undef GRO_SAFE
#undef BASE64_BUFFER_SIZE

