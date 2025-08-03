#include "wtime.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NS_PER_SECOND 1000000000

/*  Prototype  */
void wtime( double * );


/*****************************************************************/
/******         E  L  A  P  S  E  D  _  T  I  M  E          ******/
/*****************************************************************/
double elapsed_time( void )
{
    double t;

    wtime( &t );
    return( t );
}


double start[64], elapsed[64];

/*****************************************************************/
/******            T  I  M  E  R  _  C  L  E  A  R          ******/
/*****************************************************************/
void timer_clear( int n )
{
    elapsed[n] = 0.0;
}


/*****************************************************************/
/******            T  I  M  E  R  _  S  T  A  R  T          ******/
/*****************************************************************/
void timer_start( int n )
{
    start[n] = elapsed_time();
}


/*****************************************************************/
/******            T  I  M  E  R  _  S  T  O  P             ******/
/*****************************************************************/
void timer_stop( int n )
{
    double t, now;

    now = elapsed_time();
    t = now - start[n];
    elapsed[n] += t;

}


/*****************************************************************/
/******            T  I  M  E  R  _  R  E  A  D             ******/
/*****************************************************************/
double timer_read( int n )
{
    return( elapsed[n] );
}

/*****************************************************************/
/******                 M Y _ T  I  M  E  R                 ******/
/*****************************************************************/
void my_timespec(struct timespec start, struct timespec end)
{
    struct timespec td; 
    td.tv_nsec = end.tv_nsec - start.tv_nsec;
    td.tv_sec  = end.tv_sec - start.tv_sec;
    if (td.tv_sec > 0 && td.tv_nsec < 0)
    {
        td.tv_nsec += NS_PER_SECOND;
        td.tv_sec--;
    }
    else if (td.tv_sec < 0 && td.tv_nsec > 0)
    {
        td.tv_nsec -= NS_PER_SECOND;
        td.tv_sec++;
    }
    printf("time : %d.%.9ld\n", (int)td.tv_sec, td.tv_nsec);
}

void my_timespec_ms(struct timespec start, struct timespec end)
{
    struct timespec td; 
    td.tv_nsec = end.tv_nsec - start.tv_nsec;
    td.tv_sec  = end.tv_sec - start.tv_sec;
    if (td.tv_sec > 0 && td.tv_nsec < 0)
    {
        td.tv_nsec += NS_PER_SECOND;
        td.tv_sec--;
    }
    else if (td.tv_sec < 0 && td.tv_nsec > 0)
    {
        td.tv_nsec -= NS_PER_SECOND;
        td.tv_sec++;
    }
    // printf("time : %d.%.9ld\n", (int)td.tv_sec, td.tv_nsec);
    float time_ms = ((float)td.tv_sec + (float)td.tv_nsec/NS_PER_SECOND) * 1000; 
    printf("time(ms) : %f\n", time_ms);
}