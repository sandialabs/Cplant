/*
**  Project   : Heap
**
**  File(s)   : heap.c, heap.h
**
**  License   : (c)1999 Sandia National Laboratories <http://www.cs.sandia.gov>
**
**              This software was approved on Thu, 12 Aug 1999 16:04:41 -0600
**              by patents & licensing officer Fred Lewis for public release.
**
**              Although Sandia National Laboratories retains copyright
**              privledges, this software has been released to the public
**              domain and my be copied, modified, and incorperated into
**              both commercial and non-commerical software projects at no
**              charge and without prior aproval.  The only stipulation is
**              that this comment block including the liscense agreement
**              must not be removed or altered and must remain beginning at
**              line 1 of this source file.
**
**              This software is provided "as is" without any warranty of any
**              kind either expressed or implied. The licensor does not
**              warrant that the functions contained in the program will meet
**              any requirements or that the operations of the program will
**              be uninterrupted or error-free.
**
**              The licensor will not be liable for any indirect, special,
**              incidental or consequential damages, including without
**              limitation damages of for loss of profits, loss of customers,
**              loss of good will, work stoppage, data loss, computer or other
**              machine failure or malfunction, claims by any party other
**              than you, or any and all other similar damages, irrespective
**              of whether the licensor has advance notice of the possibility
**              of such damages.
**
**  Synopsis  : This software provides general heap (memory) management
**              capabilities.  It was originally architected to be a buffer
**              allocator for the caching system of ENFSD, but has been proven
**              to be an unusually good general memory allocator.  The
**              allocator provides a malloc like interface and is easily
**              adapted to an existing application.
**
**  Author(s) : Daniel R. Lehenbauer [DRL]
**              <dlehenbauer@telebot.com>
**
**  Revision  : 1.0.0 - First Release (Aug 19, 1999)
**  History
*/

/*
 *  Usage     : int heap_init() -
 *
 *                Initalizes the heap allocator, must be called before any
 *                other function.  Returns zero if successful.
 *
 *              int heap_addToPool( void* mp, size_t n ) -
 *
 *                Adds the memory pointed to by mp of size n to the free
 *                pool.  The free pool can be grown by subsequent calls to
 *                heap_addToPool().  Additional segments added to the pool
 *                need not be adjacent.  Returns zero if successful.  (You
 *                must put something into the free pool before the heap
 *                allocator will be able to allocate anything for you. ;)
 *
 *              void* heap_alloc( size_t n, int flag ) -
 *
 *                Allocates at least n bytes from the free pool and returns
 *                a pointer to the beginning of the usable memory segment.
 *                There are currently two flag options (you must specify one):
 *
 *                  HEAP_NOBLOCK - Operates in non-blocking mode like malloc().
 *
 *                  HEAP_BLOCK   - Operates in blocking mode.  If a process's
 *                                 request can not be fullfilled from the free
 *                                 pool, the process is blocked from further
 *                                 execution until the memory becomes
 *                                 available.  (process can starve).
 *
 *              void heap_free( void* mp ) -
 *
 *                Returns the memory segment pointed to by mp to the free pool.
 *                mp must be the pointer returned by heap_alloc.  You can not
 *                free part of a memory segment.
 */

#include <stddef.h>
#include "cmn.h"
#include "smp.h"

#ifndef USE_MALLOC

IDENTIFY("$Id: heap.c,v 0.6 1999/11/17 22:24:39 lee Exp $");

