///////////////////////////////////////////////////////////////////
//
// ADT Vector
//
// Abstract "array" μεταβλητού μεγέθους. Παρέχει πρόσβαση σε
// οποιοδήποτε στοιχείο (μέσω της θέσης του), και προσθήκη/αφαίρεση
// στοιχείων στο τέλος του vector.
//
///////////////////////////////////////////////////////////////////

#pragma once // #include το πολύ μία φορά

#include "common_types.h"


// Ένα vector αναπαριστάται από τον τύπο Vector. Ο χρήστης δε χρειάζεται να γνωρίζει το περιεχόμενο
// του τύπου αυτού, απλά χρησιμοποιεί τις συναρτήσεις vector_<foo> που δέχονται και επιστρέφουν Vector.
//
// Ο τύπος Vector ορίζεται ως pointer στο "struct vector" του οποίου το περιεχόμενο είναι άγνωστο
// (incomplete struct), και εξαρτάται από την υλοποίηση του ADT Vector.
//
typedef struct vector* Vector;


// Δημιουργεί και επιστρέφει ένα νεό vector μεγέθους size, με στοιχεία αρχικοποιημένα σε NULL.
// Αν destroy_value != NULL, τότε καλείται destroy_value(value) κάθε φορά που αφαιρείται (ή αντικαθίσταται) ένα στοιχείο.

Vector vector_create(int size, DestroyFunc destroy_value);

// Επιστρέφει τον αριθμό στοιχείων που περιέχει το vector vec.

int vector_size(Vector vec);

// Προσθέτει την τιμή value στο _τέλος_ του vector vec. Το μέγεθος του vector μεγαλώνει κατά 1.

void vector_insert_last(Vector vec, Pointer value);

// Αφαιρεί το τελευταίο στοιχείο του vector. Το μέγεθος του vector μικραίνει κατά 1.
// Αν το vector είναι κενό η συμπεριφορά είναι μη ορισμένη.

void vector_remove_last(Vector vec);

// Επιστρέφει την τιμή στη θέση pos του vector vec (μη ορισμένο αποτέλεσμα αν pos < 0 ή pos >= size)

Pointer vector_get_at(Vector vec, int pos);

// Αλλάζει την τιμή στη θέση pos του Vector vec σε value.
// ΔΕΝ μεταβάλλει το μέγεθος του vector, αν pos >= size το αποτέλεσμα δεν είναι ορισμένο.

void vector_set_at(Vector vec, int pos, Pointer value);

// Βρίσκει και επιστρέφει το πρώτο στοιχείο στο vector που να είναι ίσο με value
// (με βάση τη συνάρτηση compare), ή NULL αν δεν βρεθεί κανένα στοιχείο.

Pointer vector_find(Vector vec, Pointer value, CompareFunc compare);

// Αλλάζει τη συνάρτηση που καλείται σε κάθε αφαίρεση/αντικατάσταση στοιχείου σε
// destroy_value. Επιστρέφει την προηγούμενη τιμή της συνάρτησης.

DestroyFunc vector_set_destroy_value(Vector vec, DestroyFunc destroy_value);

// Ελευθερώνει όλη τη μνήμη που δεσμεύει το vector vec.
// Οποιαδήποτε λειτουργία πάνω στο vector μετά το destroy είναι μη ορισμένη.

void vector_destroy(Vector vec);


// Διάσχιση του vector ////////////////////////////////////////////////////////////
//
// Οι παρακάτω συναρτήσεις επιτρέπουν τη διάσχιση του vector μέσω κόμβων.
// Δεν είναι τόσο συχνά χρησιμοποιούμενες όσο για άλλα ADTs, γιατί μπορούμε
// εύκολα να διασχίσουμε το vector και μέσω indexes. Παραμένουν πάντως χρήσιμες,
// τόσο για ομοιομορφία με τους άλλους ADTs, αλλά και γιατί για κάποιες υλοποιήσεις
// η διάσχιση μέσω κόμβων μπορεί να είναι πιο γρήγορη.

// Οι σταθερές αυτές συμβολίζουν εικονικούς κόμβους _πριν_ τον πρώτο και _μετά_ τον τελευταίο
#define VECTOR_BOF (VectorNode)0
#define VECTOR_EOF (VectorNode)0

typedef struct vector_node* VectorNode;

// Επιστρέφουν τον πρώτο και τον τελευταίο κομβο του vector, ή VECTOR_BOF / VECTOR_EOF αντίστοιχα αν το vector είναι κενό

VectorNode vector_first(Vector vec);
VectorNode vector_last(Vector vec);

// Επιστρέφουν τον επόμενο και τον προηγούμενο κομβο του node, ή VECTOR_EOF / VECTOR_BOF
// αντίστοιχα αν ο node δεν έχει επόμενο / προηγούμενο.

VectorNode vector_next(Vector vec, VectorNode node);
VectorNode vector_previous(Vector vec, VectorNode node);

// Επιστρέφει το περιεχόμενο του κόμβου node

Pointer vector_node_value(Vector vec, VectorNode node);

// Βρίσκει το πρώτο στοιχείο στο vector που να είναι ίσο με value (με βάση τη συνάρτηση compare).
// Επιστρέφει τον κόμβο του στοιχείου, ή VECTOR_EOF αν δεν βρεθεί.

VectorNode vector_find_node(Vector vec, Pointer value, CompareFunc compare);
