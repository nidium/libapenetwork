/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_websocket.h>

TEST(Websocket, Simple)
{
    websocket_state wss;

    ape_ws_init(&wss, 0);
    EXPECT_EQ(wss.step, WS_STEP_START);
    ape_ws_close(&wss);
}

/*
//@TODO: ape_ws_process_frame
//@TODO: ape_ws_compute_key
//@TODO: ape_ws_write
//@TODO: void ape_ws_ping(websocket_state *state)
*/