/*
 *  USER CONFIGURABLE OPTIONS
 *
 *  BEST_FIT         - Best fit algorithms have been shown to produce minimal
 *                     external fragmentation, and this allocator implements
 *                     best fit in such a way that when combined with
 *                     COALESCE_ON_FREE, it frequently is faster than
 *                     FIRST_FIT.  Performance without COALESCE_ON_FREE,
 *                     however, decreases sharply as the data structures
 *                     become large.
 *
 *  FIRST_FIT        - First fit algorithms typically trade some external
 *                     fragmentation for speed, however in this implementation
 *                     better performance is generally realized using the
 *                     BEST_FIT / COALESCE_ON_FREE combination.
 *
 *  COALESCE_ON_FREE - The allocator will coalesce adjacent free segments as
 *                     they are freed.  Otherwise, the allocator will wait
 *                     until it is unable to successfully satisfy an allocation
 *                     and then sweap linearly through memory coalescing all
 *                     adjacent segments.  Enabling this feature is highly
 *                     recommended for the following reasons:
 *
 *                       1.  Coalescing at free minimizes fragmentation.
 *
 *                       2.  Constant coalescing keeps the number of fragments
 *                           small, and likewise the data structures.  This
 *                           results in minimal searching on allocation and
 *                           considerable speed increase.
 *
 *                       3.  At the time of freeing, coalescing is very
 *                           convenient and very fast.
 *
 *  LOG_2_BINS       - Using LOG_2_BINS enables a (possibly) faster
 *                     implementation of computeBin() which uses 32 bins spaced
 *                     logrithmically apart.  Without this option, a 4 byte
 *                     table lookup is used (which is user configurable).  If
 *                     properly configured, the 4 byte table can provide a
 *                     little less fragmentation for a negligble performance
 *                     penalty.
 *
 *  PARANOID         - This enables some debugging features.  With this
 *                     function enabled, the boundary tags are augmented with
 *                     a magic number to give the allocator a chance at
 *                     detecting buffer overruns.  In addition, the supporting
 *                     data structures are checked for validity on every call
 *                     to heap_free() and heap_alloc().  Enabling these
 *                     features increases the overhead of the allocator, and
 *                     is not recommended for production use.  However, it can
 *                     cause memory corruption to show closer to the point of
 *                     origin and is included as a debugging aide.
 */

#define BEST_FIT
#define COALESCE_ON_FREE

/*
 *  USER CONFIGURABLE BINNING
 *
 *  The following four tables control in which bins the allocator will place
 *  memory segments.  By modifing this table it is possible to greatly change
 *  the allocator's behavior both in terms of speed and external
 *  fragmentation.  NOTE: This table has no effect if LOG_2_BINS is defined
 *  (see above).  Some things to keep in mind:
 *
 *    1. Larger sizes should always have higher numbers as the bins are
 *       searched in increasing order.
 *
 *    2. The more bins, the longer searching may take.
 *
 *    3. The smaller the range of segment sizes matched to each bin, the
 *       more exact the best-fit approximation.  This can reduce short-term
 *       fragmentation (especially among small sized segments) but reduces
 *       the effect of the prioritized allocation scheme (which effects long-
 *       term fragmentation).  The default table included employs true best-
 *       fit for segments smaller than 256B, and logarthimically spaces bins
 *       from 256B to 4GB.
 *
 *    4. NUM_BINS must be defined maximum bin number used + 1.
 */

#define NUM_BINS  (88)

#ifndef LOG_2_BINS
static unsigned char hisetTable1[] = {                       /* 0 - 255 x 1B */
   0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3, /*   0-15  */
   4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7, /*  16-31  */
   8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11, 11, 11, 11, /*  32-47  */
  12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, /*  48-63  */
  16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, /*  64-79  */
  20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, /*  80-95  */
  24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, /*  96-111 */
  28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, /* 112-127 */
  32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, /* 128-143 */
  36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, /* 144-159 */
  40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, /* 160-175 */
  44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, /* 176-191 */
  48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, /* 192-207 */
  52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, /* 208-223 */
  56, 56, 56, 56, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, /* 224-239 */
  60, 60, 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63  /* 240-255 */
};

static unsigned char hisetTable2[] = {                   /* 256 - 64k x 256B */
   0, 64, 65, 65, 66, 66, 66, 66, 67, 67, 67, 67, 67, 67, 67, 67, /*   0-15  */
  68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, /*  16-31  */
  69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, /*  32-47  */
  69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, /*  48-63  */
  70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, /*  64-79  */
  70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, /*  80-95  */
  70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, /*  96-111 */
  70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, /* 112-127 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 128-143 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 144-159 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 160-175 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 176-191 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 192-207 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 208-223 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, /* 224-239 */
  71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71  /* 240-255 */
};

static unsigned char hisetTable3[] = {                   /* 64k - 1.6M x 64k */
   0, 72, 73, 73, 74, 74, 74, 74, 75, 75, 75, 75, 75, 75, 75, 75, /*   0-15  */
  76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, /*  16-31  */
  77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, /*  32-47  */
  77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, /*  48-63  */
  78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, /*  64-79  */
  78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, /*  80-95  */
  78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, /*  96-111 */
  78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, /* 112-127 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 128-143 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 144-159 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 160-175 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 176-191 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 192-207 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 208-223 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, /* 224-239 */
  79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79  /* 240-255 */
};

