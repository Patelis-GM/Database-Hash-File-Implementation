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
      return NULL;            \
    }                         \
  }


int HT_CreateFile(char *fileName, int buckets) {

    /* O αριθμός των Buckets πρέπει να ανήκει στο εύρος : (0, ΜΑΧ_BUCKETS]  */
    if (buckets <= 0 || buckets > MAX_BUCKETS) {
        printf("Buckets should be in the following range : (0, %ld]\n", MAX_BUCKETS);
        return HT_ERROR;
    }

    /* Δημιουργία Hash File */
    VALUE_CALL_OR_DIE(BF_CreateFile(fileName))

    int fileDescriptor;

    /* Άνοιγμα Hash File */
    VALUE_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))

    BF_Block *block;
    BF_Block_Init(&block);
    char *blockData;

    char hashFileIdentifier = HASH_FILE_IDENTIFIER;

    /* Δημιουργία και αρχικοποίηση μιας δομής HT_info που θα αντιγραφεί στο 1ο Block του Hash File */
    HT_info headerMetadata;
    headerMetadata.totalBuckets = buckets;
    headerMetadata.totalRecords = 0;
    headerMetadata.totalBlocks = 1;
    headerMetadata.fileDescriptor = fileDescriptor;

    /* Τα Buckets του Hash File γίνονται allocate on demand.
     * Ενημέρωσή του πίνακα κατακερματισμού ως εξής :
     *
     * (1) Bucket 0 -> Block - [-1]
     * (2) Bucket 1 -> Block - [-1]
     * (3) ...
     * (4) Bucket (buckets - 1) -> Block - [-1]
     * (5) ...
     * (6) Bucket (MAX_BUCKETS - 1) -> Block - [-1]
     *
     * Τα (5) - (6) δεν είναι απαραίτητα αλλά πραγματοποιούνται για λόγους συνέπειας
     */
    for (int j = 0; j < MAX_BUCKETS; ++j)
        headerMetadata.bucketToBlock[j] = NONE;

    /* Allocation του 1ου Block */
    VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
    blockData = BF_Block_GetData(block);

    /* Αντιγραφή του HASH_FILE_IDENTIFIER στο 1ο byte του 1ου Block */
    memcpy(blockData, &hashFileIdentifier, sizeof(char));

    /* Αντιγραφή των μεταδεδομένων στο 1ο Block */
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

    /* Το πρώτο byte του Hash File πρέπει να έχει αξία ιση με HASH_FILE_IDENTIFIER */
    if (hashFileIdentifier != HASH_FILE_IDENTIFIER) {

        printf("First byte of HEADER-BLOCK should be : %c\n", HASH_FILE_IDENTIFIER);

        /* Unpin του 1ου Block */
        POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

        /* Κλείσιμο του εκάστοτε αρχείου */
        POINTER_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

        /* Αποδέσμευση του BF_Block */
        BF_Block_Destroy(&block);
        return NULL;
    }

    /* Αντιγραφή των μεταδεδομένων του 1ου Block στη δομή HT_info */
    HT_info *headerMetadata = (HT_info *) malloc(sizeof(HT_info));
    memcpy(headerMetadata, blockData + sizeof(char), sizeof(HT_info));

    /* Unpin του 1ου Block */
    POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

    /* Αποδέσμευση του BF_Block */
    BF_Block_Destroy(&block);
    return headerMetadata;
}

/* Βοηθητική συνάρτηση για να κριθεί αν δυο δομές HT_info διαφέρουν μεταξύ τους */
bool areDifferent(HT_info *htInfo, HT_info *anotherHtInfo) {

    if (htInfo->totalBlocks != anotherHtInfo->totalBlocks || htInfo->totalRecords != anotherHtInfo->totalRecords)
        return true;

    for (int i = 0; i < MAX_BUCKETS; ++i)
        if (htInfo->bucketToBlock[i] != anotherHtInfo->bucketToBlock[i])
            return true;

    return false;
}


