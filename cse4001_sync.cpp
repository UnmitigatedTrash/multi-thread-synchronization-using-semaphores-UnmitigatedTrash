#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
#include <vector>
using namespace std;

// Set the number of iterations for each problem
int iterations = 5;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue) {
        sem_init(&mSemaphore, 0, initialValue);
    }

    // Destructor
    ~Semaphore() {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // Wait
    void wait() {
        sem_wait(&mSemaphore);
    }

    // Signal
    void signal() {
        sem_post(&mSemaphore);
    }
    
private:
    sem_t mSemaphore;
};

/*
    Lightswitch Function
*/
class Lightswitch {
private:
    int counter;
    Semaphore mutex;

public:
    // Create lightswitch object
    Lightswitch() : counter(0), mutex(1) {}

    // Lock door of room
    void lock(Semaphore &semaphore) {
        mutex.wait();
        counter++;

        if(counter == 1) {
            semaphore.wait();
        }

        mutex.signal();
    }

    // Unlock door of room
    void unlock(Semaphore &semaphore) {
        mutex.wait();
        counter--;

        if(counter == 0) {
            semaphore.signal();
        }

        mutex.signal();
    }
};

/*
----------------------------------------------------------------------
    Problem 1 Solution
----------------------------------------------------------------------
*/
// Intialize semaphores for solution
Lightswitch readSwitch;
Semaphore roomEmpty(1);
Semaphore turnstile(1);

// No-Starve Writer Function
void *No_Starve_Writer(void *threadID) {
    // Thread number
    int x = (long)threadID;

    for(int i = 0; i < iterations; i++) {   // originally while(1), but changed to allow program to end
        sleep(1);

        printf("Writer %d: waiting in turnstile...\n", x);
        fflush(stdout);

        // Turnstile ensures fairness with readers
        turnstile.wait();
        // Wait until room is empty
        roomEmpty.wait();

        printf("Writer %d: entering room.\n", x);
        printf("Writer %d: writing...\n", x);
        fflush(stdout);

        // Simulate writing
        sleep(2);

        printf("Writer %d: exiting room.\n", x);
        fflush(stdout);

        // Let thread out and signify room is empty
        turnstile.signal();
        roomEmpty.signal();
    }
    return NULL;
}

// No-Starve Reader Function
void *No_Starve_Reader(void *threadID) {
    // Thread number
    int x = (long)threadID;

    for(int i = 0; i < iterations; i++) { // originally while(1), but changed to allow program to end
        printf("Reader %d: waiting in turnstile...\n", x);
        fflush(stdout);

        // Turnstile ensures fairness with writers
        turnstile.wait();
        // Release turnstile for next thread
        turnstile.signal();

        // Reader locks room
        readSwitch.lock(roomEmpty);

        printf("Reader %d: entering room.\n", x);
        printf("Reader %d: reading...\n", x);
        fflush(stdout);

        // Simulate reading
        sleep(2);

        printf("Reader %d: exiting room.\n", x);
        fflush(stdout);

        // Reader unlocks room for next thread
        readSwitch.unlock(roomEmpty);

        sleep(1);
    }
    return NULL;
}

/*
----------------------------------------------------------------------
    Problem 2 Solution
----------------------------------------------------------------------
*/
//Initialize semaphores for solution
Lightswitch writeSwitch;
Semaphore noReaders(1);
Semaphore noWriters(1);

// Writer-Priority Writer Function
void *Writer_Priority_Writer(void *threadID) {
    // Thread number
    int x = (long)threadID;

    for(int i = 0; i < iterations; i++) { // originally while(1), but changed to allow program to end
        sleep(1); //sleep(3);

        printf("Writer %d: Requesting access to write...\n", x);
        fflush(stdout);

        // Prevent readers from entering
        writeSwitch.lock(noReaders);
        // Wait until no writers are in the room
        noWriters.wait();

        printf("Writer %d: entering room.\n", x);
        printf("Writer %d: writing...\n", x);
        fflush(stdout);

        // Simulate writing
        sleep(2);

        printf("Writer %d: exiting room.\n", x);
        fflush(stdout);

        // Allow another writer to enter or allow readers in
        noWriters.signal();
        // Unlock readers so they can enter room
        writeSwitch.unlock(noReaders);
    }
    return NULL;
}

