#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/bf.h"
#include "ht_table.h"

#define RECORDS_NUM 10000 // you can change it if you want
#define FILE_NAME "data.db"

#define VALUE_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
    BF_Init(LRU);

//    printf("Max Records per block: %d\n", MAX_RECORDS);

    int buckets = 10;
    HT_CreateFile(FILE_NAME, buckets);
    HT_info *info = HT_OpenFile(FILE_NAME);

//    printf("Before insertions file is : \n");
//    printf("Total records : %d\n", info->totalRecords);
//    printf("Total blocks : %d\n", info->totalBlocks);
//    printf("Total buckets : %d\n", info->totalBuckets);
//    for (int i = 0; i < buckets; ++i)
//        printf("Hash value %d goes to block %d\n", i, info->hashToBlock[i]);
//
//    printf("------\n");

    Record record;
    srand(12569874);
    int r;
//    printf("Insert Entries\n");
    for (int id = 0; id < RECORDS_NUM; ++id) {
        record = randomRecord();
//        printf("To insert Record  :\n");
//        printRecord(record);
        HT_InsertEntry(info, record);
//        printf("------\n");
    }

//    printf("RUN PrintAllEntries\n");
    int id = rand() % RECORDS_NUM;
    int blocksRequested = HT_GetAllEntries(info, id);
    printf("Blocks Requested : %d\n", blocksRequested);


//    printf("===============================\n");

    HT_CloseFile(info);
    BF_Close();
}
