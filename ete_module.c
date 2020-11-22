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

#include "manifest.h"

// Task Includes 
#include "tasks.h"
#include "t2.h"


// TODO : Add compariosion return value

#define NUM_CORES 4
#define HIGHEST_CORE_NUM 3

#define MAX_UTILIZATION 100
#define MAX_PRIORITY 99 
#define MIN_PRIORITY 0  

#define NUM_CALIBRATION_TSTAMPS 2
#define BEFORE_EXEC_INDEX 0
#define AFTER_EXEC_INDEX 1
#define INIT_NUM_ITERATIONS 3000 
#define CALIBRATION_THREAD_START_DELAY_NS 100000000
#define MAX_CALIBRATION_ITERATIONS 100 
#define MS_TO_US_CONVERSION_FACTOR 1000 
#define MS_TO_NS_CONVERSION_FACTOR 1000000
#define ERROR_TOLERANCE 10 // 0.01 miliseconds or 10 microseconds
#define SEARCH_FACTOR 2 // Binary search

#define FIRST_TASK 1
#define SUCCESSOR_TASK_OFFSET 1 
#define PARENT_NUM_ARRAY_OFFSET 1

#define CALIBRATE "calibrate"
#define RUN "run"

static char * exec_mode = CALIBRATE; 

static struct hrtimer thread_wakeup_timer; 

 module_param(exec_mode, charp, 0644); 


// Array of sub_task pointers bound to each core
static struct sub_task ** core_sub_tasks[NUM_CORES]; 

// Holds the number of sub-tasks assigned to each core.
static int core_num_subtasks[NUM_CORES] = {0, 0, 0, 0};

// Array to hold task_struct * for each core for calibration mode
static struct task_struct * core_threads[NUM_CORES] = {NULL, NULL, NULL, NULL}; 


// An array of pointers to all subtasks to be sorted by utilization.
struct sub_task * sorted_sub_tasks[TOTAL_SUBTASKS]; 


// Prints all the member variables of the task
void print_task(struct ete_task * task){
         printk(KERN_INFO "Task: %d \n subtasks_num: %d \nperiod_ms: %lu\n       \
                execution_time: %lu \n",
                task->task_num,
                task->subtasks_num,
                task->period_ms,
                task->execution_time);
}

// Prints all the member variables of the subtask 
void print_sub_tasks(struct sub_task * st){

            // Printing subtask j of task i 
            printk(KERN_INFO "Task: %d Subtask: %d \n \
                            loop_iteration_count %d \n \
                            cumulative_execution_time %lu \n \
                            execution_time %lu \n \
                            relative_deadline %lu \n \
                            utilization %lu \n \
                            core %d \n \
                            task %p \n \
                            timer ptr %p \n",
                            st->parent_num,
                            st->num,
                            st->loop_iteration_count,
                            st->cumulative_execution_time,
                            st->execution_time,
                            st->relative_deadline,
                            st->utilization,
                            st->core,
                            st->task,
                            &(st->timer));
}

/* Function to print all tasks and subtasks */
void print_all_tasks_and_subtasks(void){
    int i, j;
    // Looping through each task
    for(i = 0; i < NUM_TASKS; ++i){
        
        // Pointer to array for each ete subtask 
        struct sub_task * ete_sub_tasks;  

        const int num_sub_tasks = ete_tasks[i].subtasks_num;

        
    
        // Printing task i 
        print_task(&ete_tasks[i]);
        ete_sub_tasks = sub_tasks[i];


        // Looping through sub-tasks 
        for(j = 0; j < num_sub_tasks; ++j){               
            print_sub_tasks(&ete_sub_tasks[j]);
        }

    }
} 




static void deallocate_memory(void){
    unsigned int i;
    // Deallocating dynamically allocated memory
    for(i = 0; i < NUM_CORES; ++i){
       if(core_sub_tasks[i]){
           kfree(core_sub_tasks[i]); 
       }
       core_sub_tasks[i] = 0; // Removing free-ed pointer
    }
}

/* subtask_lookup
* Function to obtain the sub_task struct using the subtask's timer.
*/
static struct sub_task * subtask_lookup(struct hrtimer * timer){

