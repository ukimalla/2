#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>


#include <linux/sched.h>
#include <uapi/linux/sched/types.h>

#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/mm.h>

#include <asm/uaccess.h>

#include <linux/sort.h>

// Task Includes 
#include "tasks.h"
#include "t1.h"

#define NUM_CORES 4
#define MAX_UTILIZATION 100

static int tick;

static char * exec_mode = "calibrate";

module_param(exec_mode, charp, 0644); 

static ktime_t timer_interval;
struct hrtimer timer;

static struct task_struct * kthread = NULL;
static struct task_struct * kthread1 = NULL;
static struct task_struct * kthread2 = NULL;

static enum hrtimer_restart 
 timer_function( struct hrtimer * timer ){
     if(tick >= 4)
        tick = 0;

    wake_up_process(kthread2);

    if(tick == 0 || tick == 2)
        wake_up_process(kthread1);
    
    if(tick == 3) 
        wake_up_process(kthread);

    ++tick;

    hrtimer_forward_now(timer, timer_interval);
    return HRTIMER_RESTART;
}

static int
thread_fn(void * data)
{
    printk("Hello from thread %s.\n", current->comm);

    while (!kthread_should_stop()) {
        unsigned i = 0; // Inner loop counter
        ulong num_loops = 5000000;

        set_current_state(TASK_INTERRUPTIBLE);
        schedule();

        printk("Thread was woken up!\n");

        /*num_loops = (period_sec * num_loops) + 
                    (ulong) (period_nsec/800);*/
        
        for(i=0; i < num_loops; ++i){
            ktime_get();
        }
    }

    return 0;
}

/* Function to print all tasks and subtasks */
static void 
print_all_tasks_and_subtasks(void){
    int i, j;
    /* Looping through each task*/
    for(i = 0; i < NUM_TASKS; ++i){
        
        /* Pointer to array for each ete subtask */
        struct sub_task * ete_sub_tasks;  

        const int num_sub_tasks = ete_tasks[i].subtasks_num;

        
    
        /* Printing task i */
        printk(KERN_INFO "Task: %d \n subtasks_num: %d \nperiod_ms: %lu\n       \
                execution_time: %lu \n",
                ete_tasks[i].task_num,
                ete_tasks[i].subtasks_num,
                ete_tasks[i].period_ms,
                ete_tasks[i].execution_time);

        ete_sub_tasks = sub_tasks[i];


        /* Looping through sub-tasks */
        for(j = 0; j < num_sub_tasks; ++j){

            /* Printing subtask j of task i */
            printk(KERN_INFO "Task: %d Subtask: %d \n \
                            loop_iteration_count %d \n \
                            cumulative_execution_time %lu \n \
                            execution_time %lu \n \
                            relative_deadline %lu \n \
                            utilization %lu \n \
                            core %d \n ",
                            ete_sub_tasks[j].parent_num,
                            ete_sub_tasks[j].num,
                            ete_sub_tasks[j].loop_iteration_count,
                            ete_sub_tasks[j].cumulative_execution_time,
                            ete_sub_tasks[j].execution_time,
                            ete_sub_tasks[j].relative_deadline,
                            ete_sub_tasks[j].utilization,
                            ete_sub_tasks[j].core);
        }

    }

}



/* Function to compare two tasks based on utilization */
static int compare_by_utilization(const void *lhs, const void *rhs){
    struct sub_task * ltask = *(struct sub_task **) lhs;
    struct sub_task * rtask = *(struct sub_task **) rhs;

    if(ltask->utilization > rtask->utilization) return -1;
    if(ltask->utilization < rtask->utilization) return 1;

    return 0;
}

