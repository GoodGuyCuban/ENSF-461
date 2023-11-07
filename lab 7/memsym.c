#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h> //for unit_32
#define TRUE 1
#define FALSE 0

// Output file
FILE *output_file;

// TLB replacement strategy (FIFO or LRU)
char *strategy;

// PID
int PID = 0;

char **tokenize_input(char *input)
{
    char **tokens = NULL;
    char *token = strtok(input, " ");
    int num_tokens = 0;

    while (token != NULL)
    {
        num_tokens++;
        tokens = realloc(tokens, num_tokens * sizeof(char *));
        tokens[num_tokens - 1] = malloc(strlen(token) + 1);
        strcpy(tokens[num_tokens - 1], token);
        token = strtok(NULL, " ");
    }

    num_tokens++;
    tokens = realloc(tokens, num_tokens * sizeof(char *));
    tokens[num_tokens - 1] = NULL;

    return tokens;
}

// returns 2 to the power of x
int pwr2(int num)
{
    int output = 1;
    for (int x = 0; x < num; x++)
    {
        output *= 2;
    }
    return output;
}

typedef struct
{
    uint32_t *physicalMemory;
    char *message;
} DefinedResult;

char *ctxswitch(int pid)
{
    char message[256]; // Assuming the message won't exceed 255 characters
    if (pid < 0 || pid > 3)
    { // pid needs to be less 0,1,2,3
        snprintf(message, sizeof(message), "Current PID: %d. Invalid context switch to process %d\n", PID, pid);
        return strdup(message);
    }

    PID = pid;
    snprintf(message, sizeof(message), "Current PID: %d. Switched execution context to process: %d\n", PID, pid);
    return strdup(message);
}

struct pageTableEntry
{
    int valid;
    int PFN;
    int VPN;
};

struct pageTableEntry *pageTable;

struct TLBEntry
{
    int valid;
    int PFN;
    int VPN;
    //time
};

struct TLBEntry *TLB;

int defined = 0;
DefinedResult define(int OFF, int PFN, int VPN)
{
    DefinedResult result; // struct for result

    // check if defined called
    char message[256]; // Assuming the message won't exceed 255 characters
    if (defined == 1)
    { // only allows one define
        snprintf(message, sizeof(message), "Current PID: %d. Error: multiple calls to define in the same trace\n", PID);
        result.message = strdup(message);
        result.physicalMemory = NULL;
        return result;
    }
    int nFrames = pwr2(PFN);
    int nPages = pwr2(VPN);
    defined = 1;
    int arraySize = pwr2(OFF) + pwr2(PFN);

    uint32_t *physicalMemory = (u_int32_t *)malloc(arraySize * sizeof(u_int32_t));

    snprintf(message, sizeof(message), "Current PID: %d. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", PID, OFF, PFN, VPN);
    result.message = strdup(message);
    result.physicalMemory = physicalMemory;

    // Page table: per-process page tables. For simplicity, instantiate page tables
    // for 4 processes, with PID between 0 and 3
    // • Initially, all page table entries are invalid

    pageTable = (struct pageTableEntry *)malloc(nPages * sizeof(struct pageTableEntry));
    for (int i = 0; i < nPages; i++)
    {
        pageTable[i].valid = 0;
        pageTable[i].PFN = 0;
        pageTable[i].VPN = 0;
    }

    // TLB: a fixed TLB of 8 entries
    // Note: there is only one TLB! (each TLB entry must keep track of which
    // process is associated with the entry)
    TLB = (struct TLBEntry *)malloc(8 * sizeof(struct TLBEntry));

    for (int i = 0; i < 8; i++)
    {
        TLB[i].valid = 0;
        TLB[i].PFN = 0;
        TLB[i].VPN = 0;
    }

    return result;
}

