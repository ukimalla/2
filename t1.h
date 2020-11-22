#ifndef T1_H
#define T1_H

/*
    This header file was created by Uki Malla on 11/11/2020 22:59 PM

    This is the first headder file and will be created 100% manually.
*/

#include "tasks.h"

#define DEBUG 1
#define TOTAL_SUBTASKS  9

const int NUM_TASKS = 3;


/* Array of end-to-end tasks to be scheduled */
struct ete_task ete_tasks[] = {

    // Task 1
    {
        .task_num = 1,   
        .subtasks_num = 3,   
        .period_ms = 3000,  
        .execution_time = 6,   

   },

    // Task 2
    {
        .task_num = 2,   
        .subtasks_num = 3,   
        .period_ms = 3000,  
        .execution_time = 6,   
   },

    // Task 3
    {
        .task_num = 3, 
        .subtasks_num = 3,   
        .period_ms = 3000,  
        .execution_time = 12,   

   }



};



/* Array of sub_tasks of the first ete task */
struct sub_task sub_tasks_1[] = {

   {
       .num = 1,
       .parent_num = 1,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 9579,       // To be calibrated 
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
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 19312,       // To be calibrated 
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
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 29062,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running
       .execution_time = 3,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   }


};






/* Array of sub_tasks of the first ete task */
struct sub_task sub_tasks_2[] = {

   {
       .num = 1,
       .parent_num = 2,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 9750,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running 
       .execution_time = 1,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   },

   {
       .num = 2, 
       .parent_num = 2,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 19247,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running
       .execution_time = 2,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                // Calculated before running
       .priority = 0
   },

   {
       .num = 3, 
       .parent_num = 2,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 29083,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running
       .execution_time = 3,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   }


};

/* Array of sub_tasks of the first ete task */
struct sub_task sub_tasks_3[] = {

   {
       .num = 1,
       .parent_num = 3,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 19312,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running 
       .execution_time = 2,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   },

   {
       .num = 2, 
       .parent_num = 3,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 38549,
       .cumulative_execution_time = 0,
       .execution_time = 4,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                // Calculated before running
       .priority = 0
   },

   {
       .num = 3, 
       .parent_num = 3,
       .timer = {},
       .task = 0,

       .last_release_time = 0,
       .loop_iteration_count = 58007,       // To be calibrated 
       .cumulative_execution_time = 0,  // Calculated before running
       .execution_time = 6,
       .relative_deadline = 0,          // Calculated before running

       .core = 0,                       // Calculated before running
       .utilization = 0,                 // Calculated before running
       .priority = 0
   }


};


/* array of pointers to ete-subtask array */
struct sub_task * sub_tasks[] = {
    sub_tasks_1,
    sub_tasks_2,
    sub_tasks_3
};



#endif //!T1_H