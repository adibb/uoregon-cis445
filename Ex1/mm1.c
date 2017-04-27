/* External definitions for single-server queueing system. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"  /* Header file for random-number generator. */

#define Q_LIMIT 10000  /* Limit on queue length. */
#define BUSY        1  /* Mnemonics for server's being busy */
#define IDLE        0  /* and idle. */

int   next_event_type, num_custs_delayed[2],
      num_time_max, num_events,
      num_in_[2], server_status[2];
float area_num_in_[2], area_server_status[2],
      mean_interarrival, mean_service[2],
      sim_time, time_arrival[Q_LIMIT + 1], time_transfer[Q_LIMIT + 1],
      time_last_event[2], time_next_event[4],
      total_of_delays[2];
FILE  *infile, *outfile;

void  initialize(void);
void  timing(void);
void  arrive(void);
void  transfer(void);
void  depart(void);
void  report(void);
void  update_time_avg_stats(int);
float expon(float mean);


int main()  /* Main function. */
{
    /* Open input and output files. */

    infile  = fopen("mm1.in",  "r");
    outfile = fopen("mm1.out", "w");

    /* Specify the number of events for the timing function. */

    num_events = 3;

    /* Read input parameters. */

    fscanf(infile, "%f %f %f %d", &mean_interarrival, &(mean_service[0]),
           &(mean_service[1]), &num_time_max);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Tandem-server queueing system\n\n");
    fprintf(outfile, "Mean interarrival time%16.3f minutes\n\n",
            mean_interarrival);
    fprintf(outfile, "Mean service time (server 1)%10.3f minutes\n\n",
            mean_service[0]);
    fprintf(outfile, "Mean service time (server 2)%10.3f minutes\n\n",
            mean_service[1]);
    fprintf(outfile, "Time cutoff%27d minutes\n\n", num_time_max);

    /* Loop body starts */
    int i = 0;
    for (; i < 10; i++){

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
                case 1:
                    arrive();
                    break;
                case 2:
                    transfer();
                    break;
                case 3:
                    depart();
                    break;
            }
        }

        /* Invoke the report generator and end the simulation. */

        report();

    }
    /* End loop body */

    fclose(infile);
    fclose(outfile);

    return 0;
}


void initialize(void)  /* Initialization function. */
{
    /* Initialize the simulation clock. */

    sim_time = 0.0;

    /* Initialize the state variables. */

    server_status[0] = IDLE;
    server_status[1] = IDLE;
    num_in_[0]      = 0;
    num_in_[1]      = 0;
    time_last_event[0] = 0.0;
    time_last_event[1] = 0.0;

    /* Initialize the statistical counters. */

    total_of_delays[0]    = 0.0;
    total_of_delays[1]    = 0.0;
    area_num_in_[0]        = 0.0;
    area_num_in_[1]        = 0.0;
    area_server_status[0] = 0.0;
    area_server_status[1] = 0.0;

    /* Initialize event list.  Since no customers are present, the departure
       (service completion) event is eliminated from consideration. */

    time_next_event[1] = sim_time + expon(mean_interarrival);
    time_next_event[2] = 1.0e+30;
    time_next_event[3] = 1.0e+30;
}


void timing(void)  /* Timing function. */
{
    int   i;
    float min_time_next_event = 1.0e+29;

    next_event_type = 0;

    /* Determine the event type of the next event to occur. */

    for (i = 1; i <= num_events; ++i)
        if (time_next_event[i] < min_time_next_event)
        {
            min_time_next_event = time_next_event[i];
            next_event_type     = i;
        }

    /* Check to see whether the event list is empty. */

    if (next_event_type == 0)
    {
        /* The event list is empty, so stop the simulation. */

        fprintf(outfile, "\nEvent list empty at time %f", sim_time);
        exit(1);
    }

    /* The event list is not empty, so advance the simulation clock. */

    sim_time = min_time_next_event;
}


void arrive(void)  /* Arrival event function. */
{
    float delay;

    /* Schedule next arrival. */

    time_next_event[1] = sim_time + expon(mean_interarrival);

    /* Check to see whether server is busy. */

    if (server_status[0] == BUSY)
    {
        /* Server is busy, so increment number of customers in queue. */

        ++num_in_[0];

        /* Check to see whether an overflow condition exists. */

        if (num_in_[0] > Q_LIMIT)
        {
            /* The queue has overflowed, so stop the simulation. */

            fprintf(outfile, "\nOverflow of the array time_arrival at");
            fprintf(outfile, " time %f", sim_time);
            exit(2);
        }

        /* There is still room in the queue, so store the time of arrival of the
           arriving customer at the (new) end of time_arrival. */

        time_arrival[num_in_[0]] = sim_time;
    }

    else
    {
        /* Server is idle, so arriving customer has a delay of zero.  (The
           following two statements are for program clarity and do not affect
           the results of the simulation.) */

        delay               = 0.0;
        total_of_delays[0] += delay;

        /* Increment the number of customers delayed, and make server busy. */

        ++num_custs_delayed[0];
        server_status[0] = BUSY;

        /* Schedule a transfer (arrival completion). */

        time_next_event[2] = sim_time + expon(mean_service[0]);
    }
}


