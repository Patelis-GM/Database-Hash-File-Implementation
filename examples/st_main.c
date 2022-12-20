#include <stdlib.h>
#include <stdio.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

int main() {

    BF_Init(LRU);


    printf("Primary Hash File Statistics :\n");
    int status = primaryHashStatistics(FILE_NAME);
    if (status != HT_OK) {
        BF_Close();
        exit(1);
    }


    printf("Secondary Hash File Statistics :\n");
    status = secondaryHashStatistics(INDEX_NAME);
    if (status != SHT_OK) {
        BF_Close();
        exit(1);
    }

    BF_Close();

    return 0;
}