char *map(int VPN, int PFN)
{
    char message[256];
    // Search for VPN in TLB
    for (int i = 0; i < 8; i++)
    {
        if (TLB[i].valid && TLB[i].VPN == VPN)
        {
            // Update TLB entry
            TLB[i].PFN = PFN;
            // printf("Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", PID, VPN, PFN);
            snprintf(message, sizeof(message), "Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", PID, VPN, PFN);
            return strdup(message);
        }
    }

    // If not found in TLB, search in page table
    for (int i = 0; i < pwr2(VPN); i++)
    {
        if (pageTable[i].valid && pageTable[i].VPN == VPN)
        {
            // Update page table entry
            pageTable[i].PFN = PFN;
            // printf("Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", PID, VPN, PFN);
            snprintf(message, sizeof(message), "Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", PID, VPN, PFN);
            return strdup(message);
        }
    }

    // If not found in page table, create new mapping
    // Note: This is a simplified version and does not handle page faults or evictions
    pageTable[VPN].valid = 1;
    pageTable[VPN].PFN = PFN;

    // Update TLB
    for (int i = 0; i < 8; i++)
    {
        if (!TLB[i].valid)
        {
            TLB[i].valid = 1;
            TLB[i].PFN = PFN;
            TLB[i].VPN = VPN;
            // printf("Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", PID, VPN, PFN);
            snprintf(message, sizeof(message), "Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", PID, VPN, PFN);
            return strdup(message);
        }
    }
}

char *unmap(int VPN)
{
    // invalidate the mapping in both the TLB and Page table
    char message[256];
    for (int i = 0; i < 8; i++)
    {
        if (TLB[i].valid && TLB[i].VPN == VPN)
        {
            TLB[i].valid = 0;
            TLB[i].PFN = 0;
            TLB[i].VPN = 0;
            // printf("Current PID: %d. Unmapped virtual page number %d\n", PID, VPN);
            snprintf(message, sizeof(message), "Current PID: %d. Unmapped virtual page number %d\n", PID, VPN);
            return strdup(message);
        }
    }
}

// 2 registers
struct registerR 
{
    int address;
    char *name;
};

struct registerR *registers;

char *load(char *dst, char *src)
{
    char message[256];
    // src can be a register or an immediate value
    int srcValue;
    int srcType; // 0 for register, 1 for immediate value
    if (strcmp(src, "r1") == 0)
    {
        srcValue = registers[0].address;
        srcType = 0;
    }
    else if (strcmp(src, "r2") == 0)
    {
        srcValue = registers[1].address;
        srcType = 0;
    }
    else
    {
        srcValue = atoi(src);
        srcType = 1;
    }

    // dst can only be r1 or r2
    if (strcmp(dst, "r1") == 0)
    {
        registers[0].address = srcValue;
        // return format If <src> is a memory location, it should output:
        // Loaded value of location <src> (<value>) into register <dst>
        // • If <src> is an immediate, it should output:
        // Loaded immediate <src> into register <dst>
        if (srcType == 0)
        {
            snprintf(message, sizeof(message), "Loaded value of location %d (%d) into register %s\n", srcValue, registers[0].name, dst);
            return strdup(message);
        }
        else
        {
            snprintf(message, sizeof(message), "Loaded immediate %d into register %s\n", srcValue, dst);
            return strdup(message);
        }
    }
    else if (strcmp(dst, "r2") == 0)
    {
        registers[1].address = srcValue;
        // return format If <src> is a memory location, it should output:
        // Loaded value of location <src> (<value>) into register <dst>
        // • If <src> is an immediate, it should output:
        // Loaded immediate <src> into register <dst>
        if (srcType == 0)
        {
            snprintf(message, sizeof(message), "Loaded value of location %d (%d) into register %s\n", srcValue, registers[1].name, dst);
            return strdup(message);
        }
        else
        {
            snprintf(message, sizeof(message), "Loaded immediate %d into register %s\n", srcValue, dst);
            return strdup(message);
        }
    }
    else
    {
        return "Error: invalid register operand <reg>\n";
    }

    return NULL;
}

