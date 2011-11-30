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
/*                         RT-BOSS �ñ׳� ��Ʈ ����                          */
/*                                                                           */
/*    �ñ׳� ���������� 32Bit�� ��� "BOSS_SIG_MSB_BIT" �� "0x8000 0000"     */
/*    �ñ׳� ���������� 16bit�� ��� "BOSS_SIG_MSB_BIT" �� "0x8000"          */
/*---------------------------------------------------------------------------*/
#define BOSS_SIG_MSB_BIT  (boss_sigs_t)((boss_sigs_t)1 << ((sizeof(boss_sigs_t)*8)-1))
#define BOSS_SIGS_NULL    (boss_sigs_t)0

#define BOSS_SIG_SLEEP            (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 0)  /* ����     */
#define BOSS_SIG_SEM_OBTAIN       (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 1)  /* �������� */
#define BOSS_SIG_MBOX_PEND_DONE   (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 2)  /* MBox �Ϸ� */
// ����(Reserve)    BOSS_SIG_RESERVE    (BOSS_SIG_MSB_BIT >> 3)   /* ����     */

/*---------------------------------------------------------------------------*/
/*                        TCB (Task Control Block)                           */
/*---------------------------------------------------------------------------*/
typedef enum {            /* [ �½�ũ ���°� ���� ] */
  TASK_DORMANT   = -1,    // ���ŵ� �½�ũ
  TASK_WAITING   =  0,    // �ñ׳� ������
  TASK_READY,             // ���� ������
  TASK_SUSPEND,           // ������ ��������
  TASK_RUNNING,           // �������� ����
} _task_status_t;

typedef struct boss_tcb_struct {  /* [ TCB (Task Control Block) ����ü ] */  
  _task_status_t    status;         // �½�ũ ����
  
  boss_prio_t       prio;           // �켱���� (�ӽ� ���� �ɼ� ����)
  boss_prio_t       prio_orig;      // ���� �켱����
  boss_byte_t       prio_rise;      // �켱���� ���� �� ��� Ƚ��
  
  boss_sigs_t       sigs;           // �ñ׳�
  boss_sigs_t       sigs_wait;      // ��� �ñ׳�
  
  boss_stk_t        *sp;            // ���� ���� ������
  boss_stk_t        *sp_base;       // �½�ũ ������ ���� �ʱ� ������
  boss_uptr_t       stk_size;       // ���� ũ��

  boss_tmr_t        sleep_ms;       // �½�ũ Sleep �ð� (ms), "sleep_ms != 0" Sleep Waiting

  void *            args;           // �½�ũ ������ ���ް� (ex : �½�ũ �̸�)
  
  struct boss_tcb_struct *next;     // �����ٷ� ����Ʈ�� ���� TCB ������
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
