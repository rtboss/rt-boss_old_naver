/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                             RT-BOSS (Memory)                                *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Mem.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _MEM_MAGIC_CODE_        (boss_uptr_t)0x93467878

typedef struct _boss_mem_blk_struct {     /* Memory Block Head Tyep */
  boss_uptr_t               in_use;
  boss_uptr_t               size;

  struct _boss_mem_blk_struct *prev;
  struct _boss_mem_blk_struct *next;
} _boss_mem_blk_t;


#ifdef _BOSS_MEM_INFO_
typedef struct _boss_mem_info_struct {
  boss_uptr_t       used_size;
  boss_uptr_t       used_peak;
  
  boss_uptr_t       blk_div;
  _boss_mem_blk_t   *blk_first;
} _boss_mem_info_t;
#endif

/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
boss_mem_align_t _memory_pool[_BOSS_MEM_POOL_SIZE / sizeof(boss_mem_align_t)];

#ifdef _BOSS_MEM_INFO_
_boss_mem_info_t _boss_mem_info;
#endif

#define _ALIGN_SIZE                 (boss_uptr_t)(sizeof(boss_mem_align_t))

#define _MEM_POOL_START   (void *)_memory_pool
#define _MEM_POOL_END     (void *)((boss_uptr_t)_memory_pool+sizeof(_memory_pool))
/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/

/*===========================================================================
    _   B O S S _ M E M _ P O O L _ I N I T
---------------------------------------------------------------------------*/
static void _Boss_mem_pool_init(void)
{
  _boss_mem_blk_t *first_blk = (_boss_mem_blk_t *)_memory_pool;

  BOSS_ASSERT( ((boss_uptr_t)_memory_pool & (_ALIGN_SIZE - 1)) == 0 );
  BOSS_ASSERT( (sizeof(_memory_pool) & (_ALIGN_SIZE - 1)) == 0 );

  BOSS_ASSERT( first_blk->size == 0 );
  
  first_blk->in_use = _BOSS_FALSE;
  first_blk->size   = sizeof(_memory_pool);
  
  first_blk->prev   = _BOSS_NULL;
  first_blk->next   = _BOSS_NULL;
  
  #ifdef _BOSS_MEM_INFO_
  _boss_mem_info.used_size  = 0;
  _boss_mem_info.used_peak  = 0;
  _boss_mem_info.blk_div    = 0;
  _boss_mem_info.blk_first  = first_blk;
  #endif
}


/*===========================================================================
    B O S S _ M A L L O C
---------------------------------------------------------------------------*/
void *Boss_malloc(boss_uptr_t size)
{
  static boss_reg_t _mem_pool_init = _BOSS_FALSE;
  
  _boss_mem_blk_t *p_alloc;

  if(_mem_pool_init == _BOSS_FALSE)
  {
    _mem_pool_init = _BOSS_TRUE;
    _Boss_mem_pool_init();                              /* Memory Pool init */
  }
                                                        /* �޸� ũ�� ���� */
  size = size + ( sizeof(_boss_mem_blk_t) + (_ALIGN_SIZE-1) );
  size = size & ~(_ALIGN_SIZE-1);
  
  p_alloc = (_boss_mem_blk_t *)_memory_pool;
  //BOSS_ASSERT(p_alloc->size > size);  /* First �Ҵ� */
  
  BOSS_IRQ_DISABLE();                         /* �Ҵ� ������ ���� ã�´�. */
  while( p_alloc && ((p_alloc->size < size) || (p_alloc->in_use != _BOSS_FALSE)) )
  {
    p_alloc = p_alloc->next;
  }
  
  if(p_alloc != _BOSS_NULL)
  {
    if( p_alloc->size > (size + sizeof(_boss_mem_blk_t) + _ALIGN_SIZE) )
    {                                             /* �޸� �� ���� �� */
      _boss_mem_blk_t *p_prev = p_alloc;
      
      p_prev->size = p_prev->size - size;
                                                      /* �� ���� */
      p_alloc = (_boss_mem_blk_t *)((boss_uptr_t)p_prev + p_prev->size);
      p_alloc->size = size;
      
      p_alloc->prev = p_prev;
      p_alloc->next = p_prev->next;
      p_prev->next = p_alloc;
      
      if(p_alloc->next != _BOSS_NULL) {
        p_alloc->next->prev = p_alloc;
      }

      #ifdef _BOSS_MEM_INFO_
      _boss_mem_info.blk_div++;
      #endif
    }
    
    p_alloc->in_use = _MEM_MAGIC_CODE_;               /* �޸� �Ҵ� */
    
    #ifdef _BOSS_MEM_INFO_
    _boss_mem_info.used_size += p_alloc->size;
    if( _boss_mem_info.used_size > _boss_mem_info.used_peak ) {
      _boss_mem_info.used_peak = _boss_mem_info.used_size;
    }
    #endif
    
    p_alloc = (_boss_mem_blk_t *)( (boss_uptr_t)p_alloc + sizeof(_boss_mem_blk_t) );
  }
  BOSS_IRQ_RESTORE();

  BOSS_ASSERT(p_alloc != _BOSS_NULL);             /* �޸� FULL */

  return (void *)p_alloc;
}


