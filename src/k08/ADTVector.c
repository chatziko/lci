///////////////////////////////////////////////////////////
//
// Υλοποίηση του ADT Vector μέσω Dynamic Array.
//
///////////////////////////////////////////////////////////

#include <stdlib.h>
#include <assert.h>

#include "ADTVector.h"


// Το αρχικό μέγεθος που δεσμεύουμε
#define VECTOR_MIN_CAPACITY 10

// Ένα VectorNode είναι pointer σε αυτό το struct. (το struct περιέχει μόνο ένα στοιχείο, οπότε θα μπροούσαμε και να το αποφύγουμε, αλλά κάνει τον κώδικα απλούστερο)
struct vector_node {
	Pointer value;				// Η τιμή του κόμβου.
};

// Ενα Vector είναι pointer σε αυτό το struct
struct vector {
	VectorNode array;			// Τα δεδομένα, πίνακας από struct vector_node
	int size;					// Πόσα στοιχεία έχουμε προσθέσει
	int capacity;				// Πόσο χώρο έχουμε δεσμεύσει (το μέγεθος του array). Πάντα capacity >= size, αλλά μπορεί να έχουμε
	DestroyFunc destroy_value;	// Συνάρτηση που καταστρέφει ένα στοιχείο του vector.
};


Vector vector_create(int size, DestroyFunc destroy_value) {
	// Δημιουργία του struct
	Vector vec = malloc(sizeof(*vec));

	vec->size = size;
	vec->destroy_value = destroy_value;

	// Δέσμευση μνήμης για τον πίνακα. Αρχικά το vector περιέχει size
	// μη-αρχικοποιημένα στοιχεία, αλλά εμείς δεσμεύουμε xώρο για τουλάχιστον
	// VECTOR_MIN_CAPACITY για να αποφύγουμε τα πολλαπλά resizes.
	//
	vec->capacity = size < VECTOR_MIN_CAPACITY ? VECTOR_MIN_CAPACITY : size;
	vec->array = calloc(vec->capacity, sizeof(*vec->array));		// αρχικοποίηση σε 0 (NULL)

	return vec;
}

int vector_size(Vector vec) {
	return vec->size;
}

Pointer vector_get_at(Vector vec, int pos) {
	assert(pos >= 0 && pos < vec->size);	// LCOV_EXCL_LINE (αγνοούμε το branch από τα coverage reports, είναι δύσκολο να τεστάρουμε το false γιατί θα κρασάρει το test)

	return vec->array[pos].value;
}

void vector_set_at(Vector vec, int pos, Pointer value) {
	assert(pos >= 0 && pos < vec->size);	// LCOV_EXCL_LINE

	// Αν υπάρχει συνάρτηση destroy_value, την καλούμε για το στοιχείο που αντικαθίσταται
	if (value != vec->array[pos].value && vec->destroy_value != NULL)
		vec->destroy_value(vec->array[pos].value);

	vec->array[pos].value = value;
}

void vector_insert_last(Vector vec, Pointer value) {
	// Μεγαλώνουμε τον πίνακα (αν χρειαστεί), ώστε να χωράει τουλάχιστον size στοιχεία
	// Διπλασιάζουμε κάθε φορά το capacity (σημαντικό για την πολυπλοκότητα!)
	if (vec->capacity == vec->size) {
		// Προσοχή: δεν πρέπει να κάνουμε free τον παλιό pointer, το κάνει η realloc
		vec->capacity *= 2;
		vec->array = realloc(vec->array, vec->capacity * sizeof(*vec->array));
		assert(vec->array);
	}

	// Μεγαλώνουμε τον πίνακα και προσθέτουμε το στοιχείο
	vec->array[vec->size].value = value;
	vec->size++;
}

