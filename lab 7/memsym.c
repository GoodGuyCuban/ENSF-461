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

// Globals
int PID = 0;
u_int32_t counter = 0;
int offset_number = 0;


// Physical Memory
uint32_t *physicalMemory;
int physicalArraySize;

// Structs
struct pageTableEntry
{
    int valid;
    int PFN;
    int VPN;
};

struct pageTableEntry *pageTable[4];

typedef struct TLBEntry
{
    int valid;
    int PFN;
    int VPN;
    uint32_t timestamp;
    int process_id;
} TLBs;

struct TLBEntry TLB[8];

// 2 registers
struct registerR 
{
    int address;
    char *name;
};

struct registerR *registers;

// New section from Cris
typedef struct amRegisters{
    uint32_t register1;
    uint32_t register2;
} AmRegisters;

AmRegisters registersList[4];

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

// // Helper function to print error message for inspect functions
// void inspecterror(char* errorMessage) {
//     snprintf(errorMessage, sizeof(errorMessage), "Index used is out of range");
//     fprintf(output_file, "%s", errorMessage);
// }

int TLBLookup(uint32_t vpn){
    int tlbHit = 0; // 0 = miss, 1 = hit
    int i;
    for(i=0; i < 8; i++){
        if(TLB[i].VPN == vpn && TLB[i].process_id == PID){
            tlbHit = 1;
            break;
        }
    }
    if(tlbHit == 1){
        if(strcmp(strategy,"LRU") == 0){
            TLB[i].timestamp = counter;
        }
        fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %d hit in TLB entry %d. PFN is %d\n",
            PID, vpn, i, TLB[i].PFN);
        return TLB[i].PFN;
    }
    return -1; // Return -1 if tlb miss
}

uint32_t addressTranslation(u_int32_t virtualAddress) {
    uint32_t offsetBitsValue = (0xFFFFFFFF >> (32-offset_number)) & virtualAddress;
    uint32_t vpnBitsValue = (virtualAddress >> offset_number);
    int pfn = TLBLookup(vpnBitsValue);
    if(pfn == -1){
        fprintf(output_file,"Current PID: %d. Translating. Lookup for VPN %d caused a TLB miss\n",
            PID,vpnBitsValue);
        if(pageTable[PID][vpnBitsValue].valid == 0){
            fprintf(output_file,"Current PID: %d. Translating. Translation for VPN %d not found in page table\n",
                PID,vpnBitsValue);
                exit(-1);
        }
        pfn = pageTable[PID][vpnBitsValue].PFN;
        int i;
        for (i = 0; i < 8; i++)
        {   // If vpn is not in TLB then find a spot to put it
            if(TLB[i].valid == 0){
                break;
            }
        }
        if(i == 8){
            int firstIN = 0;
            for(int j = 0; j < 8; j++){
                if(TLB[j].timestamp < TLB[firstIN].timestamp){
                    firstIN = j;
                }
            }
            i = firstIN;
        }
        TLB[i] = (TLBs){1, pfn, vpnBitsValue, counter,PID};
        fprintf(output_file,"Current PID: %d. Translating. Successfully mapped VPN %d to PFN %d\n",
            PID,vpnBitsValue,pfn);
    }
    uint32_t physicalAddress = (pfn << offset_number) | offsetBitsValue;
    return physicalAddress;
}

void ctxswitch(int pid)
{
    char message[256]; // Assuming the message won't exceed 255 characters
    if (pid < 0 || pid > 3)
    { // pid needs to be less 0,1,2,3
        snprintf(message, sizeof(message), "Current PID: %d. Invalid context switch to process %d\n", PID, pid);
        fprintf(output_file, "%s", message);    
        return ;
    }

    PID = pid;
    snprintf(message, sizeof(message), "Current PID: %d. Switched execution context to process: %d\n", PID, pid);
    fprintf(output_file, "%s", message);

    return;
}