// Writer-Priority Reader Function
void *Writer_Priority_Reader(void *threadID) {
    // Thread number
    int x = (long)threadID;

    for(int i = 0; i < iterations; i++) { // originally while(1), but changed to allow program to end
        printf("Reader %d: waiting in turnstile...\n", x);
        fflush(stdout);

        // Turnstile ensures writers get priority by preventing new readers from entering
        noReaders.wait();

        // Join the active reader group or lock noWriters if first reader
        readSwitch.lock(noWriters);

        // Release turnstile for next thread
        noReaders.signal();

        printf("Reader %d: entering room.\n", x);
        printf("Reader %d: reading...\n", x);
        fflush(stdout);

        // Simulate reading
        sleep(2);

        printf("Reader %d: exiting room.\n", x);
        fflush(stdout);

        // Reader unlocks room so writers can enter
        readSwitch.unlock(noWriters);

        sleep(1);
    }
    return NULL;
}

/*
----------------------------------------------------------------------
    Problem 3 Solution
----------------------------------------------------------------------
*/

// Initialize number of philosophers
const int num_philosophers = 5;

// Create a semaphore array for philosophers 
Semaphore footman(num_philosophers-1);
std::vector<Semaphore> forks(num_philosophers, Semaphore(1));

// Get left fork
int left(int i) {
    return i;
}

// Get right fork
int right(int i) {
    return (i + 1) % 5;
}

// Pick up forks
void get_forks_1(int i) {
    // Get the footman semaphore to limit philosophers at table
    footman.wait();

    printf("Philosopher %d: waiting to pick up forks...\n", i);
    fflush(stdout);
    
    // Grab forks to prevent others from using them
    forks[right(i)].wait();
    forks[left(i)].wait();

    printf("Philosopher %d: pick up right fork %d & left fork %d\n", i, right(i), left(i));
    fflush(stdout);
}

// Put down forks
void put_forks_1(int i) {
    // Release forks so other philosophers can use them
    forks[right(i)].signal();
    forks[left(i)].signal();

    printf("Philosopher %d: put down right fork %d & left fork %d\n", i, right(i), left(i));
    fflush(stdout);

    // Release footman semaphore so another philosopher can enter
    footman.signal();
}

// Philosopher in Solution 1
void *Philosopher_1(void *threadID) {
    // Thread number
    int x = (long)threadID;

    for(int i = 0; i < iterations; i++) { // originally while(1), but changed to allow program to end
        printf("Philosopher %d: thinking...\n", x);
        fflush(stdout);
        
        // Simulate Philosopher thinking
        sleep(2);

        // Philosopher grabs forks
        get_forks_1(x);

        sleep(1);
        
        printf("Philosopher %d: eating...\n", x);
        fflush(stdout);

        // Simulate Philosopher eating
        sleep(2);

        // Philosopher puts down forks
        put_forks_1(x);
    }
    return NULL;
}

/*
----------------------------------------------------------------------
    Problem 4 Solution
----------------------------------------------------------------------
*/

// Pick up forks asymmetrically
void get_forks_2(int i) {
    //Even: Grab right then left
    if(i % 2 == 0) {
        printf("Philosopher %d: waiting to pick up forks...\n", i);
        fflush(stdout);

        // Grab right fork then left fork to prevent others from using them
        forks[right(i)].wait();
        forks[left(i)].wait();

        printf("Philosopher %d: pick up right fork %d & left fork %d\n", i, right(i), left(i));
        fflush(stdout);
    }
    // Odd: Grab left then right
    else {
        printf("Philosopher %d: waiting to pick up forks...\n", i);
        fflush(stdout);

         // Grab left fork then right fork to prevent others from using them
        forks[left(i)].wait();
        forks[right(i)].wait();

        printf("Philosopher %d: pick up left fork %d & right fork %d\n", i, left(i), right(i));
        fflush(stdout);
    }
}

