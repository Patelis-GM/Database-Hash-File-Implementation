#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 30000000
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {

    BF_Init(LRU);

//    printf("Max Records per block: %lu\n", MAX_RECORDS);

    int buckets = 1;
    HT_CreateFile(FILE_NAME, buckets);

    HT_info *info = HT_OpenFile(FILE_NAME);
    if (info == NULL) {
        BF_Close();
        return 1;
    }


    Record record;
    srand(time(NULL));


    for (int id = 0; id < RECORDS_NUM; ++id) {
        record = randomRecord();
        record.id = rand() % 50;
        int bucketIndex = record.id % info->totalBuckets;
        printf("To insert Record in Bucket %d  :\n", bucketIndex);
        printRecord(record);
        int blockIndex = HT_InsertEntry(info, record);
        if(blockIndex == -1)
            break;
        printf("Inserted it in Block %d\n", blockIndex);
        printf("------\n");
    }

//    printf("RUN PrintAllEntries\n");
//    int id = rand() % 30;
//    int blocksRequested = HT_GetAllEntries(info, id);
//    printf("Blocks Requested : %d\n", blocksRequested);
//
//
//


    int blocksRequested = HT_GetAllEntries(info, 2);
    printf("Blocks Requested : %d\n", blocksRequested);
    printf("After insertions file is : \n");
    printf("Total records : %d\n", info->totalRecords);
    printf("Total blocks : %d\n", info->totalBlocks);
    printf("Total buckets : %d\n", info->totalBuckets);
    for (int i = 0; i < buckets; ++i)
        printf("Bucket %d goes to block %d\n", i, info->bucketToBlock[i]);

    printf("------\n");

    printf("===============================\n");

    HT_PrintAllEntries(info);

    printf("===============================\n");


    HT_CloseFile(info);

    HashStatistics(FILE_NAME);

    BF_Close();
}
