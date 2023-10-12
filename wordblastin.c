#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MAX_SIZE 2000 // Number of words to hold in shared array
#define WORD_SIZE 10  // Size of each word
// Global arrays for words and counts
char* wordStack[MAX_SIZE];
int countStack[MAX_SIZE];
//use this
char *delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

int count = 0;     // To keep track of elements in the stack
int remainingWords = 0;  // For the remaining bytes to use with the last pthread
int fd, chunkSize;
pthread_mutex_t lock;

// Initialize stack to put the words in
void initWordStack() {
    for (int i = 0; i < MAX_SIZE; i++) {
        wordStack[i] = NULL;
        countStack[i] = 0;
    }
}

// Adds a word to a shared stack with thread safety.
void pushWord(char* word) {
    pthread_mutex_lock(&lock); // Lock to protect shared data

    for (int i = 0; i < count; i++) {
        if (strcmp(wordStack[i], word) == 0) {
            countStack[i]++;
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    if (count < MAX_SIZE) {
        wordStack[count] = strdup(word);
        countStack[count]++;
        count++;
    }

    pthread_mutex_unlock(&lock);
}

// This function is executed by each thread to process a portion of a file in chunks.
void *processWord(void *ptr) {
   
    char * buffer;
    char * token;

    // Allocate memory for a buffer to store file data (chunk + remainingWords)
    buffer = malloc(chunkSize+remainingWords);

    // Read a chunk of data from the file into the buffer
    read(fd, buffer, chunkSize+remainingWords);

    // Tokenize the buffer using strtok_r and process each token
    do {
        token = strtok_r(buffer, delim, &buffer);

        // Check if the token has 6 or more characters
        if (token && (int)strlen(token) > 5) {
            pushWord(token);  // Add the word to a shared stack or update its count
        }
    } while (token);
}

int main(int argc, char *argv[]) {
    //***TO DO*** Look at arguments, open file, divide by threads
    // Allocate and Initialize storage structures
    int mutexLock, err1, fileSize;
    int threadCount = strtol(argv[2], NULL, 10);  // Number of threads from args

    // Initialize stack
    initWordStack();

    // Initialize mutex lock
    if (mutexLock = pthread_mutex_init(&lock, NULL)) {
        printf("ERROR: Mutex init failed [%d]\n", mutexLock);
        exit(EXIT_FAILURE);
    }

    // Open file descriptor
    fd = open(argv[1], O_RDONLY);

    // Get file size with lseek
    fileSize = lseek(fd, 0, SEEK_END);

    // Set file position back to the beginning
    lseek(fd, 0, SEEK_SET);

    // Chunk size = file size divided by number of threads
    chunkSize = fileSize / threadCount;

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************

    // *** TO DO *** start your thread processing
    // Create pthreads
    pthread_t thread[threadCount];

    // Create threads for parallel processing.
    for (int i = 0; i < threadCount; i++) {
        int threadIndex = i; // Separate variable for each thread

        // Adjust 'remainingWords' for the last thread.
        if (i == threadCount - 1) {
            remainingWords = fileSize % threadCount;
        }

        // Create threads with error checking.
        if (err1 = pthread_create(&thread[i], NULL, processWord, (void*)&threadIndex)) {
            printf("ERROR: Thread creation failed [%d]\n", err1);
            exit(EXIT_FAILURE);
        }
    }

    // Join threads after they run
    for (int i = 0; i < threadCount; i++) {
        pthread_join(thread[i], NULL);
    }

    // ***TO DO *** Process TOP 10 and display
    
    // Selection sort algorithm for decreasing order of the stack contents
    for (int i = 0; i < MAX_SIZE - 1; i++) {
        int maxIndex = i;  // Assume the current index has the maximum count

        // Find the index of the maximum count in the remaining elements
        for (int j = i + 1; j < MAX_SIZE; j++) {
            if (countStack[j] > countStack[maxIndex]) {
                maxIndex = j;
            }
        }

        // Swap the counts
        int countTemp = countStack[i];
        countStack[i] = countStack[maxIndex];
        countStack[maxIndex] = countTemp;

        // Swap the corresponding words in wordStack
        char* wordTemp = wordStack[i];
        wordStack[i] = wordStack[maxIndex];
        wordStack[maxIndex] = wordTemp;
    }

    // Output top 10 6+ character words
    printf("\n\n");
    printf("Word Frequency Count on %s with %d threads\n", argv[1], threadCount);
    printf("Printing top 10 words 6 characters or more.\n");

    int topCount = 10; // Take minimum of count and 10
    for (int i = 0; i < topCount; i++) {
        printf("Number %d is %s with a count of %d\n", i + 1, wordStack[i], countStack[i]);
    }

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec) {
        --sec;
        n_sec = n_sec + 1000000000L;
    }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    // ***TO DO *** cleanup
    // Close file descriptor
    close(fd);

    // Destroy mutex lock
    pthread_mutex_destroy(&lock);

    // Free dynamically allocated wordStack buffers
    int i = 0;
    while (i < count) {
        free(wordStack[i]);
        i++;
    }

    return 0;
}