int HT_CloseFile(HT_info *ht_info) {

    int fileDescriptor = ht_info->fileDescriptor;

    BF_Block *block;
    BF_Block_Init(&block);

    /* Ανάκτηση του 1ου Block του Hash File */
    VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))

    char *blockData = BF_Block_GetData(block);

    /* Ανάκτηση των μεταδεδομένων του 1ου Block του Hash File */
    HT_info headerMetadata;
    memcpy(&headerMetadata, blockData + sizeof(char), sizeof(HT_info));

    /* Έλεγχος για τον αν η δομή HT_info* ht_info διαφέρει με τα μεταδεδομένα του 1ου Block του Hash file */
    if (areDifferent(ht_info, &headerMetadata)) {

        /* Εφόσον οι δυο δομές διαφέρουν μεταξύ τους ενημέρωση των μεταδεδομένων του 1ου Block του Hash file */
        memcpy(blockData + sizeof(char), ht_info, sizeof(HT_info));

        BF_Block_SetDirty(block);
    }

    /* Unpin του 1ου Block του Hash File */
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

    /* Αποδέσμευση μνήμης εφόσον η δομή HT_info* ht_info δεσμεύθηκε δυναμικά */
    free(ht_info);

    /* Κλείσιμο του Hash File */
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

    /* Αποδέσμευση του BF_Block */
    BF_Block_Destroy(&block);

    return HT_OK;
}

int HT_InsertEntry(HT_info *ht_info, Record record) {

    /* Υπολογισμός του Bucket Index του εκάστοτε Record */
    int bucketIndex = record.id % ht_info->totalBuckets;

    /* Ανάκτηση του Block Index στο οποίο αντιστοιχεί το εκάστοτε Bucket Index */
    int blockIndex = ht_info->bucketToBlock[bucketIndex];

    int fileDescriptor = ht_info->fileDescriptor;

    BF_Block *block;
    char *blockData;
    HT_block_info bucketMetadata;
    BF_Block_Init(&block);

    /* Εφόσον το allocation των Buckets του Hash File γίνεται on demand ενδεχομένως να μην έχει γίνει Block allocation για το εκάστοτε Bucket Index */
    if (blockIndex == NONE) {

        /* Μεταδεδομένα του Block που πρόκειται να γίνει allocate και θα αντιγραφούν σε αυτό */
        bucketMetadata.totalRecords = 1;
        bucketMetadata.previousBlock = NONE;

        /* Allocation του Block */
        VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
        blockData = BF_Block_GetData(block);

        /* Αντιγραφή των μεταδεδομένων στο allocated Block */
        memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));

        /* Αντιγραφή του εκάστοτε Record στην κατάλληλη θέση του allocated Block */
        memcpy(blockData + sizeof(HT_block_info), &record, sizeof(Record));

        BF_Block_SetDirty(block);
        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

        /* Ενημέρωση του πίνακα κατακερματισμού της δομής HT_info για το Block που αντιστοιχεί πλέον στο εκάστοτε Bucket Index */
        ht_info->bucketToBlock[bucketIndex] = ht_info->totalBlocks;

        /* Ενημέρωση της δομής HT_info για τον συνολικό αριθμό των Blocks που απαρτίζουν πλέον το Hash File */
        ht_info->totalBlocks += 1;

        blockIndex = ht_info->bucketToBlock[bucketIndex];
    }

        /* Το allocation των Buckets του Hash File γίνεται on demand αλλά εν προκειμένω έχει ήδη γίνει Block allocation για το εκάστοτε Bucket Index */
    else {


        /* Ανάκτηση του Block που αντιστοιχεί στο εκάστοτε Bucket Index καθώς και των μεταδεδομένων αυτού */
        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, blockIndex, block))

        blockData = BF_Block_GetData(block);

        /* Αντιγραφή των μεταδεδομένων του Block στην proxy δομή HT_block_info bucketMetadata */
        memcpy(&bucketMetadata, blockData, sizeof(HT_block_info));

        /* Το εκάστοτε Block που ανακτήθηκε έχει αρκετό χώρο για την εισαγωγή του εκάστοτε Record */
        if (bucketMetadata.totalRecords < MAX_RECORDS) {

            /* Αντιγραφή του εκάστοτε Record στην κατάλληλη θέση του Block */
            memcpy(blockData + sizeof(HT_block_info) + (bucketMetadata.totalRecords * sizeof(Record)), &record,
                   sizeof(Record));

            /* Ενημέρωση των μεταδεδομένων του Block και αντιγραφή αυτών στο τελευταίο */
            bucketMetadata.totalRecords += 1;
            memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));

            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

        }

            /* Το εκάστοτε Block που ανακτήθηκε δεν έχει αρκετό χώρο για την εισαγωγή του εκάστοτε Record */
        else {

            /* Εφόσον θα πραγματοποιηθεί allocation ενός νέου Block λόγω overflow :
             *
             * # Το επόμενο Block του τωρινού Block θα είναι ο αριθμός των Blocks του Hash File ΠΡΙΝ το allocation.
             *   Aν το Hash File απαρτίζεται απο έστω 5 Blocks στη συνέχεια θα απαρτίζεται απο 6 Blocks και το index του allocated Block θα είναι το 5
             *
             * # Το προηγούμενο Block του allocated Block θα είναι το index του Block στο οποίο έγινε απόπειρα για την εισαγωγή του εκάστοτε Record
             */
            int previousBlock = blockIndex;
            int nextBlock = ht_info->totalBlocks;

            /* Αποδέσμευση του τωρινού Block */
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

            /* Αρχικοποίηση των μεταδεδομένων του Block που θα γίνει allocate */
            bucketMetadata.totalRecords = 1;
            bucketMetadata.previousBlock = previousBlock;

            /* Allocation του Block */
            VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))


            blockData = BF_Block_GetData(block);

            /* Αντιγραφή των μεταδεδομένων στο allocated Block */
            memcpy(blockData, &bucketMetadata, sizeof(HT_block_info));

            /* Αντιγραφή του εκάστοτε Record στην κατάλληλη θέση του allocated Block */
            memcpy(blockData + sizeof(HT_block_info), &record, sizeof(Record));

            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

            /* Ενημέρωση της δομής HT_info για τον συνολικό αριθμό των Blocks που απαρτίζουν πλέον το Hash File */
            ht_info->totalBlocks += 1;

            /* Ενημέρωση του πίνακα κατακερματισμού της δομής HT_info οτι το allocated Block αντιστοιχεί πλέον στο εκάστοτε Bucket Index */
            ht_info->bucketToBlock[bucketIndex] = nextBlock;

            blockIndex = nextBlock;
        }


    }

    /* Ενημέρωση της δομής HT_info για τον συνολικό αριθμό των Records του Hash File */
    ht_info->totalRecords += 1;

    BF_Block_Destroy(&block);
    return blockIndex;
}

