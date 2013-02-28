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

    void *router_imp = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_rcvhwm (router_imp,500000000);
    zsocket_bind (router_imp, "tcp://192.168.1.3:9000");

    void *router_unimp = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_rcvhwm (router_unimp,500000000);
    zsocket_bind (router_unimp, "tcp://192.168.1.3:9001");

    void *pub = zsocket_new (ctx, ZMQ_PUB);
    zsocket_bind (pub, "tcp://192.168.1.3:9002");


    int64_t *time = malloc (sizeof (int64_t) * 1000000);

    zclock_sleep (3000);

//send the signal to start
    zmsg_t *amsg = zmsg_new ();
    zmsg_add (amsg, zframe_new ("all", 4));
    zmsg_send (&amsg, pub);

    zmq_pollitem_t pollitem[2] = { {router_imp, 0, ZMQ_POLLIN}
    , {router_unimp, 0, ZMQ_POLLIN}
    };

    int i = 0;
    int imp_counter = 0;
    while (1) {
        i++;
        if (zmq_poll (pollitem, 2, -1) == -1) {
            exit (-1);
        }

        if (pollitem[0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (router_imp);
            time[imp_counter] = zclock_time ();
            if (!msg) {
                exit (-1);
            }
            zmsg_destroy (&msg);
            imp_counter++;

        }
        else {

            if (pollitem[1].revents & ZMQ_POLLIN) {
                zmsg_t *msg = zmsg_recv (router_unimp);
                if (!msg) {
                    exit (-1);
                }
                zmsg_unwrap (msg);
                if (0 == memcmp ("finished", zframe_data (zmsg_first (msg)), 8)) {
                    break;
                }

                zmsg_destroy (&msg);
            }
        }


    }
    printf ("msgs received:%d\n", i);

    amsg = zmsg_new ();
    zmsg_add (amsg, zframe_new ("imp", 4));
    zmsg_send (&amsg, pub);


    printf ("Before receiving the time data:\n");
    zmsg_t *msg = zmsg_recv (router_imp);
    printf ("After receiving th time data\n");
    zmsg_unwrap (msg);
    int64_t *prev_time = malloc (sizeof (int64_t) * 1000000);
    zframe_t *frame = zmsg_pop (msg);
    memcpy (prev_time, zframe_data (frame), zframe_size (frame));

//compute average latency

    int64_t latency = 0;

    for (i = 0; i < 1000000; i++) {

        latency += time[i] - prev_time[i];

    }
    latency = latency / 1000000;


    printf ("\nAverage latency of important msgs: %" PRId64 "\n", latency);



}