// Put down forks asymmetrically
void put_forks_2(int i ) {
    // Release forks so other philosophers can use them
    forks[left(i)].signal();
    forks[right(i)].signal();

    printf("Philosopher %d: put down left fork %d & right fork %d\n", i, left(i), right(i));
    fflush(stdout);
}

// Philosopher in Solution 1
void *Philosopher_2(void *threadID) {
    // Thread number
    int x = (long)threadID;

    for(int i = 0; i < iterations; i++) { // originally while(1), but changed to allow program to end
        printf("Philosopher %d: thinking...\n", x);
        fflush(stdout);
        
        // Simulate Philosopher thinking
        sleep(2);

        // Philosopher grabs forks
        get_forks_2(x);

        sleep(1);
        
        printf("Philosopher %d: eating...\n", x);
        fflush(stdout);

        // Simulate Philosopher eating
        sleep(2);

        // Philosopher puts down forks
        put_forks_2(x);
    }
    return NULL;
}

/*
----------------------------------------------------------------------
    Main
----------------------------------------------------------------------
*/

void create_threads(int amount, pthread_t *thread, void *(*thread_type)(void*)) {
    // Create the writers 
    for(long p = 0; p < amount; p++) {
        long x = p;
        
        if(thread_type != Philosopher_1 && thread_type != Philosopher_2) {
            x = p + 1;
        }

        int rc = pthread_create(&thread[p], NULL, thread_type, (void *) (x));
        if(rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }
}

int main(int argc, char **argv) {    
    // Gets argument for which solution is chosen
    int input = atoi(argv[1]);
    
    //Initialize number of readers and writers
    const int readers = 5;
    const int writers = 5;
    const int philosophers = num_philosophers;

    // Create threads for readers and writers
    pthread_t writerThread[writers];
    pthread_t readerThread[readers];
    pthread_t philosopherThread[philosophers];

    // Execute solution based on inputted argument
    switch(input) {
        // Execute problem 1: No-starve
        case 1:
            // Create threads
            create_threads(writers, writerThread, No_Starve_Writer);
            create_threads(readers, readerThread, No_Starve_Reader);

            // Wait for threads to finish
            for (int i = 0; i < writers; i++) pthread_join(writerThread[i], NULL);
            for (int i = 0; i < readers; i++) pthread_join(readerThread[i], NULL);

            break;
        // Execute problem 2: Writer-priority
        case 2:
            // Create threads
            create_threads(writers, writerThread, Writer_Priority_Writer);
            create_threads(readers, readerThread, Writer_Priority_Reader);

            //Wait for threads to finish
            for (int i = 0; i < writers; i++) pthread_join(writerThread[i], NULL);
            for (int i = 0; i < readers; i++) pthread_join(readerThread[i], NULL);

            break;
        // Execute problem 3: Dining Philosophers solution 1
        case 3:
            // Create threads
            create_threads(philosophers, philosopherThread, Philosopher_1);

            // Wait for threads to finish
            for (int i = 0; i < philosophers; i++) pthread_join(philosopherThread[i], NULL);
            
            break;
        // Execute problem 4: Philosopher solution 2
        case 4:
            // Create threads
            create_threads(philosophers, philosopherThread, Philosopher_2);
            
            // Wait for threads to finish
            for (int i = 0; i < philosophers; i++) pthread_join(philosopherThread[i], NULL);

            break;
    }

    printf("Main: program completed. Exiting.\n");
    return 0;

} /* main() */