int defined = 0;
void define(int OFF, int PFN, int VPN)
{
    
    // check if defined called
    char message[256]; // Assuming the message won't exceed 255 characters
    if (defined == 1)
    { // only allows one define
        snprintf(message, sizeof(message), "Current PID: %d. Error: multiple calls to define in the same trace\n", PID);
        fprintf(output_file, "%s", message);
        exit(1);

        return;
    }

    int nFrames = pwr2(PFN);
    int nPages = pwr2(VPN);
    int pages = 1 << VPN;
    defined = 1;
    physicalArraySize = 1 << (OFF+PFN);

    physicalMemory = (u_int32_t *)calloc(physicalArraySize, sizeof(u_int32_t));

    offset_number = OFF;
    // Page table: per-process page tables. For simplicity, instantiate page tables
    // for 4 processes, with PID between 0 and 3
    // • Initially, all page table entries are invalid
    for (int i = 0; i < 4; i++) {
        pageTable[i] = (struct pageTableEntry *)malloc(pages * sizeof(struct pageTableEntry));
        memset(pageTable[i], 0, (pages * sizeof(struct pageTableEntry)));
    }
    memset(registersList, 0, (4 * sizeof(AmRegisters)));

    /*
    pageTable = (struct pageTableEntry *)malloc(nPages * sizeof(struct pageTableEntry));
    for (int i = 0; i < nPages; i++)
    {
        pageTable[i].valid = 0;
        pageTable[i].PFN = 0;
        pageTable[i].VPN = 0;
    }
    */

    // TLB: a fixed TLB of 8 entries
    // Note: there is only one TLB! (each TLB entry must keep track of which
    // process is associated with the entry)
    //TLB = (struct TLBEntry *)malloc(8 * sizeof(struct TLBEntry));

    for (int i = 0; i < 8; i++)
    {
        TLB[i] = (TLBs){0, 0, 0, 0, -1};
        /*TLB[i].valid = 0;
        TLB[i].PFN = 0;
        TLB[i].VPN = 0;*/
    }

    // snprintf(message, sizeof(message), "Current PID: %d. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", PID, OFF, PFN, VPN);
    fprintf(output_file, "Current PID: %d. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", PID, OFF, PFN, VPN);

    return;
}

void map(int VPN, int PFN)
{
    char message[256];
     int i;
    // Search for VPN in TLB
    for (i = 0; i < 8; i++)
    {
        if (TLB[i].VPN == VPN && TLB[i].process_id == PID)
        {
            break;
        }
    }
    if(i == 8){
    // If not found in TLB, search in page table
        for (i = 0; i < 8; i++)
        {
            if (TLB[i].valid == 0)
            {
                break;
            }
        }
        if(i==8){
            int min_timestamp_index = 0;
   
    for(i=0; i<8; i++){
        if(TLB[i].timestamp < TLB[min_timestamp_index].timestamp){
            min_timestamp_index = i;
        }
    }
    i=min_timestamp_index;
        }
    }
    
    TLB[i]= (TLBs){ 1, PFN, VPN, counter, PID };
    // If not found in page table, create new mapping
    // Note: This is a simplified version and does not handle page faults or evictions
    pageTable[PID][VPN].valid = 1;
    pageTable[PID][VPN].PFN = PFN;
    fprintf(output_file, "Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", 
        PID, VPN, PFN);

    return;
}

void unmap(int VPN)
{
    // invalidate the mapping in both the TLB and Page table
    char message[256];
    for (int i = 0; i < 8; i++)
    {
        if (TLB[i].process_id == PID && TLB[i].VPN == VPN)
        {
            TLB[i].valid = 0;
            TLB[i].PFN = 0;
            TLB[i].VPN = 0;
            // printf("Current PID: %d. Unmapped virtual page number %d\n", PID, VPN);
            pageTable[PID][VPN].valid = 0;
            pageTable[PID][VPN].PFN = 0;
            snprintf(message, sizeof(message), "Current PID: %d. Unmapped virtual page number %d\n", PID, VPN);
            fprintf(output_file, "%s", message);
            return ;
        }
    }

}



