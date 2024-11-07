//Written by Cyprian Siwik for Operating Systems, Project 2, 2024
//Part 2 of the project
//We set our buffer size, our indexes, our headers. We have our provider thread that adds stock to the shared buffer and our buyer thread that buys the stock off the shared buffer. This interaction keeps happening until all buyers buy something and there are no more buyers. All details of the process like allocation, signalling and waiting are documented as they occur in the file.


//header files for input output, time, pthread for support in multithreading, and boolean.
#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_SIZE 5     // Fixed buffer size as a constant. remember (x < 10) changing this value does have an effect on the speed of the code execution.
#define SLEEP_TIME 1      // defined sleep time in seconds

// Shared buffer and related variables
int buffer[BUFFER_SIZE];

int in = 0; //index for where the next item produced will be placed in the buffer
int out = 0; //index for the position on the buffer thats readyy to be consumed next
int count = 0; //just a general count for items in the buffer, if == BUFFER_SIZE, provider waits until something is bought, if == 0, consumer waits until something is produced

//  primitives for the synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //mutex for making sure only 1 thread can modify the buffer data at a time
pthread_cond_t bufferFull = PTHREAD_COND_INITIALIZER; // signal, wait for buffer to have space before producing more items
pthread_cond_t bufferEmpty = PTHREAD_COND_INITIALIZER; // signal, wait for buffer to havve items to buy.

// Thread tracking
int* items_bought;        // Track items bought by each buyer
bool program_running = true;  // flag used to control program termination
int total_buyers;        // Store N globally

// Flag to check if all buyers have bought at least one item, iterating through all buyers
bool all_buyers_served() {
    for (int i = 0; i < total_buyers; i++) {
        if (items_bought[i] == 0) {// if any of the total buyers bought less than one item, the boolean would return as false
            return false;
        }
    }
    return true;
}

// Provider thread function, responsible for adding the produced 'items' to the shared buffer
// this provider thread is just one thread, and will iterate from this while loop until the program_running flag is false. 
// it starts with an item id of 1 , then adds items into the buffer, signals the availability to buyers, increments the item id and buffer count,
// and locks and unlocks the mutex when appropriate. Any decrement of the buffer contents comes from the buyer threads.
void* provider(void* arg) {
    int item = 1;// first item to be produced
    
    while (program_running) { // only runs until program_running is set to false.
        pthread_mutex_lock(&mutex); //lock the mutex, locked until unlocked, prevents other threads from accessing
        
        while (count == BUFFER_SIZE && program_running) { // while buffer is full and program is still running:
            pthread_cond_wait(&bufferFull, &mutex); //whole thread goes into waiting state until there is room in the buffer.
        }
        
        if (!program_running) {
            pthread_mutex_unlock(&mutex); //unlock mutex before breaking out
            break; //exit the loop
        }
        
        buffer[in] = item; //place the produced item into the buffer
        printf("Provider produced item %d at position %d\n", item, in); //send statement to screen
        in = (in + 1) % BUFFER_SIZE; //update the in index
        count++; //increments the amount of items in the buffer
        item++; //increments the item # for the next produced item
        
        pthread_cond_signal(&bufferEmpty); //signals to any buyer that is waiting that an item is now available
        pthread_mutex_unlock(&mutex); //unlock the mutex to allow other threads to access the buffer items
        usleep(100000); // 0.1 second
    }
    return NULL; //exit provider thread
}

// Buyer thread function, responsible for consuming the items in the shared buffer.
void* buyer(void* arg) {
    int id = *((int*)arg);
    
    while (program_running) {
        pthread_mutex_lock(&mutex);
        
        // Wait if buffer is empty, as there then is no item to buy.
        while (count == 0 && program_running) { //while buffer empty and program is running
            pthread_cond_wait(&bufferEmpty, &mutex); //wait until buffer is supplied.
        }
        
        if (!program_running) {
            pthread_mutex_unlock(&mutex);// also unlocks mutex and breaks out of loop
            break;
        }
        
        // Consume item from buffer
        int item = buffer[out]; //item is taken out of the buffer, and assigned to the buyer
        printf("Buyer %d bought item %d from position %d\n", id, item, out); //print to output
        
        out = (out + 1) % BUFFER_SIZE; // we move to the next position in the buffer
        count--; // decrement the item count in the buffer as one was just bought
        items_bought[id]++; //increments the amount of items bought by this specific user.
        
        // Check if all buyers have been served
        if (all_buyers_served()) {
            printf("\nAll of the buyers have purchased at least one item!\n");
            program_running = false;// end the program if all buyers have bought something
            // here we signal all waiting threads to check program_running
            pthread_cond_broadcast(&bufferEmpty);
            pthread_cond_broadcast(&bufferFull);
        }
        
        pthread_cond_signal(&bufferFull); //signal to the provider that there is now space in the buffer so that it can exit its waiting phase if it was previously full
        pthread_mutex_unlock(&mutex); //unlock mutex to allow other threads to access the shared buffer
        
        sleep(SLEEP_TIME); //pause for 0.1 seconds before next purchase attempt
    }
    return NULL;
}
// main function, first checks for the correct number of command line arguments
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <N>\n", argv[0]);
        printf("  N: number of buyer threads\n"); //error message
        return 1;
    }
    
    // getting the number of buyers from command line
    total_buyers = atoi(argv[1]); //looking for the second argument
    if (total_buyers <= 0) {
        printf("Number of buyers must be greater than 0\n"); //error message
        return 1;
    }
    
    // Allocate array to track items bought by each buyer
    items_bought = (int*)calloc(total_buyers, sizeof(int));
    if (items_bought == NULL) {
        perror("Failed to allocate items_bought array");
        return 1;
    }
    
    //starting program statements
    printf("- Buffer size: %d \n", BUFFER_SIZE);
    printf("- Number of buyers: %d\n", total_buyers);
    
    // create the threads
    pthread_t provider_thread;

    //allocating memory for the buyer threads 
    pthread_t* buyer_threads = (pthread_t*)malloc(total_buyers * sizeof(pthread_t));

    //storing of buyer IDs
    int* buyer_ids = (int*)malloc(total_buyers * sizeof(int));
    
    if (!buyer_threads || !buyer_ids) {
        perror("Failed to allocate thread arrays");
        free(items_bought);
        return 1;
    }
    
    // Create provider thread
    if (pthread_create(&provider_thread, NULL, provider, NULL) != 0) {
        perror("Failed to create provider thread");
        free(items_bought);
        free(buyer_threads);
        free(buyer_ids);
        return 1;
    }
    
    // Create buyer threads
    for (int i = 0; i < total_buyers; i++) {
        buyer_ids[i] = i;
        if (pthread_create(&buyer_threads[i], NULL, buyer, &buyer_ids[i]) != 0) {
            perror("Failed to create buyer thread");
            free(items_bought);
            free(buyer_threads);
            free(buyer_ids);
            return 1;
        }
    }
    
    // waits for all threads to complete execution
    pthread_join(provider_thread, NULL);
    for (int i = 0; i < total_buyers; i++) {
        pthread_join(buyer_threads[i], NULL); //main will pause and wait until all threads are completed. NULL because we dont care about the exit status.
    }
    
    //free the dynamically allocated memory for items bought, memory for the buyer threads, and for the buyer IDs
    free(items_bought); 
    free(buyer_threads);
    free(buyer_ids);

    //exit program successfully 
    return 0;
}
