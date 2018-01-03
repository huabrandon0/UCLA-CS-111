#include "SortedList.h"
#include <string.h>
#include <sched.h>
#include <stdio.h>

/* Note: the following code assumes the tail's next is NULL. */

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert (SortedList_t *list, SortedListElement_t *element)
{
  SortedListElement_t *list_i = list;
  while (list_i->next != NULL)
    {
      if (strcmp((list_i->next)->key, element->key) >= 0)
	break;
      list_i = list_i->next;
    }
  
  if (opt_yield & INSERT_YIELD)
    sched_yield();
  
  //Insert element after list_i.
  element->prev = list_i;
  element->next = list_i->next;
  if (list_i->next != NULL)
    (list_i->next)->prev = element;
  list_i->next = element;

  return;
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete (SortedListElement_t *element)
{
  if (element->next == NULL && (element->prev)->next == element)
    {
      if (opt_yield & DELETE_YIELD)
	sched_yield();
      
      (element->prev)->next = NULL;
      return 0;
    }
  if ((element->prev)->next == element && (element->next)->prev == element)
    {
      if (opt_yield & DELETE_YIELD)
	sched_yield();
      
      (element->prev)->next = element->next;
      (element->next)->prev = element->prev;
      return 0;
    }
  else
    return 1;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
  SortedListElement_t *list_i = list;
  while (list_i->next != NULL)
    {
      if (strcmp((list_i->next)->key, key) == 0)
	break;
      
      list_i = list_i->next;
    }
  
  if (opt_yield & LOOKUP_YIELD)
    sched_yield();
  
  return list_i->next;
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumerating list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
  int len = 0;

  SortedListElement_t *list_i = list;
  while (list_i->next != NULL)
    {
      if ((list_i->next)->prev != list_i)
	return -1;
      
      len++;
      list_i = list_i->next;

      if ((list_i->prev)->next != list_i)
	return -1;
      
    }

  if (opt_yield & LOOKUP_YIELD)
    sched_yield();
  
  return len;
}
