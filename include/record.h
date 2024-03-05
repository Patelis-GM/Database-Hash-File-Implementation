#ifndef RECORD_H
#define RECORD_H


typedef enum Record_Attribute {
    ID,
    NAME,
    SURNAME,
    CITY
} Record_Attribute;

typedef struct Record {
    char record[15];
    int id;
    char name[15];
    char surname[20];
    char city[20];
} Record;

typedef struct SecondaryRecord {
    char name[15];
    int blockIndex;
    int recordIndex;
} SecondaryRecord;


Record randomRecord();

void printRecord(Record record);

SecondaryRecord secondaryRecordFromRecord(Record record, int blockIndex, int recordIndex);

void printSecondaryRecord(SecondaryRecord secondaryRecord);

#endif
