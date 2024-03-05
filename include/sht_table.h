#ifndef SHT_TABLE_H
#define SHT_TABLE_H

#include <record.h>
#include <ht_table.h>
#include <math.h>


#define SHT_OK 0
#define SHT_ERROR (-1)

#define NONE (-1)

/* Index του 1ου Block του Secondary Hash File */
#define SECONDARY_HEADER_BLOCK 0

/* Identifier του Secondary Hash File */
#define SECONDARY_HASH_FILE_IDENTIFIER 'S'


/* Μέγιστος αριθμός απο Buckets που μπορεί να έχει ο πίνακας κατακερματισμού
 * Η δομή SHT_info περιέχει τα παρακάτω στοιχεία :
 *
 * # int maxSecondaryRecords
 * # int totalSecondaryBuckets
 * # int totalSecondaryBlocks
 * # int totalSecondaryRecords
 * # int fileDescriptor
 *
 *  Ενω στην αρχή του 1ου Block του Secondary Hash File βρίσκεται ο char SECONDARY_HASH_FILE_IDENTIFIER.
 *  Συνεπώς, ο μέγιστος αριθμός απο Buckets που μπορεί να έχει ο πίνακας κατακερματισμού έτσι ώστε η συνολική δομή SHT_info να χωράει στο 1ο Block του Secondary Hash File δίνεται απο τον παρακάτω τύπο.
 *  Διαίρεση με το 2 για να είμαστε απόλυτα σίγουροι οτι η συνολική δομή SHT_info θα χωράει στο 1ο Block του Secondary Hash File και θα μπορούμε μελλοντικά να προσθέσουμε επιπλέον μεταδεδομένα  */
#define MAX_SECONDARY_BUCKETS (((BF_BLOCK_SIZE  - (5 * sizeof(int)) - sizeof(char)) / sizeof(int)) / 2)

/* Μέγιστος αριθμός Secondary Records ανα Block του Secondary Hash File */
#define MAX_SECONDARY_RECORDS ((BF_BLOCK_SIZE - sizeof(SHT_block_info)) / sizeof(SecondaryRecord))

typedef struct {
    int maxSecondaryRecords;
    int totalSecondaryBuckets;
    int totalSecondaryBlocks;
    int totalSecondaryRecords;
    int fileDescriptor;
    int bucketToBlock[MAX_SECONDARY_BUCKETS];
} SHT_info;

bool SHT_areDifferent(SHT_info* shtInfo, SHT_info* anotherShtInfo);

typedef struct {
    int bucketBlocksSoFar;
    int bucketRecordsSoFar;
    int previousBlock;
    int totalSecondaryRecords;
} SHT_block_info;

/*Η συνάρτηση SHT_CreateSecondaryIndex χρησιμοποιείται για τη δημιουργία
και κατάλληλη αρχικοποίηση ενός αρχείου δευτερεύοντος κατακερματισμού με
όνομα sfileName για το αρχείο πρωτεύοντος κατακερματισμού fileName. Σε
περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται 0, ενώ σε διαφορετική
περίπτωση -1.*/
int SHT_CreateSecondaryIndex(
        char *sfileName, /* όνομα αρχείου δευτερεύοντος ευρετηρίου*/
        int buckets, /* αριθμός κάδων κατακερματισμού*/
        char *fileName /* όνομα αρχείου πρωτεύοντος ευρετηρίου*/);


/* Η συνάρτηση SHT_OpenSecondaryIndex ανοίγει το αρχείο με όνομα sfileName
και διαβάζει από το πρώτο μπλοκ την πληροφορία που αφορά το δευτερεύον
ευρετήριο κατακερματισμού.*/
SHT_info *SHT_OpenSecondaryIndex(
        char *sfileName /* όνομα αρχείου δευτερεύοντος ευρετηρίου */);

/*Η συνάρτηση SHT_CloseSecondaryIndex κλείνει το αρχείο που προσδιορίζεται
μέσα στη δομή shtInfo. Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται
0, ενώ σε διαφορετική περίπτωση -1. Η συνάρτηση είναι υπεύθυνη και για την
αποδέσμευση της μνήμης που καταλαμβάνει η δομή που περάστηκε ως παράμετρος,
στην περίπτωση που το κλείσιμο πραγματοποιήθηκε επιτυχώς.*/
int SHT_CloseSecondaryIndex(SHT_info *shtInfo);

/*Η συνάρτηση SHT_SecondaryInsertEntry χρησιμοποιείται για την εισαγωγή μιας
εγγραφής στο αρχείο κατακερματισμού. Οι πληροφορίες που αφορούν το αρχείο
βρίσκονται στη δομή header_info, ενώ η εγγραφή προς εισαγωγή προσδιορίζεται
από τη δομή record και το block του πρωτεύοντος ευρετηρίου που υπάρχει η εγγραφή
προς εισαγωγή. Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται 0, ενώ σε
διαφορετική περίπτωση -1.*/
int SHT_SecondaryInsertEntry(
        SHT_info *header_info, /* επικεφαλίδα του δευτερεύοντος ευρετηρίου*/
        Record record, /* η εγγραφή για την οποία έχουμε εισαγωγή στο δευτερεύον ευρετήριο*/
        InsertPosition insertPosition /* το μπλοκ του αρχείου κατακερματισμού στο οποίο έγινε η εισαγωγή */);

/*Η συνάρτηση αυτή χρησιμοποιείται για την εκτύπωση όλων των εγγραφών που
υπάρχουν στο αρχείο κατακερματισμού οι οποίες έχουν τιμή στο πεδίο-κλειδί
του δευτερεύοντος ευρετηρίου ίση με name. Η πρώτη δομή περιέχει πληροφορίες
για το αρχείο κατακερματισμού, όπως αυτές είχαν επιστραφεί κατά το άνοιγμά
του. Η δεύτερη δομή περιέχει πληροφορίες για το δευτερεύον ευρετήριο όπως
αυτές είχαν επιστραφεί από την SHT_OpenIndex. Για κάθε εγγραφή που υπάρχει
στο αρχείο και έχει όνομα ίσο με value, εκτυπώνονται τα περιεχόμενά της
(συμπεριλαμβανομένου και του πεδίου-κλειδιού). Να επιστρέφεται επίσης το
πλήθος των blocks που διαβάστηκαν μέχρι να βρεθούν όλες οι εγγραφές. Σε
περίπτωση λάθους επιστρέφει -1.*/
int SHT_SecondaryGetAllEntries(
        HT_info *ht_info, /* επικεφαλίδα του αρχείου πρωτεύοντος ευρετηρίου*/
        SHT_info *header_info, /* επικεφαλίδα του αρχείου δευτερεύοντος ευρετηρίου*/
        char *name /* το όνομα στο οποίο γίνεται αναζήτηση */);


unsigned int hash(const char* field);

int secondaryHashStatistics(char *filename);

int completeSecondaryHashFile(SHT_info *sht_info);

#endif // SHT_FILE_H
