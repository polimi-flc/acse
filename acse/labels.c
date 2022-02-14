/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * labels.c
 * Formal Languages & Compilers Machine, 2007/2008
 * 
 */

#include <assert.h>
#include <ctype.h>
#include "errors.h"
#include "labels.h"
#include "list.h"


struct t_axe_label_manager
{
   t_list *labels;
   unsigned int current_label_ID;
   t_axe_label *label_to_assign;
};

void setRawLabelName(t_axe_label_manager *lmanager, t_axe_label *label,
      const char *finalName);

t_axe_label * initializeLabel(int value, int global)
{
   t_axe_label *result;

   /* create an instance of t_axe_label */
   result = (t_axe_label *)malloc(sizeof(t_axe_label));
   if (result == NULL)
      fatalError(AXE_OUT_OF_MEMORY);

   /* initialize the internal value of `result' */
   result->labelID = value;
   result->name = NULL;
   result->global = global;
   result->isAlias = 0;

   /* return the just initialized new instance of `t_axe_label' */
   return result;
}

void finalizeLabel(t_axe_label *lab)
{
   if (lab->name)
      free(lab->name);
   free(lab);
}

int isAssigningLabel(t_axe_label_manager *lmanager)
{
   /* preconditions: lmanager must be different from NULL */
   assert(lmanager != NULL);

   if (lmanager->label_to_assign != NULL)
   {
      return 1;
   }

   return 0;
}

int compareLabels(t_axe_label *labelA, t_axe_label *labelB)
{
   if ( (labelA == NULL) || (labelB == NULL) )
      return 0;

   if (labelA->labelID == labelB->labelID)
      return 1;
   return 0;
}

/* reserve a new label identifier and return the identifier to the caller */
t_axe_label * newLabelID(t_axe_label_manager *lmanager, int global)
{
   t_axe_label *result;

   /* preconditions: lmanager must be different from NULL */
   assert(lmanager != NULL);
   
   /* initialize a new label */
   result = initializeLabel(lmanager->current_label_ID, global);

   /* update the value of `current_label_ID' */
   lmanager->current_label_ID++;
   
   /* tests if an out of memory occurred */
   if (result == NULL)
      return NULL;

   /* add the new label to the list of labels */
   lmanager->labels = addElement(lmanager->labels, result, -1);

   /* return the new label */
   return result;
}

/* initialize the memory structures for the label manager */
t_axe_label_manager * initializeLabelManager()
{
   t_axe_label_manager *result;

   /* create an instance of `t_axe_label_manager' */
   result = (t_axe_label_manager *)
         malloc (sizeof(t_axe_label_manager));
   if (result == NULL)
      fatalError(AXE_OUT_OF_MEMORY);

   /* initialize the new instance */
   result->labels = NULL;
   result->current_label_ID = 0;
   result->label_to_assign = NULL;

   return result;
}

/* finalize an instance of `t_axe_label_manager' */
void finalizeLabelManager(t_axe_label_manager *lmanager)
{
   t_list *current_element;
   t_axe_label *current_label;
   
   /* preconditions */
   if (lmanager == NULL)
      return;

   /* initialize `current_element' to the head of the list
    * of labels */
   current_element = lmanager->labels;

   while (current_element != NULL)
   {
      /* retrieve the current label */
      current_label = (t_axe_label *) current_element->data;
      assert(current_label != NULL);

      /* free the memory associated with the current label */
      finalizeLabel(current_label);

      /* fetch the next label */
      current_element = current_element->next;
   }

   /* free the memory associated to the list of labels */
   freeList(lmanager->labels);

   free(lmanager);
}

/* assign the given label identifier to the next instruction. Returns
 * NULL if an error occurred; otherwise the assigned label */
t_axe_label * assignLabelID(t_axe_label_manager *lmanager, t_axe_label *label)
{
   /* precondition: lmanager must be different from NULL */
   assert(lmanager != NULL);
   assert(label != NULL);
   assert(label->labelID < lmanager->current_label_ID);

   /* test if the next instruction already has a label */
   if (lmanager->label_to_assign != NULL)
   {
      /* It does: transform the label being assigned into an alias of the
       * label of the next instruction's label
       * All label aliases have the same ID and name. */

      /* Decide the name of the alias. If only one label has a name, that name
       * wins. Otherwise the name of the label with the lowest ID wins */
      char *name = lmanager->label_to_assign->name;
      if (!name || 
            (label->labelID && 
            label->labelID < lmanager->label_to_assign->labelID))
         name = label->name;
      /* copy the name */
      if (name)
         name = strdup(name);
      
      /* Change ID and name */
      label->labelID = (lmanager->label_to_assign)->labelID;
      setRawLabelName(lmanager, label, name);

      /* Promote both labels to global if at least one is global */
      if (label->global)
         lmanager->label_to_assign->global = 1;
      else if (lmanager->label_to_assign->global)
         label->global = 1;

      /* mark the label as an alias */
      label->isAlias = 1;

      free(name);
   }
   else
      lmanager->label_to_assign = label;

   /* all went good */
   return label;
}

