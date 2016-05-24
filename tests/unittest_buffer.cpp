/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <string.h>

#include "unittest.h"

#include <ape_buffer.h>

TEST(Buffer, Simple)
{
    buffer * buf = NULL;
    
    buf = buffer_new(8);
    EXPECT_TRUE(buf != NULL);
    EXPECT_EQ(buf->used, 0);
    EXPECT_EQ(buf->size, 8);

    buffer_delete(buf);
    EXPECT_EQ(buf->used, 0);
    buffer_destroy(buf);
}

TEST(Buffer, Init)
{
    buffer buf;

    buffer_init(&buf);

    EXPECT_TRUE(buf.data == NULL);
    EXPECT_EQ(buf.pos, 0);
    EXPECT_EQ(buf.used, 0);
    EXPECT_EQ(buf.size, 0);

}

TEST(Buffer, Append)
{
    char * text;
    buffer * buf;

    buf = buffer_new(8);
    buffer_prepare(buf, 2);
    EXPECT_EQ(buf->size, 8);

    buffer_prepare(buf, 10);
    EXPECT_EQ(buf->size, 18);

    buffer_append_string(buf, "Hello");
    EXPECT_EQ(buf->used, 5);
    
    buffer_append_char(buf, ' ');
    EXPECT_TRUE(strncmp((char*)buf->data, "Hello ", buf->used) == 0);

    text = strdup("WORLD");
    buffer_append_data_tolower(buf, (unsigned char*) text, strlen(text));
    EXPECT_TRUE(strncmp((char*)buf->data, "Hello world", buf->used) == 0);
    free(text);

    text = strdup("\nWORLD\n...");
    buffer_append_string_n(buf, text, 6);
    EXPECT_TRUE(strncmp((char*)buf->data, "Hello world\nWORLD", buf->used) == 0);
    free(text);

    buffer_destroy(buf);
}

TEST(Buffer, Case)
{
    buffer * buf;

    buf = buffer_new(1);
    buffer_append_char(buf, 'n');
    EXPECT_EQ(buf->used, 1);
    EXPECT_EQ(buf->size, 1);

    buffer_append_string(buf, "Idium-a-nEw-breed-of-browser");
    EXPECT_EQ(buf->used, 29);
    buffer_camelify(buf);
    EXPECT_TRUE(strncmp((char*)buf->data, "NIdium-A-NEw-Breed-Of-Browser ", buf->used) == 0);
    //@FIXME: EXPECT_TRUE(strncmp((char*)buf->data, "NidiumANewBreedOfBrowser ", buf->used) == 0); EXPECT_EQ(buf->used, 24);
    
    buffer_destroy(buf);
}

TEST(Buffer, SimpleCase)
{
    buffer * buf;

    buf = buffer_new(3);

    buffer_append_string(buf, "K1");
    EXPECT_EQ(buf->used, 2);
    buffer_camelify(buf);
    EXPECT_TRUE(strncmp((char*)buf->data, "K1", buf->used) == 0);
    
    buffer_destroy(buf);
}

TEST(Buffer, UTF8)
{
    buffer * buf, * utf8;
    unsigned char* copy;
    size_t len;

    buf = buffer_new(1);
    buffer_append_string(buf, "诶");
    EXPECT_EQ(buf->size, 5);
    EXPECT_EQ(buf->used, 3);
    
    len = buf->used;
    copy = (unsigned char*) malloc(len + 1);
    memcpy( copy, buf->data, len);
    copy[len] = '\0';
    buffer_destroy(buf);
    
    buf = buffer_new(len);
    buffer_append_data(buf, copy, len);
    utf8 = buffer_to_buffer_utf8(buf);
    buffer_destroy(buf);
    EXPECT_EQ(utf8->used, len * 2);

    buf = buffer_utf8_to_buffer(utf8);
    EXPECT_EQ(buf->used, len);
    EXPECT_TRUE(strcmp((char*)buf->data, (char*)copy) == 0);
    EXPECT_EQ(buf->size, 4);
    EXPECT_EQ(buf->used, 3);
    
    buffer_destroy(buf);
    buffer_destroy(utf8);
    free(copy);
    
}

