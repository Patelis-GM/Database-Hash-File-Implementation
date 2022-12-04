#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 6500000
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

    int buckets = 1;
    HT_CreateFile(FILE_NAME, buckets);

    HT_info *info = HT_OpenFile(FILE_NAME);
    if (info == NULL) {
        BF_Close();
        return 1;
    }



    Record record;
    srand(time(NULL));
//    printf("Insert Entries\n");
    for (int id = 0; id < RECORDS_NUM; ++id) {
        record = randomRecord();
        record.id = rand() % 20;
//        int bucketIndex = record.id % info->totalBuckets;
//        printf("To insert Record in bucket %d  :\n",bucketIndex);
//        printRecord(record);
        HT_InsertEntry(info, record);
//        printf("Inserted it in block %d\n",blockIndex);
//        printf("------\n");
    }

//    printf("RUN PrintAllEntries\n");
//    int id = rand() % 30;
//    int blocksRequested = HT_GetAllEntries(info, id);
//    printf("Blocks Requested : %d\n", blocksRequested);
//
//
//
//    printf("===============================\n");
//
//    HT_PrintAllEntries(info);
//
//    printf("===============================\n");


    printf("After insertions file is : \n");
    printf("Total records : %ld\n", info->totalRecords);
    printf("Total blocks : %ld\n", info->totalBlocks);
    printf("Total buckets : %ld\n", info->totalBuckets);
    for (int i = 0; i < buckets; ++i)
        printf("Bucket %ld goes to block %d\n", i, info->bucketToBlock[i]);

    printf("------\n");

    HT_CloseFile(info);
    BF_Close();
}
