//	crm_svm_matrix_util.h - Support Vector Machine

////////////////////////////////////////////////////////////////////////
//    This code is originally copyright and owned by William
//    S. Yerazunis as file crm_neural_net.  In return for addition of 
//    significant derivative work, Jennifer Barry is hereby granted a full 
//    unlimited license to use this code, includng license to relicense under 
//    other licenses.
////////////////////////////////////////////////////////////////////////
//
// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

#ifndef __CRM_SVM_MATR_UTIL__H
#define __CRM_SVM_MATR_UTIL__H

//the libraries we'll want everywhere

#include <stdlib.h> 
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

/**********************************************************************
 *A utility library that defines some data structures, specifically an
 *expanding array and a linked list.  This library can be used with 
 *the matrix library or on its own.  The expanding array is general, but
 *the linked list would need the union expanded to be general.
 *
 *The functions in util.c are commented.  See them for details.
 *********************************************************************/

#define SVM_EPSILON 1e-10           //approximation to 0 
                                   //using doubles, 1e-10 works well enough
#define SVM_INF 1.0/SVM_EPSILON    //likewise, an approximation to infinity
#define MAX_INT_VAL 4294967295     //2^32-1
#define SVM_MAX_ALLOC_SIZE 4e8    //Maximum amount of memory to allocate in
                                  //one chunk (used for counting sort)

//these are debug settings
//the higher the setting, the more output you get
//these are cummulative so that level QP_DEBUG prints everything
//from level SOLVER_DEBUG and QP_DEBUG

#define SVM_SOLVER_DEBUG 1      //basic information about the solver.  possible
                                //to use even on runs that need to be fast.
                                //gives a general progress overview

#define QP_DEBUG 2              //basic information about the qp solver

#define QP_DEBUG_LOOP 3         //prints some information during each qp loop
                                //useful if the svm is getting stuck during a QP
                                //problem

#define QP_LINEAR_SOLVER 4      //prints some information during each cg loop
                                //useful to discover if the run goes on forever
                                //because the cg isn't converging 
                                //(usually indicates a bug in the add or remove
                                //constraint functions!)

#define QP_CONSTRAINTS 5        //prints out information about adding and 
                                //removing constraints during the qp solver
                             
#define MATR_OPS 6              //information about the matrix operations

#define MATR_OPS_MORE 7         //even more information about the matrix 
                                //operations (ie printing out intermediate 
                                //results for vector_add and dot, etc)

#define SVM_SOLVER_DEBUG_LOOP 8 //prints the matrices associated with the solver
                                //unless you're doing a small run, this isn't
                                //a feasible setting since the print operations
                                //put all the zeros in!

int SVM_DEBUG_MODE;            //the debug value
                               //if internal trace is on SVM_DEBUG_MODE = 
                               //SVM_INTERNAL_TRACE_LEVEL

//Sparse elements
typedef struct {
  unsigned int col;
  double data; //it's worth it to use doubles for higher precision.  really.
} PreciseSparseElement;

typedef struct {
  unsigned int col;
  int data; //small data!
} CompactSparseElement;

typedef union {
  PreciseSparseElement *precise;
  CompactSparseElement *compact;
} SparseElement;


//Types that can go in an expanding array
typedef union {
  int i;
  long l;
  float f;
  double d;
  PreciseSparseElement s;
} PreciseExpandingType;

typedef union {
  unsigned int i;
  CompactSparseElement s;
} CompactExpandingType;

typedef union {
  PreciseExpandingType *precise;
  CompactExpandingType *compact;
} ExpandingType;

//Expanding array struct
typedef struct {
  ExpandingType data; //Actual data
  int length,         //Current size of the array
    last_elt,         //Location of the last data
    first_elt,        //Location of the first data
    n_elts,           //Number of elements = last_elt - first_elt
    compact,          //1 for compactness
    was_mapped;       //1 if the data was mapped into memory
} ExpandingArray;

//Elements of a linked list
typedef struct precise_mythical_node {
  PreciseSparseElement data;
  struct precise_mythical_node *next, *prev;
} PreciseSparseNode;

typedef struct compact_mythical_node {
  CompactSparseElement data;
  struct compact_mythical_node *next, *prev;
} CompactSparseNode;

typedef struct {
  PreciseSparseNode *precise;
  CompactSparseNode *compact;
  int is_compact;
} SparseNode;

//Linked list struct
typedef struct {
  SparseNode head, tail;
  int compact;
  void *last_addr;
} SparseElementList;

