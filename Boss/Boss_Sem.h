#ifndef _BOSS_SEM_H_
#define _BOSS_SEM_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                            RT-BOSS (Semaphore)                              *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Kernel.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
typedef struct boss_sem_link_struct {
  struct boss_sem_link_struct   *prev;
  struct boss_sem_link_struct   *next;

  boss_tcb_t                    *p_tcb;
} boss_sem_link_t;


typedef struct boss_sem_struct {
  boss_byte_t       init;
  
  boss_byte_t       busy;
  
  boss_byte_t       wait_count;
  
  boss_sem_link_t   *wait_list;
  boss_tcb_t        *user_tcb;
} boss_sem_t;


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void Boss_sem_init(boss_sem_t *p_sem);
boss_reg_t Boss_sem_obtain(boss_sem_t *p_sem, boss_tmr_t wait_time_ms);
void Boss_sem_release(boss_sem_t *p_sem);

#endif  /* _BOSS_SEM_H_ */
