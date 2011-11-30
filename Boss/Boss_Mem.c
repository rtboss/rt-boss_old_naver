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
#include "Boss_Kernel.h"
#include "Boss_Mem.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _MEMORY_ALIGNMENT         sizeof(boss_uptr_t)
#define _MEMORY_MAGIC_CODE        ((void *)0x78787878)


typedef struct _mem_block_struct {
  struct _mem_block_struct  *next;
  boss_uptr_t               size;
} _mem_block_t;


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
/*static*/ _mem_block_t *_m_free_head;  /* 사용하지 않는 메모리 블럭 리스트 */
/*static*/ boss_uptr_t  _free_bytes;  /* 사용가능한 메모리 크기 (디버그 용) */

boss_uptr_t _memory_pool[ _BOSS_MEM_POOL_SIZE / sizeof(boss_uptr_t) ];

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/

/*===========================================================================
    _   B O S S _ M E M _ I N I T
---------------------------------------------------------------------------*/
static void _Boss_mem_init(void)
{
  BOSS_IRQ_LOCK();
  _m_free_head = (_mem_block_t *)_memory_pool;
  _m_free_head->next = _BOSS_NULL;
  _m_free_head->size = sizeof(_memory_pool);

  _free_bytes = sizeof(_memory_pool);
  BOSS_IRQ_FREE();
}


/*===========================================================================
    B O S S _ M E M _ A L L O C
---------------------------------------------------------------------------*/
void *Boss_mem_alloc(boss_uptr_t size)
{
  static boss_byte_t _mem_init = _BOSS_FALSE;
  
  _mem_block_t *p_prev;
  _mem_block_t *p_alloc;

  if(_mem_init == _BOSS_FALSE)
  {    
    _mem_init = _BOSS_TRUE;
    _Boss_mem_init();
  }
  
  /* 메모리 크기 정렬 (할당하는 주소 정렬) */
  size = size + sizeof(_mem_block_t) + (_MEMORY_ALIGNMENT - 1);
  size = size &  ~( (boss_uptr_t)(_MEMORY_ALIGNMENT - 1) );
  
  p_prev = _BOSS_NULL;
  
  BOSS_IRQ_LOCK();
  p_alloc = _m_free_head;
  while( (p_alloc->size < size) && (p_alloc != _BOSS_NULL) )
  {   
    p_prev = p_alloc;
    p_alloc = p_alloc->next;
  }

  if(p_alloc != _BOSS_NULL)
  {
    _mem_block_t *p_next;
    
    if( p_alloc->size > (size + sizeof(_mem_block_t) + _MEMORY_ALIGNMENT) )
    {                                         /* 메모리 블럭 분할 할당 */
      p_next = (_mem_block_t *)( (boss_uptr_t)p_alloc + size );
      
      p_next->next = p_alloc->next;
      p_next->size = p_alloc->size - size;
      
      p_alloc->size = size;
    }
    else
    {
      p_next = p_alloc->next;                /* 메모리 블럭 전체 할당 */
    }
    
    if( p_prev == _BOSS_NULL )
    {
      _m_free_head = p_next;
    }
    else
    {
      p_prev->next = p_next;
    }
    
    _free_bytes = _free_bytes - p_alloc->size;
    
    p_alloc->next = _MEMORY_MAGIC_CODE;
    p_alloc = (_mem_block_t *)( ((boss_uptr_t)p_alloc) + sizeof(_mem_block_t) );
  }
  BOSS_IRQ_FREE();
  
  BOSS_ASSERT(p_alloc != _BOSS_NULL);      /* 메모리를 할당 할수 없음 */
  
  return p_alloc;
}


/*===========================================================================
    B O S S _ M E M _ F R E E
---------------------------------------------------------------------------*/
void Boss_mem_free(void *p)
{
  _mem_block_t *p_front;
  _mem_block_t *p_free;
  _mem_block_t *p_rear;

  /* 메모리 영역 확인 */
  BOSS_ASSERT((boss_uptr_t)p > (boss_uptr_t)_memory_pool); 
  BOSS_ASSERT((boss_uptr_t)p < ((boss_uptr_t)_memory_pool + sizeof(_memory_pool)));
 
  p_free = (_mem_block_t *)( ((boss_uptr_t)p) - sizeof(_mem_block_t) );

  BOSS_ASSERT(p_free->next == _MEMORY_MAGIC_CODE);
  
  p_front = _BOSS_NULL;

  BOSS_IRQ_LOCK();
  _free_bytes = _free_bytes + p_free->size;
  
  p_rear  = _m_free_head;
  while( (p_rear != _BOSS_NULL) && (p_rear < p_free) )
  {
    p_front = p_rear;
    p_rear  = p_rear->next;
  }
  
  /* 사용하지 않는 메모리 블럭 리스트 추가 및 병합 (앞 / 뒤) */
  if( p_front == _BOSS_NULL )
  {
    _m_free_head = p_free;
  }
  else
  {
    if( ((boss_uptr_t)p_front + p_front->size) == (boss_uptr_t)p_free )
    {
      p_front->size = p_front->size + p_free->size;  /* (앞) 병합  */
      p_free = p_front;
    }
    else
    {
      p_front->next = p_free;
    }
  }
  
  if((p_rear != _BOSS_NULL) && (((boss_uptr_t)p_free + p_free->size) == (boss_uptr_t)p_rear))
  {
    p_free->size = p_free->size + p_rear->size;
    p_free->next = p_rear->next;                   /* 병합 (뒤)*/
  }
  else
  {
    p_free->next = p_rear;
  }
  BOSS_IRQ_FREE();
}

