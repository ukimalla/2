#ifndef T1_H
#define T1_H

/*
    This header file was created by Uki Malla on 11/11/2020 22:59 PM

    This is the first headder file and will be created 100% manually.
*/

#include "tasks.h"

#define DEBUG 1
#define TOTAL_SUBTASKS  3

const int NUM_TASKS = 1;


/* Array of end-to-end tasks to be scheduled */
struct ete_task ete_tasks[] = {

    // Task 1
    {
        .task_num = 1,   
        .subtasks_num = 3,   
        .period_ms = 9,  
        .execution_time = 6,   

   }

};



/* Array of sub_tasks of the first ete task */
struct sub_task sub_tasks_1[] = {

   {
       .num = 1,
       .parent_num = 1,
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 7651,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running 
       .execution_time = 1,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   },

   {
       .num = 2, 
       .parent_num = 1,
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 15304,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running
       .execution_time = 2,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                // Calculated before running
       .priority = 0
   },

   {
       .num = 3, 
       .parent_num = 1,
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 22885,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running
       .execution_time = 3,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   }


};


/* array of pointers to ete-subtask array */
struct sub_task * sub_tasks[] = {
    sub_tasks_1
};



#endif //!T1_H