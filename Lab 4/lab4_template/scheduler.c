#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>



// TODO: Add more fields to this struct
struct job {
    int id;
    int arrival;
    int length;
    struct job *next;
};

/*** Globals ***/ 
int seed = 100;

//This is the start of our linked list of jobs, i.e., the job list
struct job *head = NULL;

/*** Globals End ***/

/*Function to append a new job to the list*/
void append(int id, int arrival, int length){
  // create a new struct and initialize it with the input data
  struct job *tmp = (struct job*) malloc(sizeof(struct job));

  //tmp->id = numofjobs++;
  tmp->id = id;
  tmp->length = length;
  tmp->arrival = arrival;

  // the new job is the last job
  tmp->next = NULL;

  // Case: job is first to be added, linked list is empty 
  if (head == NULL){
    head = tmp;
    return;
  }

  struct job *prev = head;

  //Find end of list 
  while (prev->next != NULL){
    prev = prev->next;
  }

  //Add job to end of list 
  prev->next = tmp;
  return;
}


/*Function to read in the workload file and create job list*/
void read_workload_file(char* filename) {
  int id = 0;
  FILE *fp;
  size_t len = 0;
  ssize_t read;
  char *line = NULL,
       *arrival = NULL, 
       *length = NULL;

  struct job **head_ptr = malloc(sizeof(struct job*));

  if( (fp = fopen(filename, "r")) == NULL)
    exit(EXIT_FAILURE);

  while ((read = getline(&line, &len, fp)) > 1) {
    arrival = strtok(line, ",\n");
    length = strtok(NULL, ",\n");
       
    // Make sure neither arrival nor length are null. 
    assert(arrival != NULL && length != NULL);
        
    append(id++, atoi(arrival), atoi(length));
  }

  fclose(fp);

  // Make sure we read in at least one job
  assert(id > 0);

  return;
}


void policy_FIFO(struct job *head) {
  // TODO: Fill this in
  int t = 0;

  printf("Execution trace with FIFO:\n");
  for (struct job *curr = head; curr != NULL; curr = curr->next) {
      printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", t, curr->id, curr->arrival, curr->length);
      t += curr->length;
  }
  printf("End of execution with FIFO.\n");
  
  return;
}

void analyze_FIFO(struct job *head) {
  // TODO: Fill this in
  int time = 0;
  int wait = 0;
  int turnaround = 0;

  double avg_wait = 0;
  double avg_turnaround = 0;

  for (struct job *curr = head; curr != NULL; curr = curr->next) {
      wait = time - curr->arrival;
      turnaround = wait + curr->length;
      printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curr->id, wait, turnaround, wait);
      time += curr->length;

      avg_wait += wait;
      avg_turnaround += turnaround;
      if (curr->next == NULL) {
          avg_wait /= curr->id + 1;
          avg_turnaround /= curr->id + 1;
          printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", avg_wait, avg_turnaround, avg_wait);
      }
  }
  
  return;
}

int main(int argc, char **argv) {

 if (argc < 4) {
    fprintf(stderr, "missing variables\n");
    fprintf(stderr, "usage: %s analysis-flag policy workload-file\n", argv[0]);
		exit(EXIT_FAILURE);
  }

  int analysis = atoi(argv[1]);
  char *policy = argv[2],
       *workload = argv[3];

  // Note: we use a global variable to point to 
  // the start of a linked-list of jobs, i.e., the job list 
  read_workload_file(workload);

  if (strcmp(policy, "FIFO") == 0 ) {
    policy_FIFO(head);
    if (analysis) {
      printf("Begin analyzing FIFO:\n");
      analyze_FIFO(head);
      printf("End analyzing FIFO.\n");
    }

    exit(EXIT_SUCCESS);
  }

  // TODO: Add other policies 

	exit(EXIT_SUCCESS);
}
