#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
#include "ht_table.h"
#include "record.h"

#define VALUE_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return SHT_ERROR;        \
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


int SHT_CreateSecondaryIndex(char *sfileName, int buckets, char *fileName) {

    /* O αριθμός των Buckets πρέπει να ανήκει στο εύρος : (0, MAX_SECONDARY_BUCKETS]  */
    if (buckets <= 0 || buckets > MAX_SECONDARY_BUCKETS) {
        printf("Buckets should be in the following range : (0, %ld]\n", MAX_SECONDARY_BUCKETS);
        return SHT_ERROR;
    }

    /* Δημιουργία Secondary Hash File */
    VALUE_CALL_OR_DIE(BF_CreateFile(sfileName))

    int fileDescriptor;

    /* Άνοιγμα Secondary Hash File */
    VALUE_CALL_OR_DIE(BF_OpenFile(sfileName, &fileDescriptor))

    BF_Block *block;
    BF_Block_Init(&block);
    char *blockData;

    char secondaryHashFileIdentifier = SECONDARY_HASH_FILE_IDENTIFIER;

    /* Δημιουργία και αρχικοποίηση μιας δομής SHT_info που θα αντιγραφεί στο 1ο Block του Secondary Hash File */
    SHT_info headerMetadata;
    headerMetadata.maxSecondaryRecords = MAX_SECONDARY_RECORDS;
    headerMetadata.totalSecondaryBuckets = buckets;
    headerMetadata.totalSecondaryRecords = 0;
    headerMetadata.totalSecondaryBlocks = 1;
    headerMetadata.fileDescriptor = fileDescriptor;

    /* Τα Buckets του Secondary Hash File γίνονται allocate on demand.
     * Ενημέρωσή του πίνακα κατακερματισμού ως εξής :
     *
     * (1) Bucket 0 -> Block - NONE
     * (2) Bucket 1 -> Block - NONE
     * (3) ...
     * (4) Bucket (buckets - 1) -> Block - NONE
     * (5) ...
     * (6) Bucket (MAX_SECONDARY_BUCKETS - 1) -> Block - NONE
     *
     * Τα (5) - (6) δεν είναι απαραίτητα αλλά πραγματοποιούνται για λόγους συνέπειας
     */
    for (int j = 0; j < MAX_SECONDARY_BUCKETS; ++j)
        headerMetadata.bucketToBlock[j] = NONE;

    /* Allocation του 1ου Block */
    VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
    blockData = BF_Block_GetData(block);

    /* Αντιγραφή του SECONDARY_HASH_FILE_IDENTIFIER στο 1ο byte του 1ου Block */
    memcpy(blockData, &secondaryHashFileIdentifier, sizeof(char));

    /* Αντιγραφή των μεταδεδομένων στο 1ο Block */
    memcpy(blockData + sizeof(char), &headerMetadata, sizeof(SHT_info));

    BF_Block_SetDirty(block);
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))


    BF_Block_Destroy(&block);
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

    return SHT_OK;

}

SHT_info *SHT_OpenSecondaryIndex(char *indexName) {


    int fileDescriptor;
    POINTER_CALL_OR_DIE(BF_OpenFile(indexName, &fileDescriptor))


    BF_Block *block;
    BF_Block_Init(&block);

    POINTER_CALL_OR_DIE(BF_GetBlock(fileDescriptor, SECONDARY_HEADER_BLOCK, block))

    char *blockData = BF_Block_GetData(block);

    char secondaryHashFileIdentifier;
    memcpy(&secondaryHashFileIdentifier, blockData, sizeof(char));

    /* Το πρώτο byte του Secondary Hash File πρέπει να έχει αξία ιση με SECONDARY_HASH_FILE_IDENTIFIER */
    if (secondaryHashFileIdentifier != SECONDARY_HASH_FILE_IDENTIFIER) {

        printf("First byte of SECONDARY-HEADER-BLOCK should be : %c\n", SECONDARY_HASH_FILE_IDENTIFIER);

        /* Unpin του 1ου Block */
        POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

        /* Κλείσιμο του εκάστοτε αρχείου */
        POINTER_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

        /* Αποδέσμευση του BF_Block */
        BF_Block_Destroy(&block);
        return NULL;
    }

    /* Αντιγραφή των μεταδεδομένων του 1ου Block στη δομή SHT_info */
    SHT_info *headerMetadata = (SHT_info *) malloc(sizeof(SHT_info));
    memcpy(headerMetadata, blockData + sizeof(char), sizeof(SHT_info));

    /* Unpin του 1ου Block */
    POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

    /* Αποδέσμευση του BF_Block */
    BF_Block_Destroy(&block);
    return headerMetadata;
}

