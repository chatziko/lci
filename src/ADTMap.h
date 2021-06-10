///////////////////////////////////////////////////////////
//
// ADT Map
//
// Abstract map. Παρέχει γρήγορη αντιστοίχιση key => value.
//
///////////////////////////////////////////////////////////

#pragma once // #include το πολύ μία φορά

#include "common_types.h"


// Ενα map αναπαριστάται από τον τύπο Map

typedef struct map* Map;


// Δημιουργεί και επιστρέφει ένα map, στο οποίο τα στοιχεία συγκρίνονται με βάση
// τη συνάρτηση compare.
// Αν destroy_key ή/και destroy_value != NULL, τότε καλείται destroy_key(key)
// ή/και destroy_value(value) κάθε φορά που αφαιρείται ένα στοιχείο.

Map map_create(CompareFunc compare, DestroyFunc destroy_key, DestroyFunc destroy_value);

// Επιστρέφει τον αριθμό στοιχείων που περιέχει το map.

int map_size(Map map);

// Προσθέτει το κλειδί key με τιμή value. Αν υπάρχει κλειδί ισοδύναμο με key, τα παλιά key & value αντικαθίσταται από τα νέα.
//
// ΠΡΟΣΟΧΗ:
// Όσο το key είναι μέλος του map, οποιαδήποτε μεταβολή στο περιεχόμενό του (στη μνήμη που δείχνει) έχει μη ορισμένη συμπεριφορά.

void map_insert(Map map, Pointer key, Pointer value);

// Αφαιρεί το κλειδί που είναι ισοδύναμο με key από το map, αν υπάρχει.
// Επιστρέφει true αν βρέθηκε τέτοιο κλειδί, διαφορετικά false.

bool map_remove(Map map, Pointer key);

// Επιστρέφει την τιμή που έχει αντιστοιχιστεί στο συγκεκριμένο key, ή NULL αν το key δεν υπάρχει στο map.
//
// Προσοχή: η συνάρτηση επιστρέφει NULL είτε όταν το key δεν υπάρχει, είτε όταν υπάρχει και έχει τιμή NULL.
//          Αν χρειάζεται να διαχωρίσουμε τις δύο περιπτώσεις μπορούμε να χρησιμοποιήσουμε την map_find_node.

Pointer map_find(Map map, Pointer key);

// Αλλάζει τη συνάρτηση που καλείται σε κάθε αφαίρεση/αντικατάσταση key/value.
// Επιστρέφει την προηγούμενη τιμή της συνάρτησης.

DestroyFunc map_set_destroy_key  (Map map, DestroyFunc destroy_key  );
DestroyFunc map_set_destroy_value(Map map, DestroyFunc destroy_value);

// Ελευθερώνει όλη τη μνήμη που δεσμεύει το map.
// Οποιαδήποτε λειτουργία πάνω στο map μετά το destroy είναι μη ορισμένη.

void map_destroy(Map map);



// Διάσχιση του map μέσω κόμβων ////////////////////////////////////////////////////////////
//
// Η σειρά διάσχισης είναι αυθαίρετη.

// Η σταθερά αυτή συμβολίζει έναν εικονικό κόμβου _μετά_ τον τελευταίο κόμβο του map
#define MAP_EOF (MapNode)0

typedef struct map_node* MapNode;

// Επιστρέφει τον πρώτο κομβο του map, ή MAP_EOF αν το map είναι κενό

MapNode map_first(Map map);

// Επιστρέφει τον επόμενο κόμβο του node, ή MAP_EOF αν ο node δεν έχει επόμενο

MapNode map_next(Map map, MapNode node);

// Επιστρέφει το κλειδί του κόμβου node

Pointer map_node_key(Map map, MapNode node);

// Επιστρέφει το περιεχόμενο του κόμβου node

Pointer map_node_value(Map map, MapNode node);

// Βρίσκει και επιστρέφεο τον κόμβο που έχει αντιστοιχιστεί στο κλειδί key,
// ή MAP_EOF αν το κλειδί δεν υπάρχει στο map.

MapNode map_find_node(Map map, Pointer key);


//// Επιπλέον συναρτήσεις για υλοποιήσεις βασισμένες σε hashing ////////////////////////////

// Τύπος συνάρτησης κατακερματισμού

typedef uint (*HashFunc)(Pointer);

// Υλοποιημένες συναρτήσεις κατακερματισμού για συχνούς τύπους δεδομένων

uint hash_string(Pointer value);		// Χρήση όταν το key είναι char*
uint hash_int(Pointer value);			// Χρήση όταν το key είναι int*
uint hash_pointer(Pointer value);		// Χρήση όταν το key είναι pointer που θεωρείται διαφορετικός από οποιονδήποτε άλλο pointer

// Ορίζει τη συνάρτηση κατακερματισμού hash για το συγκεκριμένο map
// Πρέπει να κληθεί μετά την map_create και πριν από οποιαδήποτε άλλη συνάρτηση.

void map_set_hash_function(Map map, HashFunc hash_func);