/*===========================================================================
    B O S S _ M F R E E
---------------------------------------------------------------------------*/
void Boss_mfree(void *p)
{
  _boss_mem_blk_t *p_prev;
  _boss_mem_blk_t *p_free;
  _boss_mem_blk_t *p_next;
  
  p_free = (_boss_mem_blk_t *)( (boss_uptr_t)p - sizeof(_boss_mem_blk_t) );
  
  BOSS_ASSERT( (_MEM_POOL_START < p) && (p < _MEM_POOL_END) );
  BOSS_ASSERT( (p_free->prev != _BOSS_NULL) ? (p_free->prev->next == p_free)
                                            : (_MEM_POOL_START == p_free) );
  BOSS_ASSERT( (p_free->next != _BOSS_NULL) ? (p_free->next->prev == p_free)
        : ((boss_uptr_t)p_free + p_free->size == (boss_uptr_t)_MEM_POOL_END) );
  BOSS_ASSERT( p_free->in_use == _MEM_MAGIC_CODE_ );
  
  BOSS_IRQ_DISABLE();
  #ifdef _BOSS_MEM_INFO_
  _boss_mem_info.used_size -= p_free->size;
  #endif
  
  p_free->in_use = _BOSS_FALSE;               /* �޸� �� ���� */

  p_next = p_free->next;
  if( (p_next != _BOSS_NULL) && (p_next->in_use == _BOSS_FALSE) ) /* ���� (��) */
  {
    BOSS_ASSERT( ((boss_uptr_t)p_next - (boss_uptr_t)p_free) == p_free->size);
    
    p_free->size = p_free->size + p_next->size;
    p_free->next = p_next->next;

    if(p_next->next != _BOSS_NULL) {
      p_next->next->prev = p_free;
    }
    
    #ifdef _BOSS_MEM_INFO_
    p_next->size = 0;
    p_next->next = _BOSS_NULL;
    p_next->prev = _BOSS_NULL;
    
    _boss_mem_info.blk_div--;
    #endif
  }

  p_prev = p_free->prev;
  if( (p_prev != _BOSS_NULL) && (p_prev->in_use == _BOSS_FALSE) ) /* (��) ���� */
  {
    BOSS_ASSERT( ((boss_uptr_t)p_free - (boss_uptr_t)p_prev) == p_prev->size );
    
    p_prev->size = p_prev->size + p_free->size;
    p_prev->next = p_free->next;

    if(p_free->next != _BOSS_NULL) {
      p_free->next->prev = p_prev;
    }
    
    #ifdef _BOSS_MEM_INFO_
    p_free->size = 0;
    p_free->next = _BOSS_NULL;
    p_free->prev = _BOSS_NULL;
    
    _boss_mem_info.blk_div--;
    #endif
  }
  BOSS_IRQ_RESTORE();
}


#ifdef _BOSS_MEM_INFO_
/*===========================================================================*/
/*                              MEM INFO & DEBUG                             */
/*---------------------------------------------------------------------------*/

/*===========================================================================
    _   B O S S _ M E M _ I N F O _ F I R S T _ B L K _ F R E E _ S I Z E
---------------------------------------------------------------------------*/
boss_uptr_t _Boss_mem_info_first_blk_free_size(void)
{
  _boss_mem_blk_t *p_first = (_boss_mem_blk_t *)_memory_pool;
  
  return ( (p_first->in_use == _BOSS_FALSE) ? (p_first->size) : 0 );
}


/*===========================================================================
    _   B O S S _ M E M _ I N F O _ T O T A L
---------------------------------------------------------------------------*/
boss_uptr_t _Boss_mem_info_total(void)
{
  return sizeof(_memory_pool);
}


/*===========================================================================
    _   B O S S _ M E M _ I N F O _ F R E E
---------------------------------------------------------------------------*/
boss_uptr_t _Boss_mem_info_free(void)
{
  return (sizeof(_memory_pool) - _boss_mem_info.used_size);
}


/*===========================================================================
    _   B O S S _ M E M _ I N F O _ U S E D
---------------------------------------------------------------------------*/
boss_uptr_t _Boss_mem_info_used(void)
{
  return _boss_mem_info.used_size;
}


/*===========================================================================
    _   B O S S _ M E M _ I N F O _ P E A K
---------------------------------------------------------------------------*/
boss_uptr_t _Boss_mem_info_peak(void)
{
  return _boss_mem_info.used_peak;
}


/*===========================================================================
    _   B O S S _ M E M _ I N F O _ B L K _ D I V
---------------------------------------------------------------------------*/
boss_uptr_t _Boss_mem_info_blk_div(void)
{
  return _boss_mem_info.blk_div;
}
#endif /* _BOSS_MEM_INFO_ */