static unsigned char hisetTable4[] = {                   /* 1.6M - 4G x 1.6M */
   0, 80, 81, 81, 82, 82, 82, 82, 83, 83, 83, 83, 83, 83, 83, 83, /*   0-15  */
  84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, /*  16-31  */
  85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, /*  32-47  */
  85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, /*  48-63  */
  86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, /*  64-79  */
  86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, /*  80-95  */
  86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, /*  96-111 */
  86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, /* 112-127 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 128-143 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 144-159 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 160-175 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 176-191 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 192-207 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 208-223 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, /* 224-239 */
  87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87  /* 240-255 */
};
#endif /* !defined(LOG_2_BINS) */

/*
 *  macro definitions (not user modifiable)
 */

#define ALLOC_FLAG (2)
#define ALLOC_MASK (~2)
#define TAIL_FLAG  (1)
#define TAIL_MASK  (~1)
#define BCHECK     (0xC0DEDBAD)
#define WORDSZ     (sizeof( void* ))
#define HEAD_SIZE  (sizeof( mem_head ) + sizeof( mem_tail ))
#define MIN_SIZE   (sizeof( pbNode ))
#define WORDALIGN( size ) ((size + (WORDSZ - 1)) & ~(WORDSZ-1))

#ifdef BEST_FIT
# define CMP >
#else
# define CMP <
#endif

#ifdef __GNUC__
#ifndef INLINE
# define INLINE inline
#endif
#else
# define INLINE
#endif

#ifdef LOG_2_BINS
# undef  NUM_BINS
# define NUM_BINS  (32)
#endif

#ifdef PARANOID
# define CHECKTREES                      \
    do{                                  \
      int i;                             \
				         \
      for( i = 0; i < NUM_BINS; i++ ){   \
        checkTree( &bin_[i] );           \
      }                                  \
    } while( 0 )
#else
# define CHECKTREES
#endif
  
/*
 *  type definitions
 */

typedef struct MEM_NODE{
  void*  memStart;
  void*  memStop;
  struct MEM_NODE* next;
} mem_node;

typedef struct MEM_HEAD{
  size_t size;
#ifdef PARANOID
  int    bcheck;
#endif
} mem_head;

typedef struct MEM_TAIL{
#ifdef PARANOID
  int    bcheck;
#endif
  size_t size;
} mem_tail;

typedef struct PBTREE_NODE {
  size_t order;
  struct PBTREE_NODE* parent;
  struct PBTREE_NODE* right;
  struct PBTREE_NODE* left;
} pbNode;

/*
 *  global variables
 */

static pbNode*   bin_[NUM_BINS];  /* array of bins */
static mutex_t   mutex_;          /* global mutex  */
static cond_t    free_cond;       /* condition used to notify blocking threads
				     when more memory becomes available      */
static mem_node* mem_list;        /* linked list of managed segments, used by
				     coalesce() to sweep through memory      */
static int       free_;           /* flag indicating segments have been freed,
				     used to prevent useless coalescing.     */

/*
 *  private function prototypes
 */

static pbNode* firstFit( pbNode** tree, size_t size );
static pbNode* bestFit( pbNode** tree, size_t size );
static int     checkTree( pbNode** tree );
INLINE static void  insertNode( pbNode** tree, pbNode* newNode );
INLINE static void  removeNode( pbNode** tree, pbNode* targetNode );
INLINE static void* formatPage( void* memPtr, size_t size );
INLINE static int   computeBin( unsigned long n );
INLINE static void* alloc( int bin, size_t size );
INLINE static void  addToPool( void* memPtr, size_t size, int bin );
#ifndef COALESCE_ON_FREE
INLINE static void  coalesce();
#endif

/*
 *  private function declarations
 */

static int
checkTree( pbNode** tree ){

  pbNode* current;
  int i;

  current = *tree; i = 0;
  if( current != NULL ){
    if( current->left != NULL ){
      assert( current CMP current->left );
      i = checkTree( &(current->left) );
    }
    if( current->right != NULL ){
      assert( current CMP current->right );
      return( ++i + checkTree( &(current->right) ));
    }
    return( ++i );
  }
  return( 0 );
}

