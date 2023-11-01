#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h> //for unit_32
#define TRUE 1
#define FALSE 0

// Output file
FILE* output_file;

// TLB replacement strategy (FIFO or LRU)
char* strategy;

//PID
int PID = 0;

char** tokenize_input(char* input) {
    char** tokens = NULL;
    char* token = strtok(input, " ");
    int num_tokens = 0;

    while (token != NULL) {
        num_tokens++;
        tokens = realloc(tokens, num_tokens * sizeof(char*));
        tokens[num_tokens - 1] = malloc(strlen(token) + 1);
        strcpy(tokens[num_tokens - 1], token);
        token = strtok(NULL, " ");
    }

    num_tokens++;
    tokens = realloc(tokens, num_tokens * sizeof(char*));
    tokens[num_tokens - 1] = NULL;

    return tokens;
}

//returns 2 to the power of x
int pwr2(int num){
    int output = 1;
    for(int x = 0; x<num;x++){
        output *= 2;
    }
    return output;
}

typedef struct{
    uint32_t* physicalMemory;
    char* message;
} DefinedResult;

char* ctxswitch(int pid){
    char message[256]; // Assuming the message won't exceed 255 characters
    if(pid<0 || pid >3){//pid needs to be less 0,1,2,3
        snprintf(message, sizeof(message), "Current PID: %d. Invalid context switch to process %d\n",PID,pid);
        return strdup(message);
    }

    PID = pid;
    snprintf(message, sizeof(message), "Current PID: %d. Switched execution context to process: %d\n",PID,pid);
    return strdup(message);
}


int defined = 0; 
DefinedResult define(int OFF, int PFN, int VPN) {
    DefinedResult result;//struct for result

    //check if defined called
    char message[256]; // Assuming the message won't exceed 255 characters
    if(defined == 1){//only allows one define
        snprintf(message, sizeof(message), "Current PID: %d. Error: multiple calls to define in the same trace\n",PID);
        result.message = strdup(message);
        result.physicalMemory = NULL;
        return result;
    }
    int nFrames = pwr2(PFN);
    int nPages = pwr2(VPN);
    defined = 1;
    int arraySize = pwr2(OFF) + pwr2(PFN);

    uint32_t* physicalMemory = (u_int32_t*)malloc(arraySize * sizeof(u_int32_t));


    snprintf(message, sizeof(message), "Current PID: %d. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", PID, OFF, PFN, VPN);
    result.message = strdup(message);
    result.physicalMemory = physicalMemory;
    
    return result;

}

int main(int argc, char* argv[]) {
    const char usage[] = "Usage: memsym.out <strategy> <input trace> <output trace>\n";
    char* input_trace;
    char* output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 4) {
        printf("%s", usage);
        return 1;
    }
    strategy = argv[1];
    input_trace = argv[2];
    output_trace = argv[3];

    // Open input and output files
    FILE* input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");  

    while ( !feof(input_file) ) {
        // Read input file line by line
        char *rez = fgets(buffer, sizeof(buffer), input_file);
        if ( !rez ) {
            //fprintf(stderr, "Reached end of trace. Exiting...\n");
            return -1;
        } else {
            // Remove endline character
            buffer[strlen(buffer) - 1] = '\0';
        }
        char** tokens = tokenize_input(buffer);

        // TODO: Implement your memory simulator

        if (strcmp(tokens[0], "define") == 0) {
            DefinedResult result = define(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
            fprintf(output_file, "%s", result.message);
        }

        if (strcmp(tokens[0],"ctxswitch")== 0){
            char* result = ctxswitch(atoi(tokens[1]));
            fprintf(output_file,"%s",result);
            free(result);
        }

        // Deallocate tokens
        for (int i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);
        free(tokens);
    }

    // Close input and output files
    fclose(input_file);
    fclose(output_file);

    return 0;
}