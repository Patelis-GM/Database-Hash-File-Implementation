#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 4269
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

    int buckets = 10;

    /* Κατά τη 2η + εκτέλεση του προγράμματος θα προκύψει το σφάλμα : BF Error: The file is already being used
     * Το παραπάνω οφείλεται στο οτι το αρχείο υπάρχει ηδη και έτσι θα αποτύχει η δημιουργία του, συνεπώς κατά τη 2η + εκτέλεση αγνοήστε το */
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
        InsertPosition insertPosition = HT_InsertEntry(info, record);
        if (insertPosition.blockIndex == HT_ERROR || insertPosition.recordIndex == HT_ERROR)
            return 1;
    }

    int value = rand() % RECORDS_NUM;
    printf("Looking for value %d\n", value);
    int blocksRequested = HT_GetAllEntries(info, value);
    printf("%d blocks requested for value %d\n", blocksRequested, value);

    HT_CloseFile(info);

    primaryHashStatistics(FILE_NAME);

    BF_Close();
}
