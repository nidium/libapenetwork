#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <ape_websocket.h>

TEST(Websocket, Simple)
{
	websocket_state wss;


	ape_ws_init(&wss);
	EXPECT_EQ(wss.step, WS_STEP_START);
	ape_ws_close(&wss);
}
/*
@TODO
ape_ws_process_frame
ape_ws_compute_key
ape_ws_write
*/

