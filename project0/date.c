#include "date.h"

struct date {
	char sig[8];
};

/*
 * date_create creates a Date structure from `datestr`
 * `datestr' is expected to be of the form "dd/mm/yyyy"
 * returns pointer to Date structure if successful,
 *         NULL if not (syntax error)
 */
Date *date_create(char *datestr){
	// Allocate memory for the new date
	Date *newdate = (Date *) malloc(sizeof(Date));

	// Ensure the string is valid and memory was allocated
	if ( (datestr[10] != 0)
	     | (datestr[2] != '/')
	     | (datestr[5] != '/')
	     | (newdate == 0) 
	   )
		return 0;

	// Store the date information as a series of char-digits
	// by order of significance
	// DEVNOTE: Replace with memcpy and pointers if there's time.
	newdate->sig[0] = datestr[6];	// year
	newdate->sig[1] = datestr[7];
	newdate->sig[2] = datestr[8];
	newdate->sig[3] = datestr[9];
	newdate->sig[4] = datestr[3];	// month
	newdate->sig[5] = datestr[4];
	newdate->sig[6] = datestr[0];	// day
	newdate->sig[7] = datestr[1];
	// Return
	return newdate;
}

/*
 * date_duplicate creates a duplicate of `d'
 * returns pointer to new Date structure if successful,
 *         NULL if not (memory allocation failure)
 */
Date *date_duplicate(Date *d){
	Date *newdate = (Date *) malloc(sizeof(Date));

	// Ensure memory was allocated
	if (newdate == 0)
		return 0;

	// Copy the data over
	memcpy(newdate->sig, d->sig, 8);

	// Return
	return newdate;
}

/*
 * date_compare compares two dates, returning <0, 0, >0 if
 * date1<date2, date1==date2, date1>date2, respectively
 */
int date_compare(Date *date1, Date *date2){
	return strcmp(date1->sig, date2->sig);}

/*
 * date_destroy returns any storage associated with `d' to the system
 */
void date_destroy(Date *d){
	free(d);
}