    int hrtimer_offset = ((void *) &(sub_tasks[0]->timer) -
                                        (void *) (sub_tasks[0]));

    

 // Iterator
    #if DEBUG 
        printk(KERN_INFO 
               "Performing subtask_lookup for %p", timer);
    #endif


    return (struct sub_task * ) ((void *) timer - hrtimer_offset);
}

/* timer_expiration_function
*  Function to wakeup the process when the timer expires
*/
static enum hrtimer_restart 
timer_expiration_function(struct hrtimer * timer){
    struct sub_task * st;
    
    st = subtask_lookup(timer);
    wake_up_process(st->task);

    #if DEBUG
        printk(KERN_ALERT"Should've woken up task \n");
    #endif
 

    return HRTIMER_NORESTART;
}


/* subtask_function
   Calls ktime_get as specified by loop_iteration_count in st to simulate 
   real workload.
 */ 
void subtask_function( struct sub_task * st ){
    int i;

    // Iterator
    #if DEBUG 
        printk(KERN_INFO 
               "Running task %d and subtask %d\n", st->parent_num, st->num);
    #endif


    for(i = 0; i < st->loop_iteration_count; ++i){
        ktime_get();
    }
}



/* run_thread_function
*  
*/
static int run_thread_function(void * data){
    struct sub_task * st = (struct sub_task *) data;

    if(st){
        struct ete_task * pt;
        struct sub_task * succ_st;
        ulong period_ns;
        ktime_t period;

        hrtimer_init(&(st->timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        st->timer.function = &timer_expiration_function;

        
        while(!kthread_should_stop()){
            set_current_state(TASK_INTERRUPTIBLE);
            schedule();

        #if DEBUG
            printk(KERN_ALERT"Hello after wake up for subtask");
            print_sub_tasks(st);
        #endif
 

            pt = &(ete_tasks[st->parent_num - PARENT_NUM_ARRAY_OFFSET]);
            period_ns = pt->period_ms *
                            MS_TO_NS_CONVERSION_FACTOR;

            period = ktime_set(0, period_ns);

            #if DEBUG
                printk(KERN_ALERT"The period_ns is %lld\n", ktime_to_ns(period_ns));
            #endif 


            st->last_release_time = ktime_get();
            subtask_function(st);
            if(st->num == FIRST_TASK){ 
                if(!hrtimer_active(&(st->timer))){
                    ktime_t exp_time = period - (ktime_get() - st->last_release_time);
                   

                    #if DEBUG
                        printk(KERN_ALERT"The time is inactive \n");
                    #endif
                   printk(KERN_ALERT"The timer will fire in %lld\n",
                            ktime_to_ns(exp_time));

                    hrtimer_start(&(st->timer), 
                              exp_time, 
                              HRTIMER_MODE_REL);  


                }else{
                    #if DEBUG
                        printk(KERN_ALERT"The timer is in active\n");
                        print_sub_tasks(st);
                    #endif

                    /*hrtimer_forward(&(st->timer), st->last_release_time, period);*/
                }

                
            }else{

                     #if DEBUG
                        printk(KERN_ALERT"Not the first task");
                    #endif
            }
                        
            if(st->num != pt->subtasks_num){ // Checking for not last task 
                ktime_t next_release_time;
                
                /*succ_st = &(sub_tasks[st->parent_num - PARENT_NUM_ARRAY_OFFSET]
                                   [st->num]);*/



                 /*next_release_time = ktime_add(period,
                                                succ_st->last_release_time);*/

                if(ktime_get() < next_release_time){
                   /*hrtimer_start(&(succ_st->timer),
                                 next_release_time,
                                 HRTIMER_MODE_ABS);*/

                }else{
                    //wake_up_process(succ_st->task);
                }
            }

            
        }

        
    }
    return 0;

}


/*  calibrate_thread_function 
*   Function to calibrate the loop number of iterations. This allows for a 
*   closer approximation of a real workload that takes the execution time ;
*   specified in the subtask struct.
*/
static int calibrate_thread_function(void * data){
    int i, size;
    struct sub_task ** core_subtask_array = (struct sub_task **) data;



    #if DEBUG
        printk(KERN_INFO "Setting Calibration Thread Task Interruptable\n");
    #endif

    set_current_state(TASK_INTERRUPTIBLE);
    schedule();

    #if DEBUG
        printk(KERN_INFO "Waking up from calibration thread\n");
    #endif
 

    

    if(core_subtask_array){ // Ensuring that the pointer is not NULL


        const int current_core = core_subtask_array[0]->core; 

        #if DEBUG
            printk(KERN_INFO "Getting the size of the array\n");
        #endif
        // Getting the number of subtasks 
        size = core_num_subtasks[current_core];


        #if DEBUG
           printk(KERN_INFO "There are %d subtasks in core %d",
                             size,
                             current_core);
        #endif
 


        for(i = 0; i < size; ++i){
            struct sched_param sp;

            // Binary search tracker variables
            int j;
            int last_min_iterations = 0;
            int last_max_iterations = INIT_NUM_ITERATIONS;
            
            int64_t execution_time_in_us; 
            bool best_estimate_found;


            sp.sched_priority = core_subtask_array[i]->priority;
            sched_setscheduler(current, SCHED_FIFO, &sp);

            // TODO : fix edge case where exec_time = 0
            if(core_subtask_array[i]->loop_iteration_count == 0){
               core_subtask_array[i]->loop_iteration_count = 
                            INIT_NUM_ITERATIONS;
            }else{
                last_max_iterations =
                                 core_subtask_array[i]->loop_iteration_count;
            }

            execution_time_in_us =
                                (core_subtask_array[i]->execution_time * 
                                MS_TO_US_CONVERSION_FACTOR);

                                                

            // Iterations to search for the best approximation 
            for(best_estimate_found = false, j = 0;
                j < MAX_CALIBRATION_ITERATIONS && !(best_estimate_found);
                ++j)
            {
                int64_t time_taken_us, error;
                int current_num_itr;

                ktime_t calibration_timestamps[NUM_CALIBRATION_TSTAMPS];

                printk("Going through iteration %d on core %d", 
                       j, current_core);

                current_num_itr = core_subtask_array[i]->loop_iteration_count;

                // Simulating a real task and getting timestamps
                calibration_timestamps[BEFORE_EXEC_INDEX] = ktime_get();
                subtask_function(core_subtask_array[i]); 
                calibration_timestamps[AFTER_EXEC_INDEX] = ktime_get();

                // Calculating error 
                time_taken_us = 
                       ktime_us_delta(calibration_timestamps[AFTER_EXEC_INDEX],
                               calibration_timestamps[BEFORE_EXEC_INDEX]);


                
                error =  time_taken_us-
                        execution_time_in_us;


                // Checking if and what change is necessary

                if(abs(error) < ERROR_TOLERANCE){ // Best estimate found
                    best_estimate_found = true;
                    printk(KERN_INFO "Best esitimate found: \n");
                    print_sub_tasks(core_subtask_array[i]);
                    #if DEBUG
                        printk(KERN_INFO "time_take_us %lld \n  \
                                        execution_time_in_us %lld \n  \
                                        error %lld \n \
                                        current_num_itr %d \n \
                                        next_itr %d \n",
                                        time_taken_us,
                                        execution_time_in_us,
                                        error,
                                        current_num_itr,
                                        core_subtask_array[i]->loop_iteration_count);
                    #endif
                }
               
                
                else if(error < 0) // Took shorter than necessary 
                {
                    // Ceeling has not been reached
                    if(last_max_iterations == current_num_itr){
                        last_min_iterations = last_max_iterations;
                        last_max_iterations *= SEARCH_FACTOR;
                        core_subtask_array[i]->loop_iteration_count = 
                                    last_max_iterations;
                    }else{
                        last_min_iterations = current_num_itr;
                        core_subtask_array[i]->loop_iteration_count = 
                            (last_max_iterations + last_min_iterations) /
                            SEARCH_FACTOR;
                    }
                }
               
               
               else{ // Took longer than necessary 
                    last_max_iterations = current_num_itr;
                    core_subtask_array[i]->loop_iteration_count = 
                        (last_max_iterations + last_min_iterations) /
                        SEARCH_FACTOR; 
                }
                #if DEBUG
                if(j == MAX_CALIBRATION_ITERATIONS - 1){
                    printk(KERN_INFO "time_take_us %lld \n  \
                            execution_time_in_us %lld \n  \
                            error %lld \n \
                            current_num_itr %d \n \
                            next_itr %d \n",
                            time_taken_us,
                            execution_time_in_us,
                            error,
                            current_num_itr,
                            core_subtask_array[i]->loop_iteration_count);
                }
                #endif



            }

            // Checking if converged
            if(!best_estimate_found){
                printk(KERN_ALERT "Could not converge for: \n");
                print_sub_tasks(core_subtask_array[i]);
            }else{
                printk(KERN_ALERT "Best Iteration was Found!");
            }

        }

    }

    return 0;

}




// Function to wake up all calibration core threads
static enum hrtimer_restart
wake_up_all_core_threads( struct hrtimer * timer ){
    unsigned int core_num;

    for(core_num = 0; core_num < NUM_CORES; ++core_num){
        if(core_num_subtasks[core_num]){
            printk(KERN_INFO"Should call wakeup process for thread %d\n",
                   core_num);
            wake_up_process(core_threads[core_num]);
        }
    }


    return HRTIMER_NORESTART;
}


/* Function to compare two tasks based on utilization */
static int compare_by_utilization(const void *lhs, const void *rhs){
    struct sub_task * ltask = *(struct sub_task **) lhs;
    struct sub_task * rtask = *(struct sub_task **) rhs;

    #if DEBUG
        printk(KERN_INFO "lhs %p rhs %p", lhs, rhs);
    #endif 


    if(ltask->utilization > rtask->utilization) return -1;
    if(ltask->utilization < rtask->utilization) return 1;

    return 0;
}

/* Function to compare two tasks based on relative deadline*/
static int compare_by_relative_deadline(const void *lhs, const void *rhs){
    struct sub_task * ltask = *(struct sub_task **) lhs;
    struct sub_task * rtask = *(struct sub_task **) rhs;

    #if DEBUG
        printk(KERN_INFO "lhs %p rhs %p", lhs, rhs);
    #endif 

    if(ltask->relative_deadline > rtask->relative_deadline) return -1;
    if(ltask->relative_deadline < rtask->relative_deadline) return 1;

    return 0;
}


 
static int assign_cores(void){
  
    // Iterators 
    int i;
    unsigned j;

    int return_value = 0;
    int core_utilization[NUM_CORES] = {0, 0, 0, 0};


    // ### Sorting sub-tasks according to util ### 

    // Looping through each task
    for(i = 0; i < NUM_TASKS; ++i){
        // Number of subtasks in the current ete task
        int num_sub_tasks; 

        // Pointer to array for each ete subtask 
        struct sub_task * current_sub_tasks_array;

        num_sub_tasks = ete_tasks[i].subtasks_num;
        current_sub_tasks_array = sub_tasks[i];

        // Inserting each subtask to the array to be sorted
        for(j = 0; j < num_sub_tasks; ++j){
            sorted_sub_tasks[(i * num_sub_tasks) + j] =
                     &current_sub_tasks_array[j]; 
        } 
    }

    #if DEBUG
       printk(KERN_INFO "Sort input address %p\n",sorted_sub_tasks);
    #endif

    // Sorting the array of tasks by utilization
    sort(sorted_sub_tasks, 
        TOTAL_SUBTASKS, 
        sizeof(struct sub_task *),
        &compare_by_utilization,
        NULL);


    // ### Assigning cores ### 
    // Iterating through the sorted sub-tasks
    for(i = 0; i < TOTAL_SUBTASKS; ++i){

        bool successful_assignment = false;
        int last_assigned_core = 0;

        // Iterating through cores
        for(j = HIGHEST_CORE_NUM; (j >= 0) && (!successful_assignment); --j){

            int new_util = 
                core_utilization[j] + sorted_sub_tasks[i]->utilization;
            
            // Checking whether the task will fit in the core
            bool will_fit = (new_util <= MAX_UTILIZATION);

            #if DEBUG
               printk(KERN_INFO "Core %d has %d utilization\n", 
                    j,
                    core_utilization[j]);
            #endif
            
            // Assigning the core once it fits
            if(will_fit){
                sorted_sub_tasks[i]->core = j;   // Assigning core

                core_utilization[j] = new_util;  // Tracking core util
                ++core_num_subtasks[j];          // Tracking subtasks per-core
                last_assigned_core = j;         

                successful_assignment = true;   
            }

            #if DEBUG
               printk(KERN_INFO "Core %d has %d utilization\n", 
                    j,
                    core_utilization[j]);
            #endif

        }

        // If proper assignment was not possible
        if(!successful_assignment){ 
            if(++last_assigned_core >= NUM_CORES)
                last_assigned_core = 0;

            ++core_num_subtasks[last_assigned_core];    // Assigning last core
            sorted_sub_tasks[i]->core = last_assigned_core;
            return_value = -FAILED_TO_SCHEDULE_SUBTASKS;
        }

        #if DEBUG
            printk(KERN_INFO "Last Assigned core %d has %d utilization\n", 
                    last_assigned_core,
                    core_utilization[last_assigned_core]);
        #endif


    }


    return return_value;
}

/* Function to assign priorites to all subtasks */

static int 
set_priority(void){
    
    // Iterators
    int j, k;
    unsigned int i;

    // Allocating memory to keep track of which subtask is assigned to which
    // core and sorting the array
    for(i = 0; i < NUM_CORES; ++i){

        int cur_priority = MIN_PRIORITY; // Initializing priority insertion val


        #if DEBUG
            printk(KERN_INFO"Allocating memory for core %d\n", i) ;
        #endif 

        // If there are no sub-tasks in the core, move to the next core
        if(core_num_subtasks[i] == 0) {
            continue;
        }

        core_sub_tasks[i] = kmalloc(
                            sizeof(struct sub_task *) * core_num_subtasks[i],
                            GFP_KERNEL);

        
        
        #if DEBUG
            printk(KERN_INFO"Checking allocation for core %d\n", i) ;
            printk(KERN_INFO"Allocation value is %p\n", core_sub_tasks[i]) ;
        #endif 


        // If kmalloc fails, free all memory
        if(!core_sub_tasks[i]){ 
            #if DEBUG
                printk(KERN_INFO"Failed allocation %d\n", i) ;
            #endif 


            for(j = 0; j <= i; ++j){                    
                kfree(core_sub_tasks[j]);
                core_sub_tasks[j] = 0;
            }
            deallocate_memory();
            return -INSUFFICIENT_MEMORY;
        }


        #if DEBUG
            printk(KERN_INFO"Populating for core %d\n", i) ;
        #endif 


        // Populating populating core_sub_task
        for(j = 0, k = 0; k < TOTAL_SUBTASKS; ++k){                    
            if(sorted_sub_tasks[k]->core == i){
            #if DEBUG
                printk(KERN_INFO"Inserting task %d\n into slot %d", k,j);
            #endif 

                if(j > core_num_subtasks[i]){
                    printk(KERN_INFO"Invalid Core Task Indesx %d", j);
                }

                (core_sub_tasks[i])[j]  = sorted_sub_tasks[k]; // Setting the pointer 
                ++j;
            }
        }

        #if DEBUG
            printk(KERN_INFO"Population complete for core %d\n", i) ;
        #endif 

        #if DEBUG 
        // Pringing all subtask core and relative_deadlines
        for(k = 0; k < core_num_subtasks[i]; ++k){                    
            printk(KERN_INFO"Core: %d Relative deadline: %lu\n", 
                   (core_sub_tasks[i])[k]->core,
                   (core_sub_tasks[i])[k]->relative_deadline);


        }

        #endif



        // Sorting core_sub_tasks in ascending order by relative deadline
        sort(core_sub_tasks[i],
             core_num_subtasks[i],
             sizeof(struct sub_task *),
             &compare_by_relative_deadline, NULL);

         #if DEBUG
            printk(KERN_INFO "Sorting complete for core %d\n", i) ;
        #endif 

       

        #if DEBUG 
        // Pringing all subtask core and relative_deadlines
        for(k = 0; k < core_num_subtasks[i]; ++k){                    

            printk(KERN_INFO"Core: %d Relative deadline: %lu\n", 
                   (core_sub_tasks[i])[k]->core,
                   (core_sub_tasks[i])[k]->relative_deadline);

            print_sub_tasks(core_sub_tasks[i][k]);
        }
        #endif

        // Assigning priorities
        for(k = 0; k < core_num_subtasks[i]; ++k){
            core_sub_tasks[i][k]->priority = cur_priority++;
            if(cur_priority > MAX_PRIORITY) {
                cur_priority = MAX_PRIORITY;
            }
        }



    } 


    return 0;
    
}



/* 
int init_tasks(void)

This fucntion initializes cumulative_execution_time, relative_deadline,
and utilization of all subtasks */
static int 
init_tasks(void){

    int i, j;

    // Looping through each task
    for(i = 0; i < NUM_TASKS; ++i){
        
        // Pointer to array for each ete subtask 
        struct sub_task * ete_sub_tasks;  

        ulong cumulative_execution_time, relative_deadline, utilization;

        const int CUR_NUM_SUB_TASKS = ete_tasks[i].subtasks_num;

        cumulative_execution_time = 0;
    
        printk(KERN_INFO "Task: %d \n subtasks_num: %d \nperiod_ms: %lu\n       \
                execution_time: %lu \n",
                ete_tasks[i].task_num,
                ete_tasks[i].subtasks_num,
                ete_tasks[i].period_ms,
                ete_tasks[i].execution_time);

        // Storing the array of subtasks for current tasks
        ete_sub_tasks = sub_tasks[i]; 

        // Looping through sub-tasks 
        for(j = 0; j <  CUR_NUM_SUB_TASKS; ++j){

            relative_deadline = 0;
            utilization = 0;

            // Calculating cumulative execution time. 
            cumulative_execution_time +=
                 ete_sub_tasks[j].execution_time;

            // Calculating relative deadline rounded down to the nearest whole 
            // whole number. This is the safer as earlier deadlines have a 
            // higher priority. 
            
            relative_deadline = (ete_tasks[i].period_ms *
                 cumulative_execution_time) / ete_tasks[i].execution_time;
            
            // Calculating utilization in a scale of 1 to 100 rounded up to
            // the nearest whole number, as this will make it safer. There will
            // be no utilization = 0 and a cores will not be over-filled.
            //
            utilization = 
            (ete_sub_tasks[j].execution_time * MAX_UTILIZATION)/ ete_tasks[i].period_ms;
            
            // Adding 1 to utilization to round up based on whether mod is > 0.
            utilization +=
                (!!( ete_tasks[i].period_ms % ete_sub_tasks[j].execution_time));


            // Setting the varialbes in the sub-tasks 
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

    
    printk(KERN_INFO "Setting priority for each sub-task\n");
    if(set_priority() != 0){
        printk(KERN_ALERT "Insufficient Memory. Stopping!\n");
        return INSUFFICIENT_MEMORY;
    }
    printk(KERN_INFO "Setting priority complete\n");

    // If inside calibration mode
    if(!(strcmp(exec_mode, CALIBRATE) == 0)){
        unsigned int i;

        ktime_t thread_wakeup_delay = ktime_set(0,
                                             CALIBRATION_THREAD_START_DELAY_NS);
        hrtimer_init(&thread_wakeup_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        
        thread_wakeup_timer.function = &wake_up_all_core_threads;


        // Creating calibration thread for each core
        for(i = 0; i < NUM_CORES; ++i){
            struct sched_param sp_calibration_thread;
            char * thread_name = "calibration_thread";

            printk("Creating thread %s for core %d", thread_name, i);

            core_threads[i] = kthread_create(calibrate_thread_function,
                                             core_sub_tasks[i],
                                             thread_name);

            
            sp_calibration_thread.sched_priority = 0;

            if(IS_ERR(core_threads[i])){
                printk(KERN_ALERT "Calibration thread creation failed.\n \
                                   Stopping calibration.");
                deallocate_memory(); 
                return -FAILED_TO_CREATE_THREAD;
            }

            kthread_bind(core_threads[i], i);

            if(!(sched_setscheduler(core_threads[i],
                                SCHED_FIFO,
                                &sp_calibration_thread)) != 0)
            {
                printk(KERN_ALERT "Failed to schedule calibration thread\n");
                deallocate_memory();
                return -FAILED_TO_SET_SCHEDULER;
            }

            wake_up_process(core_threads[i]);
        }



        // Timer to wake up threads after thread_wakeup_delay
        hrtimer_start(&thread_wakeup_timer,
                      thread_wakeup_delay,
                      HRTIMER_MODE_REL);


    }else{
    //else if(strcmp(exec_mode, RUN)){
        // Instantiating thread by core
        unsigned int core_num, task_num, thread_num;
        // Iterating through subtasks assigned to each core
        for(core_num = 0, thread_num = 0; core_num < NUM_CORES; ++core_num){
            int sub_task_num;
            for(sub_task_num = 0; 
                sub_task_num < core_num_subtasks[core_num];
                ++sub_task_num)
            {
                struct sched_param sp;
                struct sub_task * cur_sub_task = 
                                core_sub_tasks[core_num][sub_task_num];


                #if DEBUG
                    printk("Creating thread for task: \n");
                    print_sub_tasks(cur_sub_task);
                #endif


                        
                cur_sub_task->task = kthread_create(run_thread_function,
                                       cur_sub_task,
                                       "thread_%d",thread_num++);

                #if DEBUG
                    printk("Created thread %p\n", cur_sub_task->task);
                #endif

                
                if(IS_ERR(cur_sub_task->task)){
                    printk(KERN_ALERT "Failed to create a thread for the \
                                        following subtask:\n");

                    print_sub_tasks(cur_sub_task);
                    printk(KERN_ALERT "Stopping all execution\n");
                    deallocate_memory();
                    return -FAILED_TO_CREATE_THREAD; 
                }

                kthread_bind(cur_sub_task->task, core_num);

                sp.sched_priority = cur_sub_task->priority;
                
                // Setting scheduler
                if(sched_setscheduler(cur_sub_task->task,
                                        SCHED_FIFO,
                                        &sp) == -1)
                {
                    printk(KERN_ALERT"Failed to schedule the following subtask \
                    \n");
                    print_sub_tasks(cur_sub_task);
                    printk(KERN_ALERT "Stopping all execution\n");
                    deallocate_memory();
                    return -FAILED_TO_SET_SCHEDULER;
                }

                #if DEBUG
                    printk("Waking up process");
                    print_sub_tasks(cur_sub_task);
                #endif



                wake_up_process(cur_sub_task->task);



                #if DEBUG
                    printk("Thread creation complete\n");
                #endif

            }



        }

        #if DEBUG
            printk("Waking up first subtask in each task \n");
        #endif



        // Waking up each first task in all tasks
        for(task_num = 0; task_num < NUM_TASKS; ++task_num){
            print_sub_tasks(&(sub_tasks[task_num][0]));
            wake_up_process((sub_tasks[task_num][0].task));
        }

        #if DEBUG
            printk("Waking procedures complete!\n");
        #endif


 
        
    }
    
    /*else{
        printk(KERN_ALERT "Invalid module initialization parameter exec_mode=\
         %s. No calibration or run threads were created.\n", exec_mode);
    }*/


    
    return 0;

}

static void 
stop_threads_and_timers( void ){
    int task_num, sub_task_num;
    for(task_num = 0; task_num < NUM_TASKS; ++task_num){
        for(sub_task_num = 0; sub_task_num < TOTAL_SUBTASKS; 
            ++sub_task_num)
        {
            if(sub_tasks[task_num][sub_task_num].task){
                kthread_stop(sub_tasks[task_num][sub_task_num].task);
                hrtimer_cancel(&(sub_tasks[task_num][sub_task_num].timer));
            }
            
        }

    }

}



static void 
ete_exit(void)
{
    // Iterators
    printk(KERN_INFO "Unloaded kernel_memory module\n");
    stop_threads_and_timers();



    deallocate_memory();

}

module_init(ete_init);
module_exit(ete_exit);

MODULE_LICENSE ("GPL");