t_axe_label * getLastPendingLabel(t_axe_label_manager *lmanager)
{
   t_axe_label *result;
   
   /* precondition: lmanager must be different from NULL */
   assert(lmanager != NULL);

   /* the label that must be returned (can be a NULL pointer) */
   result = lmanager->label_to_assign;

   /* update the value of `lmanager->label_to_assign' */
   lmanager->label_to_assign = NULL;

   /* return the label */
   return result;
}

int getLabelCount(t_axe_label_manager *lmanager)
{
   if (lmanager == NULL)
      return 0;

   if (lmanager->labels == NULL)
      return 0;

   /* postconditions */
   return getLength(lmanager->labels);
}

t_axe_label *enumLabels(t_axe_label_manager *lmanager, void **state)
{
   t_list *lastItem, *nextItem;
   assert(state != NULL);
   
   lastItem = (t_list *)(*state);
   if (lastItem == NULL) {
      nextItem = lmanager->labels;
   } else {
      nextItem = lastItem->next;
   }

   /* Skip aliased labels */
   while (nextItem != NULL && ((t_axe_label *)nextItem->data)->isAlias) {
      nextItem = nextItem->next;
   }

   *state = (void *)nextItem;
   if (nextItem == NULL)
      return NULL;
   return (t_axe_label *)nextItem->data;
}

/* Set a name to a label without resolving duplicates */
void setRawLabelName(t_axe_label_manager *lmanager, t_axe_label *label,
      const char *finalName)
{
   t_list *i;

   /* check the entire list of labels because there might be two
    * label objects with the same ID and they need to be kept in sync */
   for (i = lmanager->labels; i != NULL; i = i->next) {
      t_axe_label *thisLab = i->data;

      if (thisLab->labelID == label->labelID) {
         /* found! remove old name */
         free(thisLab->name);
         /* change to new name */
         if (finalName)
            thisLab->name = strdup(finalName);
         else
            thisLab->name = NULL;
      }
   }
}

void setLabelName(t_axe_label_manager *lmanager, t_axe_label *label,
      const char *name)
{
   int serial = -1, ok, allocatedSpace;
   char *sanitizedName, *finalName, *dstp;
   const char *srcp;

   /* remove all non a-zA-Z0-9_ characters */
   sanitizedName = calloc(strlen(name)+1, sizeof(char));
   srcp = name;
   for (dstp = sanitizedName; *srcp; srcp++) {
      if (*srcp == '_' || isalnum(*srcp))
         *dstp++ = *srcp;
   }

   /* append a sequential number to disambiguate labels with the same name */
   allocatedSpace = strlen(sanitizedName)+24;
   finalName = calloc(allocatedSpace, sizeof(char));
   snprintf(finalName, allocatedSpace, "%s", sanitizedName);
   do {
      t_list *i;
      ok = 1;
      for (i = lmanager->labels; i != NULL; i = i->next) {
         t_axe_label *thisLab = i->data;
         char *thisLabName;
         int difference;

         if (thisLab->labelID == label->labelID)
            continue;
         
         thisLabName = getLabelName(thisLab);
         difference = strcmp(finalName, thisLabName);
         free(thisLabName);

         if (difference == 0) {
            ok = 0;
            snprintf(finalName, allocatedSpace, "%s.%d", sanitizedName, ++serial);
            break;
         }
      }
   } while (!ok);

   free(sanitizedName);
   setRawLabelName(lmanager, label, finalName);
   free(finalName);
}

char *getLabelName(t_axe_label *label)
{
   char *buf;

   if (label->name) {
      buf = strdup(label->name);
   } else {
      buf = calloc(24, sizeof(char));
      snprintf(buf, 24, "l%d", label->labelID);
   }

   return buf;
}