char *store(char *dst, char *src)
{
    char message[256];
    // src can be a register or an immediate value
    int srcValue;
    int srcType; // 0 for register, 1 for immediate value
    if (strcmp(src, "r1") == 0)
    {
        srcValue = registers[0].address;
        srcType = 0;
    }
    else if (strcmp(src, "r2") == 0)
    {
        srcValue = registers[1].address;
        srcType = 0;
    }
    else
    {
        srcValue = atoi(src);
        srcType = 1;
    }

    // dst can only be r1 or r2
    if (strcmp(dst, "r1") == 0)
    {
        registers[0].address = srcValue;
        // return format If <src> is a memory location, it should output:
        // Stored value of location <src> (<value>) into register <dst>
        // • If <src> is an immediate, it should output:
        // Stored immediate <src> into register <dst>
        if (srcType == 0)
        {
            snprintf(message, sizeof(message), "Stored value of register %s (%d) into location %d\n", src, registers[0].address, srcValue);
            return strdup(message);
        }
        else
        {
            snprintf(message, sizeof(message), "Stored immediate %d into location %d\n", srcValue, srcValue);
            return strdup(message);
        }
    }
    else if (strcmp(dst, "r2") == 0)
    {
        registers[1].address = srcValue;
        // return format If <src> is a memory location, it should output:
        // Stored value of location <src> (<value>) into register <dst>
        // • If <src> is an immediate, it should output:
        // Stored immediate <src> into register <dst>
        if (srcType == 0)
        {
            snprintf(message, sizeof(message), "Stored value of register %s (%d) into location %d\n", src, registers[1].address, srcValue);
            return strdup(message);
        }
        else
        {
            snprintf(message, sizeof(message), "Stored immediate %d into location %d\n", srcValue, srcValue);
            return strdup(message);
        }
    }
    else
    {
        return "Error: invalid register operand <reg>\n";
    }

    return NULL;
}

char *pinspect(int VPN)
{
    char message[256];
    // search for vpn in page table
    for (int i = 0; i < pwr2(VPN); i++)
    {
        if (pageTable[i].valid && pageTable[i].VPN == VPN)
        {
            // Inspected page table entry <VPN>. Physical frame number: <PFN>. Valid: <VALID>
            snprintf(message, sizeof(message), "Inspected page table entry %d. Physical frame number: %d. Valid: %d\n", VPN, pageTable[i].PFN, pageTable[i].valid);
            return strdup(message);
        }
    }
}

int main(int argc, char *argv[])
{
    const char usage[] = "Usage: memsym.out <strategy> <input trace> <output trace>\n";
    char *input_trace;
    char *output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 4)
    {
        printf("%s", usage);
        return 1;
    }
    strategy = argv[1];
    input_trace = argv[2];
    output_trace = argv[3];

    // Open input and output files
    FILE *input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");

    while (!feof(input_file))
    {
        // Read input file line by line
        char *rez = fgets(buffer, sizeof(buffer), input_file);
        if (!rez)
        {
            // fprintf(stderr, "Reached end of trace. Exiting...\n");
            return -1;
        }
        else
        {
            // Remove endline character
            buffer[strlen(buffer) - 1] = '\0';
        }
        char **tokens = tokenize_input(buffer);

        // TODO: Implement your memory simulator

        if (strcmp(tokens[0], "define") == 0)
        {
            DefinedResult result = define(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
            fprintf(output_file, "%s", result.message);
        }

        if (strcmp(tokens[0], "ctxswitch") == 0)
        {
            char *result = ctxswitch(atoi(tokens[1]));
            fprintf(output_file, "%s", result);
            free(result);
        }

        if (strcmp(tokens[0], "map") == 0)
        {
            char *result = map(atoi(tokens[1]), atoi(tokens[2]));
            fprintf(output_file, "%s", result);
            free(result);
        }

        if (strcmp(tokens[0], "unmap") == 0)
        {
            char *result = unmap(atoi(tokens[1]));
            fprintf(output_file, "%s", result);
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
