#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

// RETURNS AN INTEGER WHICH IS THE HASHVALUE OF PROVIDED WORD
int hashFunction(char *word, unsigned int cache){
    int wordSum = 0;
    int hashValue;
    for(int i = 0; i < 128; i++){
        if(*(word + i) == '\0'){
            break;
        }
        wordSum = wordSum + *(word + i);
    }
    hashValue = wordSum % cache;
    return hashValue;
}

// PRINTS THE STRING LOCATED IN EACH CACHE OF MEMORY
void printCache(char **hashTable, unsigned int cache){
    int counter = 0;
    while(counter < cache){
        if(*(hashTable + counter)){
            printf("Cache index %d ==> \"%s\"\n", counter, *(hashTable + counter) );
        }
        counter ++;
    }
}

// MAAIN PROGRAM
int main(int argc, char **argv){
    // CHECK AND MAKE SURE THE CORRECT NUMBER OF ARGUMENTS ARE PROVIDED.
    if(argc != 3){
        fprintf(stderr, "ERROR: Wrong number of arguments. Only two arguments allowed.\n");
        exit(-1);
    }


    // HASHTABLE SIZE
    unsigned int cache = atoi(*(argv + 1));
    
    // MAKE SURE CACHE VALUE IS VALID IF NOT ERROR AND EXIT
    if(cache == 0){
        fprintf(stderr, "ERROR: FLOATING POINT ERROR\nPROGRAM TERMINATED");
        exit(-1);
    }
    
    // OPEN FILE, EXIT IF THE FILE DOES NOT EXIST.
    FILE *fp;
    fp = fopen(*(argv + 2), "r");
    if (fp == NULL){
        fprintf(stderr, "ERROR: File does not exist.");
        exit(-1);
    }

    // CREATE HASHTABLE ARRAY OF POINTERS TO CHAR ARRAYS
    char **hashTable = (char**)calloc(cache, sizeof(char*)); 
    
    // DECALRE A VARIABLE TO STORE EACH WORD
    char *word = calloc(128, sizeof(char)); 


    // MAIN LOGIC
    // READ FILE CHAR BY CHAR CREATING WORDS GREATER THAN SIZE 3, ADD THE WORD TO CORRECT CACHE LOCATION
    int i = 0;
    char c;
    while ((c = fgetc(fp)) != EOF){
        if(isalnum(c)){
            *(word + i) = c;
            i++;
            continue;
        }
        else{
            *(word + i) = '\0';
            if(strlen(word) < 3){
                i = 0;
                continue;
            }
            *(word + i) = '\0';
            int hash = hashFunction(word, cache);
            // FIRST ALLOCATION REALLOC NOT NECESSARY
            if (!(*(hashTable + hash))){
                *(hashTable + hash) = calloc(strlen(word) + 1, sizeof(char));
                memcpy(*(hashTable + hash), word, strlen(word) + 1);
                printf("Word \"%s\" ==> %d (calloc)\n", *(hashTable+hash), hash);
            }
            // REALLOC NECESSARY DIFFERENT SIZE WORD HAS BEEN STORED IN THAT CACHE
            else{
                *(hashTable + hash) = (char *)realloc(*(hashTable + hash), (strlen(word) + 1) * sizeof(char));
                memcpy(*(hashTable + hash), word, strlen(word) + 1);
                printf("Word \"%s\" ==> %d (realloc)\n", *(hashTable+hash), hash);
            }
            i = 0;
        }
    }    
    printCache(hashTable, cache);
    
    // DEALLOCATE MEMORY
    for(int i = 0; i < cache; i++){
        char *ptr = *(hashTable + i);
        free(ptr);
    }
    free(hashTable);
    free(word);
    fclose(fp);
   
    return 0;
}