void load(char *dst, char *src)
{
    if(strcmp(dst,"r1") != 0 && strcmp(dst,"r2") != 0){
        fprintf(output_file, "Current PID: %d. Error: invalid register operand %s\n", PID, dst);
        exit(-1);    
    }
    char message[256];
    // src can be a register or an immediate value
    uint32_t srcValue;
    int srcType; // 0 for register, 1 for immediate value
    // This is the case for when src is an immediate
    if (*src == '#')
    {
        int i;
        for(i = 1; src[i] != '\0'; i++){
            src[i-1] = src[i];
        }
        src[i-1] = src[i];
        if (*src == '\0') {
            fprintf(output_file, "Current PID: %d. ERROR: Invalid immediate\n", PID);
            exit(-1);
        }
        //srcValue = atoi(src);
        //srcType = 1;
        uint32_t immediate = atoi(src);
        
        if(strcmp(dst, "r1") == 0) {
            registersList[PID].register1 = immediate;
        } else {
            registersList[PID].register2 = immediate;
        }
        fprintf(output_file, "Current PID: %d. Loaded immediate %u into register %s\n", PID, immediate, dst);
    } // Else case is if src is a memory location
    else {
        uint32_t value;
        uint32_t virtualAddress = strtoul(src, NULL, 10);
        uint32_t physicalAddress = addressTranslation(virtualAddress);
        if(strcmp(dst, "r1") == 0){
            value = physicalMemory[physicalAddress];
            registersList[PID].register1 = value;
        }
        else{
            value = physicalMemory[physicalAddress];
            registersList[PID].register2 = value;
        }
        fprintf(output_file, "Current PID: %d. Loaded value of location %s (%u) into register %s\n",
                PID, src, value, dst);
    }

}

void store(char *dst, char *src)
{
    char message[256];
    // src can be a register or an immediate value
    uint32_t srcValue;
    /*
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
    */
   if (src[0] == '#') {
        src++;
        if (*src == '\0') {
            fprintf(output_file, "Current PID: %d. ERROR: Invalid immediate\n", PID);
            exit(-1);
        }
        srcValue = strtoul(src, NULL, 10);

        uint32_t destVirtualAddress = strtoul(dst, NULL, 10);
        uint32_t destPhysicalAddress = addressTranslation(destVirtualAddress);
        physicalMemory[destPhysicalAddress] = srcValue;
        fprintf(output_file, "Current PID: %d. Stored immediate %d into location %s\n", PID, srcValue, dst);
    }
    // dst can only be r1 or r2
    else if (strcmp(src, "r1") == 0)
    {
        // registers[0].address = srcValue;
        // return format If <src> is a memory location, it should output:
        // Stored value of location <src> (<value>) into register <dst>
        // • If <src> is an immediate, it should output:
        // Stored immediate <src> into register <dst>
        srcValue = registersList[PID].register1;
        uint32_t dstVirtualAddress = strtoul(dst, NULL, 10);
        uint32_t dstPhysicalAddress = addressTranslation(dstVirtualAddress);
        physicalMemory[dstPhysicalAddress] = srcValue;
        fprintf(output_file, "Current PID: %d. Stored value of register r1 (%u) into location %s\n",
            PID, srcValue, dst);
    }
    else if (strcmp(src, "r2") == 0)
    {
        // registers[1].address = srcValue;
        // return format If <src> is a memory location, it should output:
        // Stored value of location <src> (<value>) into register <dst>
        // • If <src> is an immediate, it should output:
        // Stored immediate <src> into register <dst>
        srcValue = registersList[PID].register2;
        uint32_t dstVirtualAddress = strtoul(dst, NULL, 10);
        uint32_t dstPhysicalAddress = addressTranslation(dstVirtualAddress);
        physicalMemory[dstPhysicalAddress] = srcValue;
        fprintf(output_file, "Current PID: %d. Stored value of register r2 (%u) into location %s\n",
            PID, srcValue, dst);
    }
    else
    {
        // Error message
        fprintf(output_file, "Current PID: %d. Error: invalid register operand <reg>\n", PID);
        exit(1);
    }
    return;
}

void pinspect(int VPN)
{
    char message[256];
    // search for vpn in page table
    // Inspected page table entry <VPN>. Physical frame number: <PFN>. Valid: <VALID>
    snprintf(message, sizeof(message), "Inspected page table entry %d. Physical frame number: %d. Valid: %d\n", VPN, pageTable[PID][VPN].PFN, pageTable[PID][VPN].valid);
    fprintf(output_file, "Current PID: %d. %s", PID, message);
    

}

