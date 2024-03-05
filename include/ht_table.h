#ifndef HT_TABLE_H
#define HT_TABLE_H

#include <stdbool.h>
#include <record.h>
#include <limits.h>


#define HT_OK 0
#define HT_ERROR (-1)

#define NONE (-1)

/* Index του 1ου Block του Hash File */
#define HEADER_BLOCK 0

/* Identifier του Hash File */
#define HASH_FILE_IDENTIFIER 'G'

/* Μέγιστος αριθμός απο Buckets που μπορεί να έχει ο πίνακας κατακερματισμού
 * Η δομή HT_info περιέχει τα παρακάτω στοιχεία :
 *
 *  # int maxRecords
 *  # int totalBuckets
 *  # int totalBlocks
 *  # int totalRecords
 *  # int fileDescriptor
 *
 *  Ενω στην αρχή του 1ου Block του Hash File βρίσκεται ο char HASH_FILE_IDENTIFIER.
 *  Συνεπώς, ο μέγιστος αριθμός απο Buckets που μπορεί να έχει ο πίνακας κατακερματισμού έτσι ώστε η συνολική δομή HT_info να χωράει στο 1ο Block του Hash File δίνεται απο τον παρακάτω τύπο.
 *  Διαίρεση με το 2 για να είμαστε απόλυτα σίγουροι οτι η συνολική δομή HT_info θα χωράει στο 1ο Block του Hash File και θα μπορούμε μελλοντικά να προσθέσουμε επιπλέον μεταδεδομένα  */
#define MAX_BUCKETS (((BF_BLOCK_SIZE  - (5 * sizeof(int)) - sizeof(char)) / sizeof(int)) / 2)

/* Μέγιστος αριθμός Records ανα Block */
#define MAX_RECORDS ((BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record))

typedef struct {
    int maxRecords;
    int totalBuckets;
    int totalBlocks;
    int totalRecords;
    int fileDescriptor;
    int bucketToBlock[MAX_BUCKETS];
} HT_info;

bool HT_areDifferent(HT_info *htInfo, HT_info *anotherHtInfo);

typedef struct {
    int totalRecords;
    int previousBlock;
    int bucketBlocksSoFar;
    int bucketRecordsSoFar;
} HT_block_info;


typedef struct {
    int blockIndex;
    int recordIndex;
} InsertPosition;

/*Η συνάρτηση HT_CreateFile χρησιμοποιείται για τη δημιουργία
και κατάλληλη αρχικοποίηση ενός άδειου αρχείου κατακερματισμού
με όνομα fileName. Έχει σαν παραμέτρους εισόδου το όνομα του
αρχείου στο οποίο θα κτιστεί ο σωρός και των αριθμό των κάδων
της συνάρτησης κατακερματισμού. Σε περίπτωση που εκτελεστεί
επιτυχώς, επιστρέφεται 0, ενώ σε διαφορετική περίπτωση -1.*/
int HT_CreateFile(
        char *fileName,    /*όνομα αρχείου*/
        int buckets         /*αριθμός από totalBuckets*/);

/*Η συνάρτηση HT_OpenFile ανοίγει το αρχείο με όνομα filename
και διαβάζει από το πρώτο μπλοκ την πληροφορία που αφορά το
αρχείο κατακερματισμού. Κατόπιν, ενημερώνεται μια δομή που κρατάτε
όσες πληροφορίες κρίνονται αναγκαίες για το αρχείο αυτό προκειμένου
να μπορείτε να επεξεργαστείτε στη συνέχεια τις εγγραφές του. Αφού
ενημερωθεί κατάλληλα η δομή πληροφοριών του αρχείου, την επιστρέφετε.
Σε περίπτωση που συμβεί οποιοδήποτε σφάλμα, επιστρέφεται τιμή NULL.
Αν το αρχείο που δόθηκε για άνοιγμα δεν αφορά αρχείο κατακερματισμού,
τότε αυτό επίσης θεωρείται σφάλμα. */
HT_info *HT_OpenFile(char *fileName /*όνομα αρχείου*/);

/*Η συνάρτηση HT_CloseFile κλείνει το αρχείο που προσδιορίζεται μέσα
στη δομή ht_info. Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται
0, ενώ σε διαφορετική περίπτωση -1. Η συνάρτηση είναι υπεύθυνη και για την
αποδέσμευση της μνήμης που καταλαμβάνει η δομή που περάστηκε ως παράμετρος,
στην περίπτωση που το κλείσιμο πραγματοποιήθηκε επιτυχώς.*/
int HT_CloseFile(HT_info *ht_info);

/*Η συνάρτηση HT_InsertEntry χρησιμοποιείται για την εισαγωγή μιας εγγραφής
στο αρχείο κατακερματισμού. Οι πληροφορίες που αφορούν το αρχείο βρίσκονται στη
δομή header_info, ενώ η εγγραφή προς εισαγωγή προσδιορίζεται από τη δομή record.
Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφετε τον αριθμό του block στο οποίο
έγινε η εισαγωγή (blockId), ενώ σε διαφορετική περίπτωση -1.
Η HT_InsertEntry έχει τροποποιηθεί κατάλληλα για να επιστρέφει ενα αντικείμενο της δομής InsertPosition με σκοπό να αντιμετωπιστούν θέματα ambiguity
τα οποία παρατίθενται στo παρακάτω post : https://eclass.uoa.gr/modules/forum/viewtopic.php?course=D22&topic=35182&forum=63149 */
InsertPosition HT_InsertEntry(HT_info *header_info, /*επικεφαλίδα του αρχείου*/ Record record /*δομή που προσδιορίζει την εγγραφή*/);

/* Η συνάρτηση αυτή χρησιμοποιείται για την εκτύπωση όλων των εγγραφών που υπάρχουν
στο αρχείο κατακερματισμού οι οποίες έχουν τιμή στο πεδίο-κλειδί ίση με value.
Η πρώτη δομή δίνει πληροφορία για το αρχείο κατακερματισμού, όπως αυτή είχε επιστραφεί
από τη HT_OpenIndex. Για κάθε εγγραφή που υπάρχει στο αρχείο και έχει τιμή στο πεδίο-κλειδί
(όπως αυτό ορίζεται στη HT_info) ίση με value, εκτυπώνονται τα περιεχόμενά της (συμπεριλαμβανομένου και του πεδίου-κλειδιού). Να επιστρέφεται επίσης το πλήθος των blocks που διαβάστηκαν μέχρι να βρεθούν όλες οι εγγραφές. Σε περίπτωση επιτυχίας επιστρέφει το πλήθος των blocks που διαβάστηκαν, ενώ σε περίπτωση λάθους επιστρέφει -1.*/
int HT_GetAllEntries(HT_info *header_info, /*επικεφαλίδα του αρχείου*/ int id /*τιμή του πεδίου-κλειδιού προς αναζήτηση*/);

int primaryHashStatistics(char *filename /* όνομα του αρχείου που ενδιαφέρει */ );

/* Βοηθητική συνάρτηση για την εκτύπωση των περισσοτέρων πληροφοριών του Hash - File */
int completePrimaryHashFile(HT_info *ht_info);


#endif // HT_FILE_H