INLINE static void
insertNode( pbNode** tree, pbNode* newNode ){
  
  pbNode*  current;
  pbNode*  parent;
  pbNode** parLink;
  
  parent  = NULL;
  parLink = tree;
  current = *tree;
  
  while( current != NULL ){
    if( newNode CMP current ){
      
      pbNode* temp;
      
      newNode->parent = current->parent;
      *parLink = newNode;
      if( current->left != NULL ){
        current->left->parent = newNode;
      }
      newNode->left = current->left;      
      if( current->right != NULL ){
        current->right->parent = newNode;
      }
      newNode->right = current->right;
      temp    = current;
      current = newNode;
      newNode = temp;
    }
    if( (newNode->order) < (current->order) ){
      parLink = &current->left;
      parent  = current;
      current = current->left;
    } else {
      parLink = &current->right;
      parent  = current;
      current = current->right;
    }
  }
  *parLink = newNode;
  newNode->parent = parent;
  newNode->left   = NULL;
  newNode->right  = NULL;
}

INLINE static void
removeNode( pbNode** tree, pbNode* targetNode ){
  
  pbNode*  current;
  pbNode*  parent;
  pbNode*  child;
  pbNode** parLink;
  pbNode*  temp;
  
  current = targetNode;
  parent  = targetNode->parent;
  if( parent != NULL ){
    if( parent->left == current ){
      parLink = &(parent->left);
    } else {
      parLink = &(parent->right);
    }  
  } else {
    parLink = tree;
  }
  while( -1 ){
    if( current->left != NULL ){
      if( current->right != NULL ){
        if( current->left CMP current->right ){
          child    = current->left;
          child->parent  = current->parent;
          current->left  = child->left;
          temp           = current->right;
          current->right = child->right;
          child->right   = temp;
          if( child->right != NULL ){
            child->right->parent = child;
          }
          if( current->left != NULL ){
            current->left->parent = current;
          }
          if( current->right != NULL ){
            current->right->parent = current;
          }
          *parLink = child;
          parLink  = &(child->left);
          child->left = current;
          current->parent = child;
        } else {
          child    = current->right;
          child->parent  = current->parent;
          current->right = child->right;
          temp           = current->left;
          current->left  = child->left;
          child->left    = temp;
          if( child->left != NULL ){
            child->left->parent = child;
          }
          if( current->left != NULL ){
            current->left->parent = current;
          }
          if( current->right != NULL ){
            current->right->parent = current;
          }
          *parLink = child;
          parLink  = &(child->right);
          child->right    = current;
          current->parent = child;
        }
      } else {
        *parLink = current->left;
        current->left->parent = current->parent;
        return;
      }
    } else {
      if( current->right != NULL ){
        *parLink = current->right;
        current->right->parent = current->parent;
        return;
      } else {
        break;
      }
    }
  }
  *parLink = NULL;
}

static pbNode*
firstFit( pbNode** tree, size_t size ){
  
  pbNode* temp;
  pbNode* current;
  
  current = *tree;  
  if( current != NULL ){
    if( current->order >= size ){
      return( current );
    }
    temp = firstFit( &(current->right), size );
    if( temp != NULL ){
      return( temp );
    }
    return( firstFit( &(current->left), size ));
  }
  return( NULL );
}

static pbNode*
bestFit( pbNode** tree, size_t size ){

  static int exFlag = 0;               /* set when an exact match is found */
  pbNode* current;
  pbNode* temp;
  
  current = *tree;
  if( current != NULL ){
    if( current->order == size ){
      exFlag = 1;
      temp = bestFit( &current->right, size );
      exFlag = 0;
      if( temp != NULL ) return( temp );
      return( current );
    } else if( current->order > size ){
      if( exFlag ) return( NULL );    /* do not return a non-exact match if
				         we have already found an exact one */
      temp = bestFit( &current->left, size );
      if( temp != NULL ) return( temp );
      return( current );
    } else {
      if( exFlag ) return( NULL );    /* do not return a non-exact match if
				         we have already found an exact one */
      temp = bestFit( &current->right, size );
      if( temp != NULL ) return( temp );
      return( bestFit( &current->left, size ));
    }
  }
  return( NULL );
}

#ifdef LOG_2_BINS
INLINE static int
computeBin( unsigned long n ){

  int log = 0;
  
  static unsigned log_byte[] = {
     0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  };

  if( n > 0xFFFF ){ n >>= 16; log |= 16; }
  if( n > 0xFF )  { n >>=  8; log |=  8; }
  log |= log_byte[n];
  return( log );
}