void tinspect(int TLBN) {
    char message[256];
    int array_size = sizeof(TLB)/sizeof(TLB[0]);

    // Simple error case if TLBN is greater than the size of the TLB
    // if (TLBN > array_size) {
    //     inspecterror(message);
    // }
    
    // Otherwise, iterate through the TLB to find the desired entry
    for (int i = 0; i < array_size; i++) {
        if (i == TLBN) {
            snprintf(message, sizeof(message), "Inspected TLB entry %d. VPN: %d. PFN: %d. Valid: %d. PID: %d. Timestamp: %d\n", TLBN, TLB[i].VPN, TLB[i].PFN, TLB[i].valid, TLB[i].process_id, TLB[i].timestamp);
            fprintf(output_file, "Current PID: %d. %s", PID, message);
        }
    }

}

void linspect(int PL) {
    char message[256];

    // Checks obvious error case if PL is greater than the size of Physical Memory
    // if (PL > physicalArraySize) {
    //     inspecterror(message);
    // }

    // snprintf(message, sizeof(message), " Inspected physical location %d. Value: %d\n", PL, physicalMemory[PL]);
    fprintf(output_file, "Current PID: %d. Inspected physical location %d. Value: %d\n",PID, PL, physicalMemory[PL] );
}

void rinspect(char* amRegister) {
    char output[256];
    int value; 


    // Since there are only two registers, can just check both cases individually
    if (strcmp(amRegister, "r1") == 0) {
        value = registersList[PID].register1;
    }
    else if (strcmp(amRegister, "r2") == 0) {
        value = registersList[PID].register2;
    }
    else {
        fprintf(output_file, "Current PID: %d. Error: invalid register operand <reg>\n", PID);
        exit(1);
    }
    // snprintf(output, sizeof(output), "Current PID: %d. Inspected register %s. Content: %d",PID, amRegister, value);
    fprintf(output_file,"Current PID: %d. Inspected register %s. Content: %d\n",PID, amRegister, value);

    return;
}

void add(){
    uint32_t r1 = registersList[PID].register1;
    uint32_t r2 = registersList[PID].register2;
    registersList[PID].register1 = r1 + r2;
    fprintf(output_file, "Current PID: %d. Added contents of registers r1 (%u) and r2 (%u). Result: %u\n", PID, r1, r2, r1+r2);

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
            define(atoi(tokens[1]), atoi(tokens[2]), atoi(tokens[3]));
        }
        else if (strcmp(tokens[0], "%") != 0 && strcmp(tokens[0], "") != 0 && !defined) {
            fprintf(output_file, "Current PID: %d. Error: attempt to execute instruction before define\n", PID);
            exit(-1);
        }

        else if (strcmp(tokens[0], "ctxswitch") == 0)
        {
            ctxswitch(atoi(tokens[1]));
        }

        else if (strcmp(tokens[0], "map") == 0)
        {
            map(atoi(tokens[1]), atoi(tokens[2]));
        }

        else if (strcmp(tokens[0], "unmap") == 0)
        {
            unmap(atoi(tokens[1]));
        }

        else if (strcmp(tokens[0], "rinspect") == 0) {
            rinspect(tokens[1]);
        }

        else if (strcmp(tokens[0], "linspect") == 0) {
            linspect(atoi(tokens[1]));
        }

        else if (strcmp(tokens[0], "pinspect") == 0) {
            pinspect(atoi(tokens[1]));
        }

        else if (strcmp(tokens[0], "tinspect") == 0) {
            tinspect(atoi(tokens[1]));
        }

        else if (strcmp(tokens[0], "load") == 0) {
            load(tokens[1], tokens[2]);
        }

        else if (strcmp(tokens[0], "store") == 0) {
            store(tokens[1], tokens[2]);
        }
        
        else if (strcmp(tokens[0], "add") == 0) {
            add();
        }
        
        // Deallocate tokens
        for (int i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);
        free(tokens);
        counter++;
    }

    // Close input and output files
    fclose(input_file);
    fclose(output_file);

    return 0;
}