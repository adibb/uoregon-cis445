/* External definitions for single-server queueing system. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"  /* Header file for random-number generator. */
#include "pq.h"       /* Header file for linked list priority queue. */

#define Q_LIMIT  1000  /* Limit on queue length. */
#define QUEUES      2  /* Number of queues (the 'c' in M/M/c) */
#define BUSY        1  /* Mnemonics for server's being busy */
#define IDLE        0  /* and idle. */
#define REPS       10  /* Number of runs for the simulation. */

int    next_event_type, num_custs_delayed[QUEUES],
       num_time_max, num_in_transit_max, num_in_transit, num_events,
       num_in_queue[QUEUES], server_status[QUEUES];
float  area_num_in_queue[QUEUES], area_server_status[QUEUES],
       area_num_in_transit, mean_interarrival, mean_service[QUEUES],
       min_transit_time, max_transit_time, sim_time, 
       time_arrival[QUEUES][Q_LIMIT + 1],
       time_last_event[QUEUES], 
       total_of_delays[QUEUES];
e_list *events; // DEVNOTE: Wonder if I could make it a matrix of event lists?
FILE   *infile, *outfile;

void  initialize(void);
void  timing(void);
void  arrive(int);
void  transfer(void);
void  depart(int);
void  report(void);
void  update_time_avg_stats(int);
float expon(float);
float uniform(float, float);


int main()  /* Main function. */
{
    /* Open input and output files. */

    infile  = fopen("mm2.in",  "r");
    outfile = fopen("mm2.out", "w");

    /* Specify the number of events for the timing function. */

    num_events = QUEUES * 2; // DEVNOTE: Re-implement rest with the QUEUES later for modularity

    /* Allocate the priority queues. */
    events = new_list();

    /* Read input parameters. */

    fscanf(infile, "%f %f %f %f %f %d", &mean_interarrival, &(mean_service[0]),
           &(mean_service[1]), &min_transit_time, &max_transit_time, &num_time_max);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Tandem-server queueing system\n\n");
    fprintf(outfile, "Mean interarrival time%16.3f minutes\n\n",
            mean_interarrival);
    fprintf(outfile, "Mean service time (server 1)%10.3f minutes\n\n",
            mean_service[0]);
    fprintf(outfile, "Mean service time (server 2)%10.3f minutes\n\n",
            mean_service[1]);
    fprintf(outfile, "Minimum transit time%18.3f minutes\n\n",
            min_transit_time);
    fprintf(outfile, "Maximum transit time%18.3f minutes\n\n",
            max_transit_time);
    fprintf(outfile, "Time cutoff%27d minutes\n\n", num_time_max);

    /* Loop body starts */
    int i = 0;
    for (; i < REPS; i++){

        /* Initialize the simulation. */

        initialize();

        /* Run the simulation while more delays are still needed. */

        while (sim_time < num_time_max)
        {
            
            /* Determine the next event. */

            timing();

            /* Invoke the appropriate event function. */

            switch (next_event_type)
            {
                case 0:
                    arrive(0);
                    break;
                case 1:
                    depart(0);
                    break;
                case 2:
                    arrive(1);
                    break;
                case 3:
                    depart(1);
                    break;
            }
        }

        /* Invoke the report generator and end the simulation. */

        report();

    }
    /* End loop body */

    fclose(infile);
    fclose(outfile);
    free_list(events);

    return 0;
}


void initialize(void)  /* Initialization function. */
{
    /* Initialize the simulation clock. */

    sim_time = 0.0;

    /* Initialize the state variables. */

    server_status[0]     = IDLE;
    server_status[1]     = IDLE;
    num_in_queue[0]      = 0;
    num_in_queue[1]      = 0;
    time_last_event[0]   = 0.0;
    time_last_event[1]   = 0.0;

    /* Initialize the statistical counters. */

    total_of_delays[0]      = 0.0;
    total_of_delays[1]      = 0.0;
    area_num_in_queue[0]    = 0.0;
    area_num_in_queue[1]    = 0.0;
    area_server_status[0]   = 0.0;
    area_server_status[1]   = 0.0;
    area_num_in_transit     = 0.0;
    num_in_transit_max      = 0;
    num_in_transit          = 0;
    
    /* Initialize the events priority queue. */
   
    free_list(events);
    free_list(transit);
    events  = new_list();

    /* Initialize event list with one arrival in the first queue. */

    push(events, sim_time + expon(mean_interarrival), 0);
}


void timing(void)  /* Timing function. */
{
    /* Determine the event type of the next event to occur. */
    float min_time_next_event;

    if (!is_empty(events))
    {
        /* Pop the next event from the events list. */
    
        e_node *event = pop(events);
        next_event_type = get_event_type(event);
        min_time_next_event = get_event_time(event);
        free(event);
    
    } else {
        
        /* The event list is empty, so stop the simulation. */

        fprintf(outfile, "\nEvent list empty at time %f", sim_time);
        exit(1);
    }

    /* The event list is not empty, so advance the simulation clock. */

    sim_time = min_time_next_event;
}