/* Βοηθητική συνάρτηση για να κριθεί αν δυο δομές SHT_info διαφέρουν μεταξύ τους */
bool SHT_areDifferent(SHT_info *shtInfo, SHT_info *anotherShtInfo) {

    if (shtInfo->totalSecondaryBlocks != anotherShtInfo->totalSecondaryBlocks || shtInfo->totalSecondaryRecords != anotherShtInfo->totalSecondaryRecords)
        return true;

    for (int i = 0; i < MAX_SECONDARY_BUCKETS; ++i)
        if (shtInfo->bucketToBlock[i] != anotherShtInfo->bucketToBlock[i])
            return true;

    return false;
}


int SHT_CloseSecondaryIndex(SHT_info *shtInfo) {

    int fileDescriptor = shtInfo->fileDescriptor;

    BF_Block *block;
    BF_Block_Init(&block);

    /* Ανάκτηση του 1ου Block του Secondary Hash File */
    VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, SECONDARY_HEADER_BLOCK, block))

    char *blockData = BF_Block_GetData(block);

    /* Ανάκτηση των μεταδεδομένων του 1ου Block του Secondary Hash File */
    SHT_info headerMetadata;
    memcpy(&headerMetadata, blockData + sizeof(char), sizeof(SHT_info));

    /* Έλεγχος για τον αν η δομή SHT_info* shtInfo διαφέρει με τα μεταδεδομένα του 1ου Block του Secondary Hash file */
    if (SHT_areDifferent(shtInfo, &headerMetadata)) {

        /* Εφόσον οι δυο δομές διαφέρουν μεταξύ τους ενημέρωση των μεταδεδομένων του 1ου Block του Secondary Hash file */
        memcpy(blockData + sizeof(char), shtInfo, sizeof(SHT_info));

        BF_Block_SetDirty(block);
    }

    /* Unpin του 1ου Block του Secondary Hash File */
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

    /* Αποδέσμευση μνήμης εφόσον η δομή SHT_info* ht_info δεσμεύθηκε δυναμικά */
    free(shtInfo);

    /* Κλείσιμο του Secondary Hash File */
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

    /* Αποδέσμευση του BF_Block */
    BF_Block_Destroy(&block);

    return SHT_OK;
}


unsigned int hash(SecondaryRecord *secondaryRecord) {

    int character;
    unsigned int hashValue = 5381;

    for (int i = 0; i < strlen(secondaryRecord->name); ++i) {
        character = secondaryRecord->name[i];
        hashValue = ((hashValue << 5) + hashValue) + character;
    }

    return hashValue;
}

