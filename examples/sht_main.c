#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 30
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"


int main() {

    srand(time(NULL));
    BF_Init(LRU);

    HT_CreateFile(FILE_NAME, 10);
    SHT_CreateSecondaryIndex(INDEX_NAME, 10, FILE_NAME);

    HT_info *info = HT_OpenFile(FILE_NAME);
    SHT_info *index_info = SHT_OpenSecondaryIndex(INDEX_NAME);

    Record record = randomRecord();
    char searchName[15];
    strcpy(searchName, record.name);


    for (int id = 0; id < RECORDS_NUM; ++id) {

        InsertPosition insertPosition = HT_InsertEntry(info, record);
        if (insertPosition.recordIndex == HT_ERROR || insertPosition.blockIndex == HT_ERROR) {
            BF_Close();
            exit(1);
        }

        printf("##########\n");
        printf("Inserted Record with Bucket Index %d\n", record.id % info->totalBuckets);
        printRecord(record);
        printf("in Block %d\n", insertPosition.blockIndex);


        int secondaryBlockIndex = SHT_SecondaryInsertEntry(index_info, record, insertPosition);
        if (secondaryBlockIndex == SHT_ERROR) {
            BF_Close();
            exit(1);
        }

        printf("----------\n");
        printf("Inserted Secondary Record with Bucket Index %d\n", hash(record.name) % index_info->totalSecondaryBuckets);
        printSecondaryRecord(secondaryRecordFromRecord(record));
        printf("in Block %d\n", secondaryBlockIndex);


        printf("##########\n\n");
    }

    printf("Looking for name %s\n", searchName);
    int blocksRequested = SHT_SecondaryGetAllEntries(info, index_info, searchName);
    if (blocksRequested == SHT_ERROR) {
        BF_Close();
        exit(1);
    }

    printf("%d Block(s) requested for name %s\n", blocksRequested, searchName);


    int status;

    status = SHT_CloseSecondaryIndex(index_info);
    if (status == SHT_ERROR) {
        BF_Close();
        exit(1);
    }

    status = HT_CloseFile(info);
    if (status == HT_ERROR) {
        BF_Close();
        exit(1);
    }


    BF_Close();
}
