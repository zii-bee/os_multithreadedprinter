/**
 * paragraph_threads.c
 * 
 * this program demonstrates thread synchronization using semaphores by printing
 * a paragraph where each thread is responsible for printing specific words.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <semaphore.h>
 #include <unistd.h>
 #include <string.h>
 #include <time.h>
 
 #define NUM_THREADS 5
 
 // the paragraph to be printed
 const char *paragraph = "Computer science is the study of computation, automation, and information. "
                        "Computer science spans theoretical disciplines to practical disciplines. "
                        "Computer science is generally considered an area of academic research and "
                        "distinct from computer programming.";
 
 // thread data structure
 typedef struct {
     int thread_id;
     char **words;        // array of words for this thread
     int word_count;      // number of words assigned to this thread
     sem_t *sem_wait;     // semaphore to wait on
     sem_t *sem_signal;   // semaphore to signal
     int is_chaos_mode;   // flag for chaos mode
 } thread_data_t;
 
 // global variables
 sem_t semaphores[NUM_THREADS];
 char **all_words = NULL;
 int total_words = 0;
 
 /**
  * splits the paragraph into words
  * returns the total number of words
  */
 int split_paragraph_into_words() {
     // count the spaces to determine number of words
     int spaces = 0;
     for (int i = 0; paragraph[i] != '\0'; i++) {
         if (paragraph[i] == ' ') {
             spaces++;
         }
     }
     
     // total words = spaces + 1
     total_words = spaces + 1;
     
     // allocate memory for array of word pointers
     all_words = (char**)malloc(total_words * sizeof(char*));
     if (all_words == NULL) {
         perror("malloc failed");
         exit(EXIT_FAILURE);
     }
     
     // create a copy of the paragraph to tokenize
     char *paragraph_copy = strdup(paragraph);
     if (paragraph_copy == NULL) {
         perror("strdup failed");
         free(all_words);
         exit(EXIT_FAILURE);
     }
     
     // tokenize the paragraph and store each word
     char *token = strtok(paragraph_copy, " ");
     int word_idx = 0;
     
     while (token != NULL && word_idx < total_words) {
         all_words[word_idx] = strdup(token);
         if (all_words[word_idx] == NULL) {
             perror("strdup failed");
             // cleanup already allocated memory
             for (int i = 0; i < word_idx; i++) {
                 free(all_words[i]);
             }
             free(all_words);
             free(paragraph_copy);
             exit(EXIT_FAILURE);
         }
         word_idx++;
         token = strtok(NULL, " ");
     }
     
     free(paragraph_copy);
     return total_words;
 }
 
 /**
  * frees the memory allocated for words
  */
 void free_words() {
     if (all_words != NULL) {
         for (int i = 0; i < total_words; i++) {
             free(all_words[i]);
         }
         free(all_words);
         all_words = NULL;
     }
 }
 
 /**
  * thread function that prints assigned words
  * waits on its semaphore, prints its part, and signals the next thread
  */
 void* print_thread(void *arg) {
     thread_data_t *data = (thread_data_t *)arg;
     
     // loop through all words assigned to this thread
     for (int i = 0; i < data->word_count; i++) {
         if (!data->is_chaos_mode) {
             // normal mode - use semaphores for synchronization
             sem_wait(data->sem_wait);
         }
         
         // add random delay (10-100ms)
         usleep((rand() % 91 + 10) * 1000);
         
         // print the word
         printf("Thread %d: %s ", data->thread_id + 1, data->words[i]);
         
         // add newline after a complete phrase
         if (strchr(data->words[i], '.') != NULL) {
             printf("\n");
         }
         
         if (!data->is_chaos_mode) {
             // normal mode - signal the next thread
             sem_post(data->sem_signal);
         }
     }
     
     return NULL;
 }
 
 /**
  * initializes semaphores
  */
 void init_semaphores() {
     for (int i = 0; i < NUM_THREADS; i++) {
         // initialize all semaphores to 0 except the first one
         if (sem_init(&semaphores[i], 0, (i == 0) ? 1 : 0) != 0) {
             perror("sem_init failed");
             exit(EXIT_FAILURE);
         }
     }
 }
 
 /**
  * destroys semaphores
  */
 void destroy_semaphores() {
     for (int i = 0; i < NUM_THREADS; i++) {
         sem_destroy(&semaphores[i]);
     }
 }
 
 /**
  * resets semaphores for a new run
  */
 void reset_semaphores() {
     // first destroy existing semaphores
     destroy_semaphores();
     // then initialize them again
     init_semaphores();
 }
 
 /**
  * prints paragraph using multiple threads
  * mode: 0 for normal mode, 1 for chaos mode
  */
 void print_paragraph(int mode) {
     pthread_t threads[NUM_THREADS];
     thread_data_t thread_data[NUM_THREADS];
     
     // words per thread (at minimum)
     int words_per_thread = total_words / NUM_THREADS;
     
     // extra words to distribute among the first few threads
     int extra_words = total_words % NUM_THREADS;
     
     // initialize thread data and create threads
     int word_index = 0;
     
     for (int i = 0; i < NUM_THREADS; i++) {
         thread_data[i].thread_id = i;
         
         // calculate number of words for this thread
         thread_data[i].word_count = words_per_thread + (i < extra_words ? 1 : 0);
         
         // allocate memory for words
         thread_data[i].words = (char**)malloc(thread_data[i].word_count * sizeof(char*));
         if (thread_data[i].words == NULL) {
             perror("malloc failed");
             exit(EXIT_FAILURE);
         }
         
         // assign words to this thread
         for (int j = 0; j < thread_data[i].word_count; j++) {
             thread_data[i].words[j] = all_words[word_index++];
         }
         
         // set semaphores for synchronization
         thread_data[i].sem_wait = &semaphores[i];
         thread_data[i].sem_signal = &semaphores[(i + 1) % NUM_THREADS];
         thread_data[i].is_chaos_mode = mode;
         
         // create thread
         if (pthread_create(&threads[i], NULL, print_thread, (void*)&thread_data[i]) != 0) {
             perror("pthread_create failed");
             exit(EXIT_FAILURE);
         }
     }
     
     // wait for all threads to complete
     for (int i = 0; i < NUM_THREADS; i++) {
         pthread_join(threads[i], NULL);
         
         // free allocated memory for words
         free(thread_data[i].words);
     }
 }
 
 int main() {
     // seed the random number generator
     srand(time(NULL));
     
     // split the paragraph into words
     split_paragraph_into_words();
     
     // initialize semaphores
     init_semaphores();
     
     // print in normal mode
     printf("\n=== Normal Mode (With Semaphore Synchronization) ===\n");
     print_paragraph(0);
     
     // reset semaphores for chaos mode
     reset_semaphores();
     
     // wait a moment to visually separate the outputs
     sleep(1);
     
     // print in chaos mode
     printf("\n=== Chaos Mode (Without Semaphore Synchronization) ===\n");
     print_paragraph(1);
     
     // cleanup
     destroy_semaphores();
     free_words();
     
     return 0;
 }