int SHT_SecondaryInsertEntry(SHT_info *sht_info, Record record, InsertPosition insertPosition) {

    SecondaryRecord secondaryRecord = secondaryRecordFromRecord(record, insertPosition.blockIndex, insertPosition.recordIndex);

    unsigned int hashValue = hash(&secondaryRecord);
    int bucketIndex = hashValue % sht_info->totalSecondaryBuckets;
    int blockIndex = sht_info->bucketToBlock[bucketIndex];

    int fileDescriptor = sht_info->fileDescriptor;

    BF_Block *block;
    char *blockData;
    SHT_block_info bucketMetadata;
    BF_Block_Init(&block);

    /* Εφόσον το allocation των Buckets του Secondary Hash File γίνεται on demand ενδεχομένως να μην έχει γίνει Block allocation για το εκάστοτε Bucket Index */
    if (blockIndex == NONE) {

        /* Μεταδεδομένα του Block που πρόκειται να γίνει allocate και θα αντιγραφούν σε αυτό */
        bucketMetadata.totalSecondaryRecords = 1;
        bucketMetadata.previousBlock = NONE;

        /* Allocation του Block */
        VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
        blockData = BF_Block_GetData(block);

        /* Αντιγραφή των μεταδεδομένων στο allocated Block */
        memcpy(blockData, &bucketMetadata, sizeof(SHT_block_info));

        /* Αντιγραφή του εκάστοτε Secondary Record στην κατάλληλη θέση του allocated Block */
        memcpy(blockData + sizeof(SHT_block_info), &secondaryRecord, sizeof(SecondaryRecord));

        BF_Block_SetDirty(block);
        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

        /* Ενημέρωση του πίνακα κατακερματισμού της δομής SHT_info για το Block που αντιστοιχεί πλέον στο εκάστοτε Bucket Index */
        sht_info->bucketToBlock[bucketIndex] = sht_info->totalSecondaryBlocks;

        /* Ενημέρωση της δομής SHT_info για τον συνολικό αριθμό των Blocks που απαρτίζουν πλέον το Secondary Hash File */
        sht_info->totalSecondaryBlocks += 1;

        blockIndex = sht_info->bucketToBlock[bucketIndex];
    }

        /* Το allocation των Buckets του Secondary Hash File γίνεται on demand αλλά εν προκειμένω έχει ήδη γίνει Block allocation για το εκάστοτε Bucket Index */
    else {

        /* Ανάκτηση του Block που αντιστοιχεί στο εκάστοτε Bucket Index καθώς και των μεταδεδομένων αυτού */
        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, blockIndex, block))

        blockData = BF_Block_GetData(block);

        /* Αντιγραφή των μεταδεδομένων του Block στην proxy δομή SHT_block_info bucketMetadata */
        memcpy(&bucketMetadata, blockData, sizeof(SHT_block_info));

        /* Το εκάστοτε Block που ανακτήθηκε έχει αρκετό χώρο για την εισαγωγή του εκάστοτε Record */
        if (bucketMetadata.totalSecondaryRecords < sht_info->maxSecondaryRecords) {

            /* Αντιγραφή του εκάστοτε Secondary Record στην κατάλληλη θέση του Block */
            memcpy(blockData + sizeof(SHT_block_info) + (bucketMetadata.totalSecondaryRecords * sizeof(SecondaryRecord)), &secondaryRecord,
                   sizeof(SecondaryRecord));

            /* Ενημέρωση των μεταδεδομένων του Block και αντιγραφή αυτών στο τελευταίο */
            bucketMetadata.totalSecondaryRecords += 1;
            memcpy(blockData, &bucketMetadata, sizeof(SHT_block_info));

            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
        }

            /* Το εκάστοτε Block που ανακτήθηκε δεν έχει αρκετό χώρο για την εισαγωγή του εκάστοτε Secondary Record */
        else {

            /* Εφόσον θα πραγματοποιηθεί allocation ενός νέου Block λόγω overflow :
             *
             * # Το επόμενο Block του τωρινού Block θα είναι ο αριθμός των Blocks του Hash File ΠΡΙΝ το allocation.
             *   Aν το Secondary Hash File απαρτίζεται απο έστω 5 Blocks στη συνέχεια θα απαρτίζεται απο 6 Blocks και το index του allocated Block θα είναι το 5
             *
             * # Το προηγούμενο Block του allocated Block θα είναι το index του Block στο οποίο έγινε απόπειρα για την εισαγωγή του εκάστοτε Secondary Record
             */
            int previousBlock = blockIndex;
            int nextBlock = sht_info->totalSecondaryBlocks;

            /* Αποδέσμευση του τωρινού Block */
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

            /* Αρχικοποίηση των μεταδεδομένων του Block που θα γίνει allocate */
            bucketMetadata.totalSecondaryRecords = 1;
            bucketMetadata.previousBlock = previousBlock;

            /* Allocation του Block */
            VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))

            blockData = BF_Block_GetData(block);

            /* Αντιγραφή των μεταδεδομένων στο allocated Block */
            memcpy(blockData, &bucketMetadata, sizeof(SHT_block_info));

            /* Αντιγραφή του εκάστοτε Secondary Record στην κατάλληλη θέση του allocated Block */
            memcpy(blockData + sizeof(SHT_block_info), &secondaryRecord, sizeof(SecondaryRecord));

            BF_Block_SetDirty(block);
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

            /* Ενημέρωση της δομής SHT_info για τον συνολικό αριθμό των Blocks που απαρτίζουν πλέον το Secondary Hash File */
            sht_info->totalSecondaryBlocks += 1;

            /* Ενημέρωση του πίνακα κατακερματισμού της δομής SHT_info οτι το allocated Block αντιστοιχεί πλέον στο εκάστοτε Bucket Index */
            sht_info->bucketToBlock[bucketIndex] = nextBlock;

            blockIndex = nextBlock;
        }

    }

    /* Ενημέρωση της δομής SHT_info για τον συνολικό αριθμό των Secondary Records του Secondary Hash File */
    sht_info->totalSecondaryRecords += 1;
    BF_Block_Destroy(&block);
    return blockIndex;
}


int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info, char *name) {

    return SHT_OK;
}

int secondaryHashStatistics(char *filename) {


    return SHT_OK;
}