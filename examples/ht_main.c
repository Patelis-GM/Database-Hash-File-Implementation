#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/bf.h"
#include "ht_table.h"

#define RECORDS_NUM 10 // you can change it if you want
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

    HT_CreateFile(FILE_NAME, 3);
    HT_info *info = HT_OpenFile(FILE_NAME);

    printf("Before insertions file has %d\n",info->totalRecords);

    Record record;
    srand(12569874);
    int r;
    printf("Insert Entries\n");
    for (int id = 0; id < RECORDS_NUM; ++id) {
        record = randomRecord();
        printf("To insert Record  :\n");
        printRecord(record);
        HT_InsertEntry(info, record);
    }

    printf("RUN PrintAllEntries\n");
    int id = rand() % RECORDS_NUM;
    int blocksRequested = HT_GetAllEntries(info, id);
    printf("Blocks Requested : %d", blocksRequested);

    HT_CloseFile(info);
    BF_Close();
}