void arrive(int queue_id)  /* Arrival event function. */
{
    float delay;
    int queue_event_base = 2 * queue_id;

    /* Schedule next arrival if in first queue. */

    if (queue_id == 0)
        push(events, sim_time + expon(mean_interarrival), queue_event_base);
    else {
        /* We've changing a variable that impacts an area variable, so 
           update those areas first. */
        
        update_time_avg_stats(queue_id);
        
        /* Decrement the transit count. */
        
        num_in_transit--;
    }


    /* Check to see whether server is busy. */

    if (server_status[queue_id] == BUSY)
    {
        /* Server is busy, so increment number of customers in queue. */

        ++num_in_queue[queue_id];

        /* Check to see whether an overflow condition exists. */

        if (num_in_queue[queue_id] > Q_LIMIT)
        {
            /* The queue has overflowed, so stop the simulation. */

            fprintf(outfile, "\nOverflow of the array time_arrival at");
            fprintf(outfile, " time %f", sim_time);
            exit(2);
        }

        /* There is still room in the queue, so store the time of arrival of the
           arriving customer at the (new) end of time_arrival. */

        time_arrival[queue_id][num_in_queue[queue_id]] = sim_time;
    }

    else
    {
        /* Server is idle, so arriving customer has a delay of zero.  (The
           following two statements are for program clarity and do not affect
           the results of the simulation.) */

        delay                       = 0.0;
        total_of_delays[queue_id]  += delay;

        /* Increment the number of customers delayed, and make server busy. */

        ++num_custs_delayed[queue_id];
        server_status[queue_id] = BUSY;

        /* Schedule a departure from the queue. */

        push(events, sim_time + expon(mean_service[queue_id]), queue_event_base + 1);
    }
}


void depart(int queue_id)  /* Departure event function. */
{
    int   i;
    float delay;
    
    int queue_event_base = queue_id * 2;

    /* Check to see whether the queue is empty. */

    if (num_in_queue[queue_id] == 0)
    {
        /* The queue is empty so make the server idle. */

        server_status[queue_id] = IDLE;
    }

    else
    {
        /* The queue is nonempty, so decrement the number of customers in
           queue. */

        --num_in_queue[queue_id];

        /* Compute the delay of the customer who is beginning service and update
           the total delay accumulator. */

        delay               = sim_time - time_arrival[queue_id][1];
        if (delay < 0)
            printf("Delay for customer is %f\n", delay);
        total_of_delays[queue_id] += delay;

        /* Increment the number of customers delayed, and schedule departure. */

        ++num_custs_delayed[queue_id];
        
        push(events, sim_time + expon(mean_service[queue_id]), queue_event_base + 1);

        /* Move each customer in queue (if any) up one place. */

        for (i = 1; i <= num_in_queue[queue_id]; ++i)
            time_arrival[queue_id][i] = time_arrival[queue_id][i + 1];
    }
    
    /* If not the last queue, then we need to schedule an arrival in the next one. */
    if (queue_id == 0){
        num_in_transit++;
        if(num_in_transit > num_in_transit_max)
            num_in_transit_max = num_in_transit;
        
        push(events, sim_time + uniform(min_transit_time, max_transit_time), 
            queue_event_base + 2);
    }

    /* Update time-average statistical accumulators for server. */

    update_time_avg_stats(queue_id);
}


void report(void)  /* Report generator function. */
{
    /* Compute and write estimates of desired measures of performance. */

    fprintf(outfile, "\n\nAverage delay in queue (1)%12.3f minutes\n\n",
            total_of_delays[0] / num_custs_delayed[0]);
    fprintf(outfile, "Average delay in queue (2)%12.3f minutes\n\n",
            total_of_delays[1] / num_custs_delayed[1]);
    fprintf(outfile, "Average number in queue (1)%11.3f\n\n",
            area_num_in_queue[0] / sim_time);
    fprintf(outfile, "Average number in queue (2)%11.3f\n\n",
            area_num_in_queue[1] / sim_time);
    fprintf(outfile, "Server 1 utilization%18.3f\n\n",
            area_server_status[0] / sim_time);
    fprintf(outfile, "Server 2 utilization%18.3f\n\n",
            area_server_status[1] / sim_time);
    fprintf(outfile, "Average number in transit%13.3f\n\n",
            area_num_in_transit / sim_time);
    fprintf(outfile, "Most in transit%23.d\n\n", 
            num_in_transit_max);
    fprintf(outfile, "Time simulation ended%17.3f minutes\n", sim_time);
}


void update_time_avg_stats(int s)  /* Update area accumulators for
                                         time-average statistics. */
{
    float time_since_last_event;

    /* Compute time since last event, and update last-event-time marker. */

    time_since_last_event = sim_time - time_last_event[s];
    time_last_event[s]       = sim_time;

    /* Update area under number-in-queue function. */

    area_num_in_queue[s]      += num_in_queue[s] * time_since_last_event;
    
    /* Update the area under number-in-transit function. */
   
    area_num_in_transit       += num_in_transit * time_since_last_event;

    /* Update area under server-busy indicator function. */

    area_server_status[s] += server_status[s] * time_since_last_event;
}


float expon(float mean)  /* Exponential variate generation function. */
{
    /* Return an exponential random variate with mean "mean". */

    return -mean * log(lcgrand(1));
}

float uniform(float min, float max)  /* Uniform variate generation function. */
{
    /* Return a uniformly distributed random variate between "min" and "max" */
   
    return min + ((max - min)*lcgrand(1));
}