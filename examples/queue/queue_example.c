/**
 * \file
 *
 * Example project that demonstrates usage of queues in KERNEL.
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "queue_example.h"

#include "task_consumer.h"
#include "task_producer.h"

#include "kernel.h"



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

static struct KERNEL_EventGrp que_example_events;




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * Each example should define this funtion: it creates first application task
 */
void init_task_create(void)
{
   task_producer_create();
}


/**
 * See comments in the header file
 */
void queue_example_init(void)
{
   //-- create application events
   //   (see enum E_QueExampleFlag in the header)
   SYSRETVAL_CHECK(kernel_eventgrp_create(&que_example_events, (0)));

   //-- init architecture-dependent stuff
   queue_example_arch_init();
}

/**
 * See comments in the header file
 */
struct KERNEL_EventGrp *queue_example_eventgrp_get(void)
{
   return &que_example_events;
}

/*******************************************************************************
 *    end of file
 ******************************************************************************/