#else

INLINE static int
computeBin( unsigned long n ){

  static unsigned char hisetMaskTable[] = {
    255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  unsigned long tmp;
  int log;

  tmp = n & 0xff;
  log = hisetTable1[tmp];
  tmp = (n >> 8) & 0xff;
  log = hisetTable2[tmp] | (log & hisetMaskTable[tmp]);
  tmp = (n >> 16) & 0xff;
  log = hisetTable3[tmp] | (log & hisetMaskTable[tmp]);
  tmp = n >> 24;
  log = hisetTable4[tmp] | (log & hisetMaskTable[tmp]);
  return( log );
}
#endif /* defined(LOG_2_BINS) */

INLINE static void*
formatPage( void* memPtr, size_t size ){

  mem_head* head;
  mem_tail* tail;

  head = (mem_head*) memPtr;
  tail = (mem_tail*) (memPtr + size - sizeof( mem_tail ));

  size |= ALLOC_FLAG;
  head->size   = size;
  tail->size   = (size | TAIL_FLAG);
#ifdef PARANOID
  head->bcheck = BCHECK;
  tail->bcheck = BCHECK;
#endif /* defined(PARANOID) */
  return( memPtr + sizeof( mem_head ));
}

INLINE static void
addToPool( void* memPtr, size_t size, int bin ){

  pbNode* newNode;
  mem_tail* tail;

  newNode = (pbNode*) memPtr;
  newNode -> order = size;  
  tail = (mem_tail*) (memPtr + size - sizeof( mem_tail ));
  tail->size = (size | TAIL_FLAG);
  insertNode( &(bin_[bin]), newNode );
}

#ifndef COALESCE_ON_FREE
INLINE static void
coalesce(){

  mem_node* mn;
  void* current;
  void* regionStart;
  size_t regionLen;
  pbNode* node;
  int size, bin;

  if( !free_ ) return;
  free_ = 0;
  for( mn = mem_list; mn != NULL; mn = mn->next ){
    current = mn->memStart;
    while( current < mn->memStop ){
      node = (pbNode*) current;
      if( node->order & ALLOC_FLAG ){
	size = node->order & ALLOC_MASK;
	current += size;
      } else {
	regionStart = current;
	regionLen = 0;
	while( (current < mn->memStop) &&
	       !((node = (pbNode*) current)->order & ALLOC_FLAG) ){
	  size = node->order;
	  regionLen += size;
	  bin = computeBin( size );
	  removeNode( &(bin_[bin]), node );
	  current += node->order;
	}
	addToPool( regionStart, regionLen, computeBin( regionLen ) );
      }
    }
  }
}
#endif /* !defined( COALESCE_ON_FREE ) */

INLINE static void*
alloc( int bin, size_t size ){

  size_t  pageSize, splitSize;
  pbNode* node;
  void*   page;
  int     splitBin;

  node = NULL;
  for( ; bin < NUM_BINS; bin++ ){
    if( bin_[bin] != NULL ){
#ifdef FIRSTFIT
      node = firstFit( &(bin_[bin]), size );
#else
      node = bestFit( &(bin_[bin]), size );
#endif /* defined(FIRSTFIT) */
    }
    if( node != NULL ){
      removeNode( &(bin_[bin]), node );
      pageSize = node -> order;
      splitSize = pageSize - size;
      if( splitSize > MIN_SIZE ){
        page = formatPage( (void*)node, size );
        splitBin = computeBin( splitSize );
	addToPool( (void*)node + size, splitSize, splitBin );
      } else {
        page = formatPage( (void* )node, pageSize );
      }
      return( page );
    }
  }
  return( NULL );
}

/*
 *  public function definitions
 */

int
heap_init() {

  int i;

  mutex_init( &mutex_ );
  cond_init( &free_cond );
  mem_list = NULL;
  free_ = 0;
  for( i = 0; i < NUM_BINS; i++ ){
    bin_[i] = NULL;
  }
  return( 0 );
}

int
heap_addToPool( void* memPtr, size_t size ){

#ifdef COALESCE_ON_FREE
  mem_head* head;
  mem_tail* tail;
#else
  mem_node* node;
#endif /* defined(COALESCE_ON_FREE) */
  
  mutex_lock( &mutex_ );
#ifdef COALESCE_ON_FREE
  tail = memPtr;
  tail->size = (ALLOC_FLAG | TAIL_FLAG);
  head = memPtr + size - sizeof( mem_head );
  head->size = ALLOC_FLAG;
  memPtr += sizeof( mem_head );
  size -= (sizeof( mem_head ) + sizeof( mem_tail ));
#else
  node = memPtr;
  memPtr += sizeof( mem_node );
  size   -= sizeof( mem_node );
  node->memStart = memPtr;
  node->memStop  = memPtr + size;
  node->next = mem_list;
  mem_list = node;
#endif /* defined(COALESCE_ON_FREE) */
  addToPool( memPtr, size, computeBin( size ) );
  free_ = -1;
  cond_broadcast( &free_cond );
  mutex_unlock( &mutex_ );
  return( 0 );
}

void*
heap_alloc( size_t size, int flag ){

  int bin;
  void* mem;

  mutex_lock( &mutex_ );
  CHECKTREES;
  size = WORDALIGN( size + HEAD_SIZE );
  if( size < MIN_SIZE ){
    size = MIN_SIZE;
  }
  bin = computeBin( size );
  mem = alloc( bin, size );
  if( mem != NULL ){
    mutex_unlock( &mutex_ );
    return( mem );
  }
#ifndef COALESCE_ON_FREE
  coalesce();
  mem = alloc( bin, size );
#endif /* defined(COALESCE_ON_FREE) */
  if( (mem == NULL) && (flag & HEAP_BLOCK) ){
    while( mem == NULL ){
      mem = alloc( bin, size );
      if( mem != NULL ) break;
#ifndef COALESCE_ON_FREE
      coalesce();
      if( mem != NULL ) break;
#endif /* !defined(COALESCE_ON_FREE) */
      cond_wait( &free_cond, &mutex_ );
    }
  }
  mutex_unlock( &mutex_ );
  if ( mem == NULL ) errno = ENOMEM;
  return( mem );
}

void
heap_free( void* memPtr ){

  mem_head* head;
  mem_tail* tail;
  pbNode*   newNode;
  pbNode**  bin;
  size_t    size;
#ifdef COALESCE_ON_FREE
  mem_head* nhead;
  mem_tail* ntail;
  size_t    tsize;
#endif /* defined(COALESCE_ON_FREE) */

  mutex_lock( &mutex_ );
  CHECKTREES;
  head = (mem_head*)(memPtr - sizeof( mem_head ));
  assert( head->size & ALLOC_FLAG );
  size = (head->size) & ALLOC_MASK;
  tail = (mem_tail*)(((void*)head) + size - sizeof( mem_tail ));
  assert( head->size == (tail->size & TAIL_MASK));
#ifdef PARANOID
  assert( head->bcheck == BCHECK );
  assert( tail->bcheck == BCHECK );
#endif /* defined(PARANOID) */
#ifdef COALESCE_ON_FREE
  ntail = ((void* )head) - sizeof( mem_tail );
  if( ntail->size & TAIL_FLAG ){
    tsize = ntail->size & TAIL_MASK;            /* we found a tail tag */
    if( !(tsize & ALLOC_FLAG) ){
      head = (void* )head - tsize;
      assert( head->size == tsize );
      bin = &bin_[computeBin( tsize )];
      removeNode( bin, (pbNode* )head );
      size += tsize;
    }
  } else {
    head = (void* )head - sizeof( pbNode );     /* we have a pbNode */
    bin = &bin_[computeBin( sizeof( pbNode ))];
    removeNode( bin, (pbNode* )head );
    size += sizeof( pbNode );
  }
  nhead = ((void* )head) + size;
  if( !(nhead->size & ALLOC_FLAG) ){            /* may be either, don't care */
    tail = (void* )nhead + nhead->size - sizeof( mem_tail );
    bin = &bin_[computeBin( nhead->size )];
    removeNode( bin, (pbNode* )nhead );
    size += nhead->size;
  }
#endif /* defined(COALESCE_ON_FREE) */
  newNode = (pbNode*)(head);
  newNode->order = size;
  tail->size = (size | TAIL_FLAG);
  bin = &(bin_[computeBin( size )]);
  insertNode( bin, newNode );
  free_ = -1;
  cond_broadcast( &free_cond );
  mutex_unlock( &mutex_ );
}
#endif /* !defined(USE_MALLOC) */
