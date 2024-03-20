/**
 * \file
 *
 * Example project that demonstrates usage of queues in KERNEL.
 */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "task_consumer.h"
#include "task_producer.h"
#include "queue_example.h"
#include "kernel.h"

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- task stack size
#define  TASK_CONSUMER_STACK_SIZE      (KERNEL_MIN_STACK_SIZE + 96)

//-- priority of consumer task: the highest one
#define  TASK_CONSUMER_PRIORITY        0

//-- number of items in the consumer message queue
#define  CONS_QUE_BUF_SIZE    4

//-- max timeout for waiting for memory and free message
#define  WAIT_TIMEOUT         10





/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/**
 * Type of message that consumer receives.
 */
struct TaskConsumerMsg {
   enum E_TaskConsCmd   cmd;        //-- command to perform
                                    //   (well, now there's just one command)
   enum E_TaskConsPin   pin_num;    //-- number of pin for which command
                                    //   should be performed
};



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

//-- define array for task stack
KERNEL_STACK_ARR_DEF(task_consumer_stack, TASK_CONSUMER_STACK_SIZE);

//-- task descriptor: it's better to explicitly zero it
static struct KERNEL_Task task_consumer = {};

//-- define queue and buffer for it
struct KERNEL_DQueue     cons_que;
void                *cons_que_buf[ CONS_QUE_BUF_SIZE ];

//-- define fixed memory pool and buffer for it
struct KERNEL_FMem       cons_fmem;
KERNEL_FMEM_BUF_DEF(cons_fmem_buf, struct TaskConsumerMsg, CONS_QUE_BUF_SIZE);



/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/

/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    EXTERNAL FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

static void task_consumer_body(void *par)
{
   //-- init things specific to consumer task

   //-- create memory pool
   SYSRETVAL_CHECK(
         kernel_fmem_create(
            &cons_fmem,
            cons_fmem_buf,
            KERNEL_MAKE_ALIG_SIZE(sizeof(struct TaskConsumerMsg)),
            CONS_QUE_BUF_SIZE
            )
         );

   //-- create queue
   SYSRETVAL_CHECK(
         kernel_queue_create(&cons_que, (void *)cons_que_buf, CONS_QUE_BUF_SIZE)
         );

   //-- cry that consumer task has initialized
   SYSRETVAL_CHECK(
         kernel_eventgrp_modify(
            queue_example_eventgrp_get(),
            KERNEL_EVENTGRP_OP_SET,
            QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT
            )
         );


   struct TaskConsumerMsg *p_msg;

   //-- enter endless loop in which we will receive and handle messages
   for (;;)
   {
      //-- wait for message, potentially can wait forever
      //   (if there are no messages)
      enum KERNEL_RCode rc = SYSRETVAL_CHECK(
            kernel_queue_receive(&cons_que, (void *)&p_msg, KERNEL_WAIT_INFINITE)
            );

      if (rc == KERNEL_RC_OK){

         //-- message successfully received, let's check command
         switch (p_msg->cmd){

            case TASK_CONS_CMD__PIN_TOGGLE:
               //-- toggle specified bit
               queue_example_arch_pins_toggle(1 << p_msg->pin_num);
               break;

            default:
               //-- should never be here
               SOFTWARE_BREAK();
               break;
         }

         //-- free memory
         SYSRETVAL_CHECK(kernel_fmem_release(&cons_fmem, (void *)p_msg));
      }

   }
}






/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the header file
 */
void task_consumer_create(void)
{

   SYSRETVAL_CHECK(
         kernel_task_create(
            &task_consumer,
            task_consumer_body,
            TASK_CONSUMER_PRIORITY,
            task_consumer_stack,
            TASK_CONSUMER_STACK_SIZE,
            KERNEL_NULL,
            (KERNEL_TASK_CREATE_OPT_START)
            )
         );

}

/**
 * See comments in the header file
 */
int task_consumer_msg_send(enum E_TaskConsCmd cmd, enum E_TaskConsPin pin_num)
{
   int ret = 0;

   struct TaskConsumerMsg *p_msg;
   enum KERNEL_RCode kernel_rc;

   //-- get memory block from memory pool
   kernel_rc = SYSRETVAL_CHECK_TO(
         kernel_fmem_get(&cons_fmem, (void *)&p_msg, WAIT_TIMEOUT)
         );
   if (kernel_rc == KERNEL_RC_OK){

      //-- put correct data to the allocated memory
      p_msg->cmd     = cmd;
      p_msg->pin_num = pin_num;

      //-- send it to the consumer task
      kernel_rc = SYSRETVAL_CHECK_TO(
            kernel_queue_send(&cons_que, (void *)p_msg, WAIT_TIMEOUT)
            );

      if (kernel_rc == KERNEL_RC_OK){
         ret = 1/*success*/;
      } else {
         //-- there was some error while sending the message,
         //   so, we should free buffer that we've allocated
         SYSRETVAL_CHECK(kernel_fmem_release(&cons_fmem, (void *)p_msg));
      }
   } else {
      //-- there was some error while allocating memory,
      //   nothing to do here: ret is still 0, and it is
      //   going to be returned
   }

   return ret;
}





/*******************************************************************************
 *    end of file
 ******************************************************************************/



