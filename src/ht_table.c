#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bf.h"
#include "ht_table.h"
#include "record.h"


#define VALUE_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }


#define POINTER_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return NULL;             \
    }                         \
  }


int HT_CreateFile(char *fileName, int buckets) {

    /* Buckets should be in the following range : (0, MAX_BUCKETS] */
    if (buckets <= 0 || buckets > MAX_BUCKETS) {
        printf("Buckets should be in the following range : (0, %d]\n", MAX_BUCKETS);
        return HT_ERROR;
    }

    /* Create file */
    VALUE_CALL_OR_DIE(BF_CreateFile(fileName))

    int fileDescriptor;

    /* Open file */
    VALUE_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))

    BF_Block *block;
    BF_Block_Init(&block);
    char *blockData;

    char hashFileIdentifier = HASH_FILE_IDENTIFIER;
    HT_info headerMetadata;

    headerMetadata.totalBuckets = buckets;
    headerMetadata.totalRecords = 0;
    headerMetadata.totalBlocks = 1;
    headerMetadata.fileDescriptor = fileDescriptor;
    for (int j = 0; j < MAX_BUCKETS; ++j)
        headerMetadata.hashToBlock[j] = NONE;

    VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
    blockData = BF_Block_GetData(block);
    memcpy(blockData, &hashFileIdentifier, sizeof(char));
    memcpy(blockData + sizeof(char), &headerMetadata, sizeof(HT_info));
    BF_Block_SetDirty(block);
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))


    BF_Block_Destroy(&block);
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

    return HT_OK;
}

HT_info *HT_OpenFile(char *fileName) {


    int fileDescriptor;
    POINTER_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))

    BF_Block *block;
    BF_Block_Init(&block);

    POINTER_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))

    char *blockData = BF_Block_GetData(block);

    char hashFileIdentifier;
    memcpy(&hashFileIdentifier, blockData, sizeof(char));

    if (hashFileIdentifier != HASH_FILE_IDENTIFIER) {

        printf("First byte of HEADER-BLOCK should be : %c\n", HASH_FILE_IDENTIFIER);

        POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

        POINTER_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

        BF_Block_Destroy(&block);
        return NULL;
    }

    HT_info *headerMetadata = (HT_info *) malloc(sizeof(HT_info));
    memcpy(headerMetadata, blockData + sizeof(char), sizeof(HT_info));

    POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

    BF_Block_Destroy(&block);
    return headerMetadata;
}


bool areDifferent(HT_info *htInfo, HT_info *anotherHtInfo) {

    if (htInfo->totalBlocks != anotherHtInfo->totalBlocks || htInfo->totalRecords != anotherHtInfo->totalRecords || htInfo->totalBuckets != anotherHtInfo->totalBuckets)
        return true;

    for (int i = 0; i < MAX_BUCKETS; ++i)
        if (htInfo->hashToBlock[i] != anotherHtInfo->hashToBlock[i])
            return true;

    return false;
}


int HT_CloseFile(HT_info *ht_info) {

    int fileDescriptor = ht_info->fileDescriptor;

    BF_Block *block;
    BF_Block_Init(&block);

    VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))

    char *blockData = BF_Block_GetData(block);

    HT_info headerMetadata;
    memcpy(&headerMetadata, blockData + sizeof(char), sizeof(HT_info));

    if (areDifferent(ht_info, &headerMetadata)) {
        memcpy(blockData + sizeof(char), ht_info, sizeof(HT_info));
        BF_Block_SetDirty(block);
    }

    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

    free(ht_info);
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))
    BF_Block_Destroy(&block);

    return HT_OK;
}

int HT_InsertEntry(HT_info *ht_info, Record record) {


    int hashValue = record.id % ht_info->totalBuckets;
    int blockIndex = ht_info->hashToBlock[hashValue];
    int fileDescriptor = ht_info->fileDescriptor;

    printf("Hash Value is %d and will ATTEMPT to insert in block %d\n", hashValue, blockIndex);

    BF_Block *block;
    char *blockData;
    HT_block_info bucketMetadata;
    BF_Block_Init(&block);


    if (blockIndex == NONE) {

        bucketMetadata.totalRecords = 1;
        bucketMetadata.nextBlock = NONE;
        bucketMetadata.previousBlock = NONE;
        VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
        blockData = BF_Block_GetData(block);
        memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));
        memcpy(blockData + sizeof(HT_block_info), &record, sizeof(Record));
        BF_Block_SetDirty(block);
        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
        ht_info->hashToBlock[hashValue] = ht_info->totalBlocks;
        ht_info->totalBlocks += 1;
        printf("Block didn't EXIST had to create it and inserted there which now is block %d!\n", ht_info->hashToBlock[hashValue]);
    }

    else {

        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, blockIndex, block))
        blockData = BF_Block_GetData(block);
        memcpy(&bucketMetadata, blockData, sizeof(HT_block_info));

        if (bucketMetadata.totalRecords < MAX_RECORDS) {
            memcpy(blockData + sizeof(HT_block_info) + (bucketMetadata.totalRecords * sizeof(Record)), &record, sizeof(Record));
            bucketMetadata.totalRecords += 1;
            memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));
            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
            printf("Block %d had space and inserted there!\n", blockIndex);
        }

        else {

            int previousBlock = blockIndex;
            int nextBlock = ht_info->totalBlocks;

            bucketMetadata.nextBlock = nextBlock;
            memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));
            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

            bucketMetadata.totalRecords = 1;
            bucketMetadata.nextBlock = NONE;
            bucketMetadata.previousBlock = previousBlock;

            VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
            blockData = BF_Block_GetData(block);
            memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));
            memcpy(blockData + sizeof(HT_block_info), &record, sizeof(Record));
            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
            ht_info->totalBlocks += 1;
            ht_info->hashToBlock[hashValue] = nextBlock;
            printf("Block %d DIDN'T HAVE space and inserted in block %d after extension!\n", blockIndex, nextBlock);
        }


    }


    ht_info->totalRecords += 1;
    BF_Block_Destroy(&block);
    return blockIndex;
}

int HT_GetAllEntries(HT_info *ht_info, int value) {

    printf("Looking for value %d\n", value);

    int hashValue = value % ht_info->totalBuckets;
    int blockIndex = ht_info->hashToBlock[hashValue];
    int fileDescriptor = ht_info->fileDescriptor;
    int blocksRequested = 0;

    bool keepLooking = true;
    bool foundValue = false;

    BF_Block *block;
    char *blockData;
    HT_block_info bucketMetadata;
    Record record;
    BF_Block_Init(&block);

    if (blockIndex == NONE)
        keepLooking = false;

    while (keepLooking) {

        printf("Looking in block %d\n", blockIndex);

        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, blockIndex, block))
        blockData = BF_Block_GetData(block);
        memcpy(&bucketMetadata, blockData, sizeof(HT_block_info));
        blocksRequested += 1;

        for (int i = 0; i < bucketMetadata.totalRecords; ++i) {
            memcpy(&record, blockData + sizeof(HT_block_info) + (i * sizeof(Record)), sizeof(Record));
            if (record.id == value) {
                printRecord(record);
                foundValue = true;
            }

        }

        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

        blockIndex = bucketMetadata.previousBlock;
        if (blockIndex == NONE)
            keepLooking = false;

    }


    BF_Block_Destroy(&block);

    if (!foundValue)
        printf("Could not find value %d\n", value);

    return blocksRequested;
}




