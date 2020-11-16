#ifndef CUSTOM_ETE_TASKS
#define CUSTOM_ETE_TASKS

#include <linux/sched.h>

/* End-to-end task struct */
struct ete_task {
    int task_num;                    // Unique Task number
    int subtasks_num;                 // Number of subtasks

    ulong period_ms;                  // Period
    ulong execution_time;                  // Time required to execute each task 

};

/* Sub-task Struct */
struct sub_task
{
    int num;                         // Unique Sub-task number
    int parent_num;                  // Unique parent task number
   
    struct hrtimer timer;            // Timer to reschedule  
    struct task_struct * task;       // Thread for the task
    ktime_t last_release_time;       // Time whenthe thread was last released
    int loop_iteration_count;        // Number of loop iterations

    ulong cumulative_execution_time; // Cumulative time upto to cur subtask  
    ulong execution_time;            // Total execution time of the ete task
    
    ulong relative_deadline;         // Relative deadline of the task

    int core;                        // Core that the task will bind to

    ulong utilization;               // Utilization of the task

    int priority;                   // Real-time priority of the subtask
    
};



#endif