void transfer(void) /* Transfer event function. */
{
    /* STEP 1: Departure from server 1. */

    int   i;
    float delay;

    /* Check to see whether the queue is empty. */

    if (num_in_[0] == 0)
    {
        /* The queue is empty so make the server idle and eliminate the
           transfer (arrival completion) event from consideration. */

        server_status[0]      = IDLE;
        time_next_event[2] = 1.0e+30;
    }

    else
    {
        /* The queue is nonempty, so decrement the number of customers in
           queue. */

        --num_in_[0];

        /* Compute the delay of the customer who is beginning service and update
           the total delay accumulator. */

        delay               = sim_time - time_arrival[1];
        total_of_delays[0] += delay;

        /* Increment the number of customers delayed, and schedule transfer. */

        ++num_custs_delayed[0];
        time_next_event[2] = sim_time + expon(mean_service[0]);

        /* Move each customer in queue (if any) up one place. */

        for (i = 1; i <= num_in_[0]; ++i)
            time_arrival[i] = time_arrival[i + 1];
    }

    /* Update time-average statistical accumulators for first server. */

    update_time_avg_stats(0);



    /* STEP 2: Arrival at server 2. */

    delay = 0.0;

    /* Check to see whether server is busy. */

    if (server_status[1] == BUSY)
    {
        /* Server is busy, so increment number of customers in queue. */

        ++num_in_[1];

        /* Check to see whether an overflow condition exists. */

        if (num_in_[1] > Q_LIMIT)
        {
            /* The queue has overflowed, so stop the simulation. */

            fprintf(outfile, "\nOverflow of the array time_transfer at");
            fprintf(outfile, " time %f", sim_time);
            exit(2);
        }

        /* There is still room in the queue, so store the time of arrival of the
           arriving customer at the (new) end of time_arrival. */

        time_transfer[num_in_[1]] = sim_time;
    }

    else
    {
        /* Server is idle, so arriving customer has a delay of zero.  (The
           following two statements are for program clarity and do not affect
           the results of the simulation.) */

        delay               = 0.0;
        total_of_delays[1] += delay;

        /* Increment the number of customers delayed, and make server busy. */

        ++num_custs_delayed[1];
        server_status[1] = BUSY;

        /* Schedule a departure (transfer completion). */

        time_next_event[3] = sim_time + expon(mean_service[1]);
    }
}


void depart(void)  /* Departure event function. */
{
    int   i;
    float delay;

    /* Check to see whether the queue is empty. */

    if (num_in_[1] == 0)
    {
        /* The queue is empty so make the server idle and eliminate the
           departure (service completion) event from consideration. */

        server_status[1]   = IDLE;
        time_next_event[3] = 1.0e+30;
    }

    else
    {
        /* The queue is nonempty, so decrement the number of customers in
           queue. */

        --num_in_[1];

        /* Compute the delay of the customer who is beginning service and update
           the total delay accumulator. */

        delay               = sim_time - time_transfer[1];
        total_of_delays[1] += delay;

        /* Increment the number of customers delayed, and schedule departure. */

        ++num_custs_delayed[1];
        time_next_event[3] = sim_time + expon(mean_service[1]);

        /* Move each customer in queue (if any) up one place. */

        for (i = 1; i <= num_in_[0]; ++i)
            time_transfer[i] = time_transfer[i + 1];
    }

    /* Update time-average statistical accumulators for server 2. */

    update_time_avg_stats(1);
}


void report(void)  /* Report generator function. */
{
    /* Compute and write estimates of desired measures of performance. */

    fprintf(outfile, "\n\nAverage delay in queue (1)%12.3f minutes\n\n",
            total_of_delays[0] / num_custs_delayed[0]);
    fprintf(outfile, "Average delay in queue (2)%12.3f minutes\n\n",
            total_of_delays[1] / num_custs_delayed[1]);
    fprintf(outfile, "Average number in queue (1)%11.3f\n\n",
            area_num_in_[0] / sim_time);
    fprintf(outfile, "Average number in queue (2)%11.3f\n\n",
            area_num_in_[1] / sim_time);
    fprintf(outfile, "Server 1 utilization%18.3f\n\n",
            area_server_status[0] / sim_time);
    fprintf(outfile, "Server 2 utilization%18.3f\n\n",
            area_server_status[1] / sim_time);
    fprintf(outfile, "Time simulation ended%17.3f minutes", sim_time);
}


void update_time_avg_stats(int s)  /* Update area accumulators for
                                         time-average statistics. */
{
    float time_since_last_event;

    /* Compute time since last event, and update last-event-time marker. */

    time_since_last_event = sim_time - time_last_event[s];
    time_last_event[s]       = sim_time;

    /* Update area under number-in-queue function. */

    area_num_in_[s]      += num_in_[s] * time_since_last_event;

    /* Update area under server-busy indicator function. */

    area_server_status[s] += server_status[s] * time_since_last_event;
}


float expon(float mean)  /* Exponential variate generation function. */
{
    /* Return an exponential random variate with mean "mean". */

    return -mean * log(lcgrand(1));
}