void vector_remove_last(Vector vec) {
	assert(vec->size != 0);		// LCOV_EXCL_LINE

	// Αν υπάρχει συνάρτηση destroy_value, την καλούμε για το στοιχείο που αφαιρείται
	if (vec->destroy_value != NULL)
		vec->destroy_value(vec->array[vec->size - 1].value);

	// Αφαιρούμε στοιχείο οπότε ο πίνακας μικραίνει
	vec->size--;

	// Μικραίνουμε τον πίνακα αν χρειαστεί, ώστε να μην υπάρχει υπερβολική σπατάλη χώρου.
	// Για την πολυπλοκότητα είναι σημαντικό να μειώνουμε το μέγεθος στο μισό, και μόνο
	// αν το capacity είναι τετραπλάσιο του size (δηλαδή το 75% του πίνακα είναι άδειος).
	//
	if (vec->capacity > vec->size * 4 && vec->capacity > 2*VECTOR_MIN_CAPACITY) {
		vec->capacity /= 2;
		vec->array = realloc(vec->array, vec->capacity * sizeof(*vec->array));
	}
}

Pointer vector_find(Vector vec, Pointer value, CompareFunc compare) {
	// Διάσχιση του vector
	for (int i = 0; i < vec->size; i++)
		if (compare(vec->array[i].value, value) == 0)
			return vec->array[i].value;		// βρέθηκε

	return NULL;				// δεν υπάρχει
}

DestroyFunc vector_set_destroy_value(Vector vec, DestroyFunc destroy_value) {
	DestroyFunc old = vec->destroy_value;
	vec->destroy_value = destroy_value;
	return old;
}

void vector_destroy(Vector vec) {
	// Αν υπάρχει συνάρτηση destroy_value, την καλούμε για όλα τα στοιχεία
	if (vec->destroy_value != NULL)
		for (int i = 0; i < vec->size; i++)
			vec->destroy_value(vec->array[i].value);

	// Πρέπει να κάνουμε free τόσο τον πίνακα όσο και το struct!
	free(vec->array);
	free(vec);			// τελευταίο το vec!
}


// Συναρτήσεις για διάσχιση μέσω node /////////////////////////////////////////////////////

VectorNode vector_first(Vector vec) {
	if (vec->size == 0)
		return VECTOR_BOF;
	else	
		return &vec->array[0];
}

VectorNode vector_last(Vector vec) {
	if (vec->size == 0)
		return VECTOR_EOF;
	else
		return &vec->array[vec->size-1];
}

VectorNode vector_next(Vector vec, VectorNode node) {
	if (node == &vec->array[vec->size-1])
		return VECTOR_EOF;
	else
		return node + 1;
}

VectorNode vector_previous(Vector vec, VectorNode node) {
	if (node == &vec->array[0])
		return VECTOR_EOF;
	else
		return node - 1;
}

Pointer vector_node_value(Vector vec, VectorNode node) {
	return node->value;
}

VectorNode vector_find_node(Vector vec, Pointer value, CompareFunc compare) {
	// Διάσχιση του vector
	for (int i = 0; i < vec->size; i++)
		if (compare(vec->array[i].value, value) == 0)
			return &vec->array[i];		// βρέθηκε

	return VECTOR_EOF;				// δεν υπάρχει
}


static void swap(VectorNode a, VectorNode b) {
	Pointer temp = a->value;
	a->value = b->value;
	b->value = temp;
}

static int partition (Vector vec, CompareFunc compare, int low, int high) {
	// select random pivot, set to last position
    swap(&vec->array[high], &vec->array[low + rand()%(high - low + 1)]);
    Pointer pivot = vec->array[high].value;
    int i = (low - 1);
 
    for (int j = low; j <= high; j++) {
        if (compare(vec->array[j].value, pivot) < 0) {
            i++;
            swap(&vec->array[i], &vec->array[j]);
        }
    }
    swap(&vec->array[i + 1], &vec->array[high]);
    return i + 1;
}
 
static void quick_sort(Vector vec, CompareFunc compare, int low, int high) {
    if (low < high) {
        int pi = partition(vec, compare, low, high);
 
        quick_sort(vec, compare, low, pi - 1);
        quick_sort(vec, compare, pi + 1, high);
    }
}

void vector_sort(Vector vec, CompareFunc compare) {
	quick_sort(vec, compare, 0, vec->size-1);
}