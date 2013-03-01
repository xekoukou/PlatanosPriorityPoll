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
#include<stdlib.h>

#define SIZE 50


int
main (int argc, char *argv[])
{

    if (argc != 5) {
        exit (-1);
    }

    int numb_msgs = atoi (argv[2]);
    int delay = atoi (argv[3]);
    int speed = atoi (argv[4]);  //per 10000msgs how many mill sleep

    zctx_t *ctx = zctx_new ();

    void *dealer = zsocket_new (ctx, ZMQ_DEALER);
    zsocket_set_linger (dealer, -1);
    zsocket_set_sndhwm (dealer, 500000000);
    zsocket_connect (dealer, "%s:9001", argv[1]);

    void *sub = zsocket_new (ctx, ZMQ_SUB);
    zsocket_connect (sub, "%s:9002", argv[1]);
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "all", 4);


    int64_t time[2];

    unsigned char *random = malloc (numb_msgs);
    int i;
    for (i = 0; i < numb_msgs; i++) {
        random[i] = rand () % 100;
    }

    zmq_pollitem_t pollitem[1] = { {sub, 0, ZMQ_POLLIN}
    };

    zmq_poll (pollitem, 1, -1);
    zmsg_t *signal = zmsg_recv (sub);
    zmsg_destroy (&signal);

    char blob[SIZE] = { 0 };
    zmsg_t *msg = zmsg_new ();
    zframe_t *frame = zframe_new (blob, SIZE);
    zmsg_add (msg, frame);

zclock_sleep(delay);

    time[0] = zclock_time ();
int j;
    for (i = 0; i < numb_msgs/10000; i++) {
    for (j = 0; j < 10000; j++) {
    
        zmsg_t *nmsg = zmsg_dup (msg);
        zmsg_push (nmsg, zframe_new (random + i, 1));
        zmsg_send (&nmsg, dealer);
    }
zclock_sleep(speed);
}
    zmsg_destroy (&msg);
    time[1] = zclock_time ();
    zmq_poll (pollitem, 1, -1);
    signal = zmsg_recv (sub);
    zmsg_destroy (&signal);
    msg = zmsg_new ();
    frame = zframe_new (time, sizeof (int64_t) * 2);
    zmsg_add (msg, frame);
    zmsg_send (&msg, dealer);
    zctx_destroy (&ctx);
}
