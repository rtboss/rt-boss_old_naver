#ifndef _BOSS_KERNEL_H_
#define _BOSS_KERNEL_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                              RT-BOSS (Kernel)                               *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Conf.h"


/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _BOSS_NULL          ((void *) 0)

#define _BOSS_FALSE         ( 0)
#define _BOSS_TRUE          (!_BOSS_FALSE)

#define _BOSS_SUCCESS       ( 0)
#define _BOSS_FAILURE       (-1)    /* (32bit=0xFFFFFFFF) / (16bit=0xFFFF) */

/*---------------------------------------------------------------------------*/
/*                         RT-BOSS 시그널 비트 정의                          */
/*                                                                           */
/*    시그널 데이터형이 32Bit일 경우 "BOSS_SIG_MSB_BIT" 는 "0x8000 0000"     */
/*    시그널 데이터형이 16bit일 경우 "BOSS_SIG_MSB_BIT" 는 "0x8000"          */
/*---------------------------------------------------------------------------*/
#define BOSS_SIG_MSB_BIT  (boss_sigs_t)((boss_sigs_t)1 << ((sizeof(boss_sigs_t)*8)-1))
#define BOSS_SIGS_NULL    (boss_sigs_t)0

#define BOSS_SIG_SLEEP            (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 0)  /* 슬립     */
#define BOSS_SIG_SEM_OBTAIN       (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 1)  /* 세마포어 */
#define BOSS_SIG_MBOX_PEND_DONE   (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 2)  /* MBox 완료 */
// 예비(Reserve)    BOSS_SIG_RESERVE    (BOSS_SIG_MSB_BIT >> 3)   /* 예비     */

/*---------------------------------------------------------------------------*/
/*                        TCB (Task Control Block)                           */
/*---------------------------------------------------------------------------*/
typedef enum {            /* [ 태스크 상태값 정의 ] */
  TASK_DORMANT   = -1,    // 제거된 태스크
  TASK_WAITING   =  0,    // 시그널 대기상태
  TASK_READY,             // 실행 대기상태
  TASK_SUSPEND,           // 실행중 보류상태
  TASK_RUNNING,           // 실행중인 상태
} _task_status_t;

typedef struct boss_tcb_struct {  /* [ TCB (Task Control Block) 구조체 ] */  
  _task_status_t    status;         // 태스크 상태
  
  boss_prio_t       prio;           // 우선순위 (임시 변경 될수 있음)
  boss_prio_t       prio_orig;      // 실제 우선순위
  boss_byte_t       prio_rise;      // 우선순위 고정 및 상승 횟수
  
  boss_sigs_t       sigs;           // 시그널
  boss_sigs_t       sigs_wait;      // 대기 시그널
  
  boss_stk_t        *sp;            // 현재 스택 포인터
  boss_stk_t        *sp_base;       // 태스크 생성시 스택 초기 포인터
  boss_uptr_t       stk_size;       // 스택 크기

  boss_tmr_t        sleep_ms;       // 태스크 Sleep 시간 (ms), "sleep_ms != 0" Sleep Waiting

  void *            args;           // 태스크 생성시 전달값 (ex : 태스크 이름)
  
  struct boss_tcb_struct *next;     // 스케줄러 리스트의 다음 TCB 포인터
} boss_tcb_t;


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void Boss_init(boss_tcb_t *idle_tcb, void (*idle_exe)(void),
                                  boss_stk_t *sp_base, boss_uptr_t stk_bytes);

void Boss_start(void (*init_func)(void));

boss_tcb_t *Boss_self(void);
void _Boss_sched_lock(void);
void _Boss_sched_free(void);
boss_reg_t Boss_sched_locking(void);

void Boss_sigs_send(boss_tcb_t *p_tcb, boss_sigs_t sigs);
void Boss_sigs_clear(boss_tcb_t *p_tcb, boss_sigs_t sigs);
boss_sigs_t Boss_wait(boss_sigs_t wait_sigs, boss_tmr_t sleep_ms);
boss_sigs_t Boss_sigs_receive(void);


void Boss_task_priority(boss_tcb_t *p_tcb, boss_prio_t new_prio);
void Boss_task_create( boss_tcb_t *p_tcb, void (*executer)(void), 
      boss_prio_t prio, boss_stk_t *sp_base, boss_uptr_t stk_bytes, void *args);
void Boss_task_delete(boss_tcb_t *p_tcb);

#endif  /* _BOSS_KERNEL_H_ */
