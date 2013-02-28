/*
    Copyright contributors as noted in the AUTHORS file.
                
    This file is part of PLATANOS.

    PLATANOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
            
    PLATANOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
        
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include<czmq.h>
#include<stdint.h>
#include<stddef.h>

#define SIZE 50


int
main (void)
{

    zctx_t *ctx = zctx_new ();

    void *dealer = zsocket_new (ctx, ZMQ_DEALER);
    zsocket_set_linger (dealer, -1);
    zsocket_connect (dealer, "tcp://192.168.1.3:9000");

    void *sub = zsocket_new (ctx, ZMQ_SUB);
    zsocket_connect (sub, "tcp://192.168.1.3:9002");
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "all", 4);
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "imp", 4);

    int64_t *time = malloc (sizeof (int64_t) * 1000000);

    zmq_pollitem_t pollitem[1] = { {sub, 0, ZMQ_POLLIN}
    };

    zmq_poll (pollitem, 1, -1);
    zmsg_recv (sub);

    zmsg_t *msg = zmsg_new ();
    zframe_t *frame = zframe_new (NULL, SIZE);
    zmsg_add (msg, frame);

    int i;
    for (i = 0; i < 1000000; i++) {
        zmsg_t *nmsg = zmsg_dup (msg);

        time[i] = zclock_time ();
        zmsg_send (&nmsg, dealer);


    }

    zmq_poll (pollitem, 1, -1);

    msg = zmsg_new ();
    frame = zframe_new (time, sizeof (time));
    zmsg_add (msg, frame);
    zmsg_send (&msg, dealer);

    zctx_destroy (&ctx);
}