static int assign_cores_and_priority(void){
  
    /* Iterators */
    int i, j;

    int return_value = 0;
    const int MAX_PRIORITY = __NR_sched_get_priority_max;
    const int MIN_PRIORITY = __NR_sched_get_priority_min;

    #if DEBUG
        printk(KERN_INFO "Max Priority set to %d", MAX_PRIORITY);
    #endif

    int core_utilization[NUM_CORES] = {0, 0, 0, 0};
    int core_subtask_priority[NUM_CORES] = {MAX_PRIORITY, MAX_PRIORITY,
                                            MAX_PRIORITY, MAX_PRIORITY};


    /* ### Sorting sub-tasks according to util ### */

    /* An array of pointers to all subtasks to be sorted by utilization.*/
    struct sub_task * sorted_sub_tasks[TOTAL_SUBTASKS]; 

    /* Looping through each task*/
    for(i = 0; i < NUM_TASKS; ++i){
        // Number of subtasks in the current ete task
        int num_sub_tasks; 

        /* Pointer to array for each ete subtask */
        struct sub_task * current_sub_tasks_array;

        num_sub_tasks = ete_tasks[i].subtasks_num;
        current_sub_tasks_array = sub_tasks[i];

        /* Inserting each subtask to the array to be sorted*/
        for(j = 0; j < num_sub_tasks; ++j){
            sorted_sub_tasks[(i * num_sub_tasks) + j] =
                     &current_sub_tasks_array[j]; 
        } 
    }

    // Sorting the array of tasks by utilization
    sort(sorted_sub_tasks, 
        TOTAL_SUBTASKS, 
        sizeof(struct sub_task *),
        &compare_by_utilization,
        NULL);

    for(j = 0;j < TOTAL_SUBTASKS; ++j){
            /* Printing subtask j of task i */
            printk(KERN_INFO "Task: %d Subtask: %d \n \
                            loop_iteration_count %d \n \
                            cumulative_execution_time %lu \n \
                            execution_time %lu \n \
                            relative_deadline %lu \n \
                            utilization %lu \n \
                            core %d \n ",
                            sorted_sub_tasks[j]->parent_num,
                            sorted_sub_tasks[j]->num,
                            sorted_sub_tasks[j]->loop_iteration_count,
                            sorted_sub_tasks[j]->cumulative_execution_time,
                            sorted_sub_tasks[j]->execution_time,
                            sorted_sub_tasks[j]->relative_deadline,
                            sorted_sub_tasks[j]->utilization,
                            sorted_sub_tasks[j]->core);
    }

    /* ### Assigning cores and priority ### */
    // Iterating through the sorted sub-tasks
    for(i = 0; i < TOTAL_SUBTASKS; ++i){

        bool successful_assignment = false;
        int last_assigned_core = 0;

        // Iterating through cores
        for(j = 0; (j < NUM_CORES) && (!successful_assignment); ++j){
            int new_util = 
                core_utilization[j] + sorted_sub_tasks[i]->utilization;
            
            // Checking whether the task will fit in the core
            bool will_fit = (new_util <= MAX_UTILIZATION);

            if(will_fit){
                // Assigning the core once it fits
                sorted_sub_tasks[i]->core = j;   // Assigning core

                // Assigning sub-task priority
                if(core_subtask_priority[i] <= 0){                       
                    sorted_sub_tasks[i]->priority = 0;
                }else{                        
                    sorted_sub_tasks[i]->priority =
                                     core_subtask_priority[i]--;
                }

                core_utilization[j] = new_util;  // Tracking core util
                successful_assignment = true;
                last_assigned_core = j;
            }

        }

        // If proper assignment was not possible
        if(!successful_assignment){ 
            sorted_sub_tasks[i]->core = ++last_assigned_core;

            // Assigning sub-task priority
            if(core_subtask_priority[i] <= 0){                       
                sorted_sub_tasks[i]->priority = 0;
            }else{                        
                sorted_sub_tasks[i]->priority =
                                    core_subtask_priority[i]--;
            }

            return_value = -1;
        }

        #if DEBUG
            printk(KERN_INFO "Core %d has %d utilization\n", 
                    j, core_utilization[j]);
        #endif


    }


    return return_value;
}

/* Function to assign priorites to all subtasks */

static void 
set_priority(void){


}



/* 
int init_tasks(void)

This fucntion initializes cumulative_execution_time, relative_deadline,
and utilization of all subtasks */
static int 
init_tasks(void){

    int i, j;

    /* Looping through each task*/
    for(i = 0; i < NUM_TASKS; ++i){
        
        /* Pointer to array for each ete subtask */
        struct sub_task * ete_sub_tasks;  

        ulong cumulative_execution_time, relative_deadline, utilization;

        const int cur_num_sub_tasks = ete_tasks[i].subtasks_num;

        cumulative_execution_time = 0;
    
        printk(KERN_INFO "Task: %d \n subtasks_num: %d \nperiod_ms: %lu\n       \
                execution_time: %lu \n",
                ete_tasks[i].task_num,
                ete_tasks[i].subtasks_num,
                ete_tasks[i].period_ms,
                ete_tasks[i].execution_time);

        ete_sub_tasks = sub_tasks[i];

         

        /* Looping through sub-tasks */
        for(j = 0; j <  cur_num_sub_tasks; ++j){

            relative_deadline = 0;
            utilization = 0;

            /* Calculating cumulative execution time. */
            cumulative_execution_time +=
                 ete_sub_tasks[j].execution_time;

            /* Calculating relative deadline rounded down to the nearest whole 
            *  whole number. This is the safer as earlier deadlines have a 
            *  higher priority. 
            */
            relative_deadline = (ete_tasks[i].period_ms *
                 cumulative_execution_time) / ete_tasks[i].execution_time;
            
            /* Calculating utilization in a scale of 1 to 100 rounded up to
            * the nearest whole number, as this will make it safer. There will
            * be no utilization = 0 and a cores will not be over-filled.
            */
            utilization = 
            (ete_sub_tasks[j].execution_time * MAX_UTILIZATION)/ ete_tasks[i].period_ms;
            
            // Adding 1 to utilization to round up based on whether mod is > 0.
            utilization +=
                (!!( ete_tasks[i].period_ms % ete_sub_tasks[j].execution_time));


            /* Setting the varialbes in the sub-tasks */
            
            ete_sub_tasks[j].cumulative_execution_time = 
                cumulative_execution_time;
            
            ete_sub_tasks[j].relative_deadline = relative_deadline;


            ete_sub_tasks[j].utilization = utilization;
        }
    }

    return 0;
}



static int
ete_init(void)
{


    printk(KERN_INFO "Loaded kernel_memory module with argument %s\n", 
            exec_mode);

    init_tasks();
    printk(KERN_INFO "Tasks Value Initializiation Complete!\n");
    printk(KERN_INFO "Starting Core Assignment \n");
    if(assign_cores() != 0){
        printk(KERN_ALERT "This set of tasks is not schedulable.\n             \
                           Cores have been asigned anyway.\n");
    }

    printk(KERN_INFO "Assigning Cores Complete!\n");

    print_all_tasks_and_subtasks();

    




    return 0;



}

static void 
ete_exit(void)
{
    printk(KERN_INFO "Unloaded kernel_memory module\n");
}

module_init(ete_init);
module_exit(ete_exit);

MODULE_LICENSE ("GPL");
