#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <ape_base64.h>

#define BASE64_BUFFER_SIZE 512
#define ORG "main(t){for(t=0;;t++)putchar(t*(((t>>12)|(t>>8))&(63&(t>>4))));}"
#define GRO "bWFpbih0KXtmb3IodD0wOzt0KyspcHV0Y2hhcih0KigoKHQ+PjEyKXwodD4+OCkpJig2MyYodD4+NCkpKSk7fQ=="
#define GRO_SAFE "bWFpbih0KXtmb3IodD0wOzt0KyspcHV0Y2hhcih0KigoKHQ-PjEyKXwodD4-OCkpJig2MyYodD4-NCkpKSk7fQ" 

TEST(Base64_Encode,EncodeDecode) {
	char * org, * gro, * encoded, * decoded;
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

TEST(Base64_Encode,EncodeDecodeSafe) {
	char * org, * gro, * encoded, * decoded;
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


TEST(Base64_Encode,EncodeDecodeBin) {
	char * org, * gro, * encoded, * decoded;
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

TEST(Base64_Encode,EncodeDecodeBinSafe) {
	char * org, * gro, * encoded, * decoded;
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


#undef GRO
#undef ORG
#undef BASE64_BUFFER_SIZE	