int HT_GetAllEntries(HT_info *ht_info, int value) {

    printf("Looking for value %d\n", value);

    /* Υπολογισμός του Bucket Index του εκάστοτε Record */
    int bucketIndex = value % ht_info->totalBuckets;

    /* Ανάκτηση του Block Index στο οποίο αντιστοιχεί το εκάστοτε Bucket Index */
    int blockIndex = ht_info->bucketToBlock[bucketIndex];

    int fileDescriptor = ht_info->fileDescriptor;
    int blocksRequested = 0;

    bool keepLooking = true;
    bool foundValue = false;

    BF_Block *block;
    char *blockData;
    HT_block_info bucketMetadata;
    Record record;
    BF_Block_Init(&block);

    /* Εφόσον το allocation των Buckets του Hash File γίνεται on demand γνωρίζουμε εκ των προτέρων οτι το εκάστοτε Record δε βρίσκεται στο Hash - File */
    if (blockIndex == NONE)
        keepLooking = false;

    while (keepLooking) {

        printf("Looking in block %d\n", blockIndex);

        /* Ανάκτηση του κατάλληλου Block */
        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, blockIndex, block))
        blockData = BF_Block_GetData(block);

        /* Αντιγραφή των μεταδεδομένων του Block στην proxy δομή HT_block_info bucketMetadata */
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

        /* Ανάκτηση του Index του προηγούμενου Block */
        blockIndex = bucketMetadata.previousBlock;
        if (blockIndex == NONE)
            keepLooking = false;

    }


    BF_Block_Destroy(&block);

    if (!foundValue)
        printf("Could not find value %d\n", value);

    return blocksRequested;
}


void HT_PrintAllEntries(HT_info *ht_info) {

    int fileDescriptor = ht_info->fileDescriptor;

    BF_Block *block;
    char *blockData;
    HT_block_info bucketMetadata;
    Record record;
    int blockIndex;
    BF_Block_Init(&block);

    for (int i = 0; i < ht_info->totalBuckets; ++i) {

        bool keepLooking = true;
        blockIndex = ht_info->bucketToBlock[i];

        if (blockIndex == NONE)
            printf("Bucket %d has no records\n", i);

        else {

            while (keepLooking) {

                BF_GetBlock(fileDescriptor, blockIndex, block);
                blockData = BF_Block_GetData(block);
                memcpy(&bucketMetadata, blockData, sizeof(HT_block_info));

                printf("Block %d has %d records\n", blockIndex, bucketMetadata.totalRecords);
                for (int j = 0; j < bucketMetadata.totalRecords; ++j) {
                    memcpy(&record, blockData + sizeof(HT_block_info) + (j * sizeof(Record)), sizeof(Record));
                    printRecord(record);
                }

                BF_UnpinBlock(block);

                blockIndex = bucketMetadata.previousBlock;
                if (blockIndex == NONE)
                    keepLooking = false;
            }

        }

    }

    BF_Block_Destroy(&block);

}


