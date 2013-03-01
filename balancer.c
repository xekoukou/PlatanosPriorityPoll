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


void update_drop_rate(int64_t *diff,unsigned char *drop_priority,int *ndiffs){
        if (*diff) {
            if (*drop_priority > 0) {
                (*drop_priority)--;
            }


        }
        else {
            (*ndiffs)++;
            if (*ndiffs == 1000) {
                *ndiffs = 0;
               if (*drop_priority < 101) {
                (*drop_priority)++;
            }}
        }

}

int
main (int argc, char *argv[])
{

    if (argc != 4) {
        exit (-1);
    }

    int numb_msgs = atoi (argv[2]);
    int numb_unimp_threads = atoi (argv[3]);

    zctx_t *ctx = zctx_new ();

    void *router_imp = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_rcvhwm (router_imp, 500000000);
    zsocket_bind (router_imp, "%s:9000", argv[1]);

    void *router_unimp = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_set_rcvhwm (router_unimp, 500000000);
    zsocket_bind (router_unimp, "%s:9001", argv[1]);

    void *pub = zsocket_new (ctx, ZMQ_PUB);
    zsocket_bind (pub, "%s:9002", argv[1]);


    int64_t time[3];

    int64_t idle=0;
    int64_t diff;


    zclock_sleep (1000);

//send the signal to start
    zmsg_t *amsg = zmsg_new ();
    zmsg_add (amsg, zframe_new ("all", 4));
    zmsg_send (&amsg, pub);

    zmq_pollitem_t pollitem[2] = { {router_imp, 0, ZMQ_POLLIN}
    , {router_unimp, 0, ZMQ_POLLIN}
    };
    unsigned long av_dropped_priority=0;
    unsigned long dropped=0;
    unsigned long av_processed_priority=0;
    unsigned long processed=0;
    int i;
    int imp_counter = 0;
    int once = 1;
    int ndiffs = 0;
    unsigned char drop_priority = 0;
    for (i = 0; i < 2 * numb_msgs; i++) {
        diff = zclock_time ();
        if (zmq_poll (pollitem, 2, -1) == -1) {
            exit (-1);
        }

        diff = zclock_time () - diff;
        idle = idle + diff;

update_drop_rate(&diff,&drop_priority,&ndiffs);


        if (pollitem[0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (router_imp);
            if (!msg) {
                exit (-1);
            }
            zmsg_destroy (&msg);
            imp_counter++;
            if (imp_counter == numb_msgs) {
                time[0] = zclock_time ();
            }

        }
        else {

            if (pollitem[1].revents & ZMQ_POLLIN) {
                if (once) {
                    time[1] = zclock_time ();
                    once = 0;
                }
                zmsg_t *msg = zmsg_recv (router_unimp);
                if (!msg) {
                    exit (-1);
                }
                zframe_t *frame = zmsg_unwrap (msg);
                zframe_destroy (&frame);
                unsigned char priority;
                memcpy (&priority, zframe_data (zmsg_first (msg)), 1);
                if (priority < drop_priority) {
                    av_dropped_priority+=priority;
                    dropped++;
                 // printf ("dropped:%u\n", priority);
                    zmsg_destroy (&msg);
                }
                else {
                    av_processed_priority+=priority;
                    processed++;
                 //   printf ("received:%u\n", priority);
                    zmsg_destroy (&msg);
                }


            }
        }


    }
    time[2] = zclock_time ();
    printf ("msgs received:%d\n", i);

    amsg = zmsg_new ();
    zmsg_add (amsg, zframe_new ("all", 4));
    zmsg_send (&amsg, pub);


    zmsg_t *msg = zmsg_recv (router_imp);
    zframe_t *frame = zmsg_unwrap (msg);
    zframe_destroy (&frame);
    int64_t time_imp[2];
    frame = zmsg_pop (msg);
    memcpy (time_imp, zframe_data (frame), zframe_size (frame));
    zframe_destroy (&frame);
    zmsg_destroy (&msg);


    int64_t time_unimp[numb_unimp_threads][2];
for(i=0; i<numb_unimp_threads; i++){
   
     msg = zmsg_recv (router_unimp);
    frame = zmsg_unwrap (msg);
    zframe_destroy (&frame);
    frame = zmsg_pop (msg);
    memcpy (time_unimp[i], zframe_data (frame), zframe_size (frame));
    zframe_destroy (&frame);
    zmsg_destroy (&msg);

}


//compute average latency

    printf ("\nTime when important msgs started to be sent: %lld\n",
            time_imp[0]);
    printf ("\nTime when important msgs were processed: %lld\n", time[0]);
    printf ("\nDifference: %lld\n", time[0] - time_imp[0]);

for(i=0; i<numb_unimp_threads; i++){
    printf ("\nTime when unimportant msgs started to be sent: %lld\n",
            time_unimp[i][0]);
    printf ("\nTime when unimportant msgs were processed: %lld\n", time[2]);
    printf ("\nDifference: %lld\n", time[2] - time_unimp[i][0]);


}

    printf ("\nTime when unimportant msgs started to be processed: %lld\n",
            time[1]);


    printf ("idle time:%llu\n",idle);

    printf ("dropped msgs:%llu\n",dropped);
    printf ("av_dropped_priority:%llu\n",av_dropped_priority/dropped);
    printf ("processed msgs:%llu\n",processed);
    printf ("av_processed_priority:%llu\n",av_processed_priority/processed);


}
