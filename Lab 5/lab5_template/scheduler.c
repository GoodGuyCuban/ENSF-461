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
    int tickets;
    struct job *next;
    int remainingTime;
};

/*** Globals ***/ 
int seed = 100;

//This is the start of our linked list of jobs, i.e., the job list
struct job *head = NULL;

/*** Globals End ***/

/*Function to append a new job to the list*/
void append(int id, int arrival, int length, int tickets){
  // create a new struct and initialize it with the input data
  struct job *tmp = (struct job*) malloc(sizeof(struct job));

  //tmp->id = numofjobs++;
  tmp->id = id;
  tmp->length = length;
  tmp->arrival = arrival;
  tmp->tickets = tickets;
  tmp->remainingTime = length;

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
  int tickets = 0;

  struct job **head_ptr = malloc(sizeof(struct job*));

  if( (fp = fopen(filename, "r")) == NULL)
    exit(EXIT_FAILURE);

  while ((read = getline(&line, &len, fp)) > 1) {
    arrival = strtok(line, ",\n");
    length = strtok(NULL, ",\n");
    tickets += 100;
       
    // Make sure neither arrival nor length are null. 
    assert(arrival != NULL && length != NULL);
        
    append(id++, atoi(arrival), atoi(length), tickets);
  }

  fclose(fp);

  // Make sure we read in at least one job
  assert(id > 0);

  return;
}


void policy_STCF(struct job *head, int slice) {
  int time = 0;

  while(1){
    struct job *lowest = head;
    struct job *curr = head;
    //find the smallest job that has arrived
    while(curr != NULL){
      if(curr->arrival <= time && curr->length < lowest->length){
        lowest = curr;
      }
      curr = curr->next;
    }
    
    if(lowest->arrival > time){
      time += slice;
    }
    //ex
    else{
      if(lowest->length <= slice){
        
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, lowest->id, lowest->arrival, lowest->length);
        time += lowest->length;
        lowest->length = 0;
        
      }
      else{
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, lowest->id, lowest->arrival, slice);
        time += slice;
        lowest->length -= slice;
      }
    }
    if(lowest->length == 0){
      lowest->arrival = INT_MAX;
    }
    //check if all jobs are done
    int done = 1;
    curr = head;
    while(curr != NULL){
      if(curr->length != 0){
        done = 0;
      }
      curr = curr->next;
    }
    if(done == 1){
      break;
    }
  
  
  }
  return;
}


  

void analyze_STCF(struct job *head, int slice) {
      int time = 0;
      double avgResponse = 0;
      double avgTurnaround = 0;
      double avgWait = 0;
      int count = 0;

  while(1){

    struct job *lowest = head;
    struct job *curr = head;
    //find the smallest job that has arrived
    int response = 0;
    int turnaround = 0;
    int wait = 0;
    
    while(curr != NULL){
      if(curr->arrival <= time && curr->length < lowest->length){
        lowest = curr;
      }
      curr = curr->next;
    }
    
    if(lowest->arrival > time){
      time += slice;
    }
    
    else{
      //remaingTime will be used as run time
      lowest->remainingTime = 0;

      if(lowest->length <= slice){
        

        if(response == 0){
          response = time - lowest->arrival;
        }

        turnaround = time - lowest->arrival;

        lowest->remainingTime += lowest->length;
        
        wait = time - lowest->arrival - lowest->remainingTime;

        printf("Job %d -- Response time: %d Turnaround: %d Wait: %d\n", lowest->id, response, turnaround, wait);
        avgResponse += response;
        avgTurnaround += turnaround;
        avgWait += wait;

        time += lowest->length;
        
        lowest->length = 0;
        
      }
      else{
        
        if(response == 0){
          response = time - lowest->arrival;
        }
        time += slice;
        lowest->remainingTime += slice;
        lowest->length -= slice;
      }
    }
    if(lowest->length == 0){
      lowest->arrival = INT_MAX;
    }
    //check if all jobs are done
    int done = 1;
    curr = head;
    while(curr != NULL){
      if(curr->length != 0){
        done = 0;
      }
      curr = curr->next;
      count++;
    }
    if(done == 1){
      break;
    }
  
  
  }
  avgResponse = avgResponse / count;
  avgTurnaround = avgTurnaround / count;
  avgWait = avgWait / count;

  printf("Average -- Response: %.2f Turnaround: %.2f Wait: %.2f\n", avgResponse, avgTurnaround, avgWait);

  return;
}

int main(int argc, char **argv) {

 if (argc < 5) {
    fprintf(stderr, "missing variables\n");
    fprintf(stderr, "usage: %s analysis-flag policy workload-file slice-length\n", argv[0]);
		exit(EXIT_FAILURE);
  }

  int analysis = atoi(argv[1]);
  char *policy = argv[2],
       *workload = argv[3];
  int slice = atoi(argv[4]);

  // Note: we use a global variable to point to 
  // the start of a linked-list of jobs, i.e., the job list 
  read_workload_file(workload);

  if (strcmp(policy, "STCF") == 0 ) {
    policy_STCF(head, slice);
    if (analysis) {
      printf("Begin analyzing STCF:\n");
      analyze_STCF(head, slice);
      printf("End analyzing STCF.\n");
    }

    exit(EXIT_SUCCESS);
  }

  // TODO: Add other policies 

	exit(EXIT_SUCCESS);
}
