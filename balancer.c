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
#include<string.h>
#include<stdio.h>

#define SIZE 50


int
main (int argc, char *argv[])
{

if(argc!=3){
exit(-1);
}

int numb_msgs=atoi(argv[2]);

    zctx_t *ctx = zctx_new ();

    void *router_imp = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_rcvhwm (router_imp, 500000000);
    zsocket_bind (router_imp, "%s:9000",argv[1]);

    void *router_unimp = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_rcvhwm (router_unimp, 500000000);
    zsocket_bind (router_unimp, "%s:9001",argv[1]);

    void *pub = zsocket_new (ctx, ZMQ_PUB);
    zsocket_bind (pub, "%s:9002",argv[1]);


    int64_t time[3];


zclock_sleep(1000);

//send the signal to start
    zmsg_t *amsg = zmsg_new ();
    zmsg_add (amsg, zframe_new ("all", 4));
    zmsg_send (&amsg, pub);

    zmq_pollitem_t pollitem[2] = { {router_imp, 0, ZMQ_POLLIN}
    , {router_unimp, 0, ZMQ_POLLIN}
    };

    int i = 0;
    int imp_counter = 0;
    int once=1;
    while (1) {
        i++;
        if (zmq_poll (pollitem, 2, -1) == -1) {
            exit (-1);
        }

        if (pollitem[0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (router_imp);
            if (!msg) {
                exit (-1);
            }
            zmsg_destroy (&msg);
            imp_counter++;
           if(imp_counter==numb_msgs){
time[0]=zclock_time();
}

        }
        else {

            if (pollitem[1].revents & ZMQ_POLLIN) {
                if(once){
time[1]=zclock_time();
once=0;
}

                zmsg_t *msg = zmsg_recv (router_unimp);
                if (!msg) {
                    exit (-1);
                }
                zframe_t *frame=zmsg_unwrap (msg);
                zframe_destroy(&frame);
                if (0 == memcmp ("finished", zframe_data (zmsg_first (msg)), 8)) {
                time[2] = zclock_time ();
                    break;
                }

                zmsg_destroy (&msg);
            }
        }


    }
    printf ("msgs received:%d\n", i);

    amsg = zmsg_new ();
    zmsg_add (amsg, zframe_new ("all", 4));
    zmsg_send (&amsg, pub);


    zmsg_t *msg = zmsg_recv (router_imp);
    zframe_t *frame=zmsg_unwrap (msg);
     zframe_destroy(&frame);
    int64_t time_imp[2];
    frame = zmsg_pop (msg);
    memcpy (time_imp, zframe_data (frame), zframe_size (frame));
     zframe_destroy(&frame);
                zmsg_destroy (&msg);

    msg = zmsg_recv (router_unimp);
    frame=zmsg_unwrap (msg);
     zframe_destroy(&frame);
    int64_t time_unimp[2];
    frame = zmsg_pop (msg);
    memcpy (time_unimp, zframe_data (frame), zframe_size (frame));
     zframe_destroy(&frame);
                zmsg_destroy (&msg);



//compute average latency

    printf ("\nTime when important msgs started to be sent: %lld\n", time_imp[0]);
    printf ("\nTime when important msgs were processed: %lld\n",time[0]);
    printf ("\nDifference: %lld\n",time[0]-time_imp[0]);

    printf ("\nTime when unimportant msgs started to be sent: %lld\n", time_unimp[0]);
    printf ("\nTime when inimportant msgs were processed: %lld\n",time[2]);
    printf ("\nDifference: %lld\n",time[2]-time_unimp[0]);


    printf ("\nTime when unimportant msgs started to be processed: %lld\n",time[1]);




}