//Expanding array functions
ExpandingArray *make_expanding_array(int init_size, int compact);
void expanding_array_insert(ExpandingType d, ExpandingArray *A);
void expanding_array_set(ExpandingType d, int c, ExpandingArray *A);
ExpandingType expanding_array_get(int c, ExpandingArray *A);
void expanding_array_trim(ExpandingArray *A);
int expanding_array_search(unsigned int c, int init, ExpandingArray *A);
int expanding_array_int_search(unsigned int c, int init, ExpandingArray *A);
int expanding_array_insert_before(ExpandingType ne, int before,
				   ExpandingArray *A);
int expanding_array_insert_after(ExpandingType ne, int after,
				  ExpandingArray *A);
void expanding_array_clear(ExpandingArray *A);
void expanding_array_remove_elt(int elt, ExpandingArray *A);
size_t expanding_array_write(ExpandingArray *A, FILE *fp);
void expanding_array_read(ExpandingArray *A, FILE *fp);
ExpandingArray *expanding_array_map(void **addr, void *last_addr);
void expanding_array_free_data(ExpandingArray *A);
void expanding_array_free(ExpandingArray *A);

//SparseElementList functions
SparseElementList *make_list(int compact);
SparseNode list_search(unsigned int c, SparseNode init, SparseElementList *l);
SparseNode list_insert_before(SparseElement newelt, SparseNode before,
			SparseElementList *l); 
SparseNode list_insert_after(SparseElement ne, SparseNode after,
		       SparseElementList *l);
void list_clear(SparseElementList *l);
void list_remove_elt(SparseElementList *l, SparseNode toremove);
int list_is_empty(SparseElementList *l);
size_t list_write(SparseElementList *l, FILE *fp);
int list_read(SparseElementList *l, FILE *fp, int n_elts);
SparseElementList *list_map(void **addr, void *last_addr, int *n_elts_ptr);
void *list_memmove(void *to, SparseElementList *from);

//Macros to deal with nodes

SparseNode make_null_node(int compact);
int null_node(SparseNode n);
double node_data(SparseNode n);
unsigned int node_col(SparseNode n);
SparseNode next_node(SparseNode n);
SparseNode prev_node(SparseNode n);
void node_set_data(SparseNode n, double d);
void node_set_col(SparseNode n, unsigned int c);
void node_free(SparseNode n);

//Having these be inline makes a huge difference in running time!
extern inline SparseNode make_null_node(int compact)
{
  SparseNode n;
  
  n.is_compact = compact;

  if (compact) {
    n.compact = NULL;
  } else {
    n.precise = NULL;
  }

  return n;
}

extern inline int null_node(SparseNode n)
{  if (n.is_compact) {
    return (n.compact == NULL);
  }
  return (n.precise == NULL);
}

extern inline double node_data(SparseNode n) 
{
  if (n.is_compact) {
    return (double)n.compact->data.data;
  }
  return n.precise->data.data;
}

extern inline unsigned int node_col(SparseNode n)
{
  if (n.is_compact) {
    return n.compact->data.col;
  }
  return n.precise->data.col;
}

extern inline SparseNode next_node(SparseNode n)
{
  SparseNode ret;
  
  ret.is_compact = n.is_compact;

  if (null_node(n)) {
    return make_null_node(n.is_compact);
  }
  if (n.is_compact) {
    ret.compact = n.compact->next;
  } else {
    ret.precise = n.precise->next;
  }
  return ret;
}

extern inline SparseNode prev_node(SparseNode n)
{
  SparseNode ret;

  ret.is_compact = n.is_compact;

  if (null_node(n)) {
    return make_null_node(n.is_compact);
  }

  if (n.is_compact) {
    ret.compact = n.compact->prev;
  } else {
    ret.precise = n.precise->prev;
  }
  return ret;
}

extern inline void node_set_data(SparseNode n, double d)
{
  if (null_node(n)) {
    return;
  }
  if (n.is_compact) {
    n.compact->data.data = (int)d;
  } else {
    n.precise->data.data = d;
  }
}

extern inline void node_set_col(SparseNode n, unsigned int c)
{
  if (null_node(n)) {
    return;
  }
  if (n.is_compact) {
    n.compact->data.col = c;
  } else {
    n.precise->data.col = c;
  }
}

extern inline void node_free(SparseNode n) {
  if (n.is_compact) {
    free(n.compact);
  } else {
    free(n.precise);
  }
}

//Comparator functions for QSort
int compact_expanding_type_int_compare(const void *a, const void *b);
int precise_sparse_element_val_compare(const void *a, const void *b);
int precise_sparse_element_col_compare(const void *a, const void *b);

#endif //crm_svm_matrix_util.h