int HashStatistics(char *filename) {

    HT_info *info = HT_OpenFile(filename);

    if (info == NULL)
        return HT_ERROR;

    printf("Total blocks of Hash File : %d\n", info->totalBlocks);

    int bucketBlocks[info->totalBuckets];
    int bucketRecords[info->totalBuckets];
    int fileDescriptor = info->fileDescriptor;
    BF_Block *block;
    char *blockData;
    HT_block_info bucketMetadata;
    BF_Block_Init(&block);

    for (int bucketIndex = 0; bucketIndex < info->totalBuckets; ++bucketIndex) {

        int blockIndex = info->bucketToBlock[bucketIndex];

        /* Το allocation των Buckets του Hash File γίνεται on demand */
        if (blockIndex == NONE) {
            bucketBlocks[bucketIndex] = 0;
            bucketRecords[bucketIndex] = 0;
        }

        else {

            bool keepLooking = true;
            int totalRecords = 0;
            int totalBlocks = 0;

            while (keepLooking) {

                /* Ανάκτηση του κατάλληλου Block */
                VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, blockIndex, block))
                blockData = BF_Block_GetData(block);

                /* Αντιγραφή των μεταδεδομένων του Block στην proxy δομή HT_block_info bucketMetadata */
                memcpy(&bucketMetadata, blockData, sizeof(HT_block_info));
                totalBlocks += 1;
                totalRecords += bucketMetadata.totalRecords;

                VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

                /* Ανάκτηση του Index του προηγούμενου Block */
                blockIndex = bucketMetadata.previousBlock;

                if (blockIndex == NONE)
                    keepLooking = false;

            }


            bucketBlocks[bucketIndex] = totalBlocks;
            bucketRecords[bucketIndex] = totalRecords;

        }

    }


    int averageNumberOfBlocks = 0;
    for (int bucketIndex = 0; bucketIndex < info->totalBuckets; ++bucketIndex)
        averageNumberOfBlocks += bucketBlocks[bucketIndex];
    printf("%f number of blocks per bucket\n", (float) ((float) averageNumberOfBlocks / (float) info->totalBuckets));


    int mostRecords = INT_MIN;
    int mostRecordsIndex;
    int leastRecords = INT_MAX;
    int leastRecordsIndex;
    int averageNumberOfRecords = 0;

    for (int bucketIndex = 0; bucketIndex < info->totalBuckets; ++bucketIndex) {

        int totalRecords = bucketRecords[bucketIndex];
        printf("Bucket %d has %d records\n", bucketIndex, totalRecords);

        if (totalRecords > mostRecords) {
            mostRecords = totalRecords;
            mostRecordsIndex = bucketIndex;
        }

        if (totalRecords < leastRecords) {
            leastRecords = totalRecords;
            leastRecordsIndex = bucketIndex;
        }

        averageNumberOfRecords += totalRecords;
    }
    printf("%f number of records per bucket\n", (float) ((float) averageNumberOfRecords / (float) info->totalBuckets));
    printf("Bucket %d has the least number of %d records\n", leastRecordsIndex, leastRecords);
    printf("Bucket %d has the maximum number of %d records\n", mostRecordsIndex, mostRecords);


    int totalBucketsWithOverflow = 0;
    for (int bucketIndex = 0; bucketIndex < info->totalBuckets; ++bucketIndex)
        if (bucketBlocks[bucketIndex] > 1) {
            totalBucketsWithOverflow += 1;
            printf("Bucket %d has %d overflow blocks\n", bucketIndex, bucketBlocks[bucketIndex] - 1);
        }
    printf("Overflow occurs in %d buckets\n", totalBucketsWithOverflow);


    BF_Block_Destroy(&block);

    int code = HT_CloseFile(info);

    if (code != HT_OK)
        return HT_ERROR;

    return HT_OK;
}

