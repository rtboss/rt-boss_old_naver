/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                       RT-BOSS PORT [ Cortex-M3 Keil]                        *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/
/*===========================================================================*/
/*                      [[ Cortex-M3 ���� �ʱ� ���� ]]                       */
/*---------------------------------------------------------------------------*/
/*
[ _Boss_stk_init() ]
                  |------------|
                  | ����  ���� |
                  |############|------------------------+
                  | 0xFFFFFFFF | <- sp_base[0]          |
                  | 0xFFFFFFFF | <- sp_base[1]          |
                  |============|                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  | 0xEEEEEEEE |                        |
                  |============|                        |
    ���� "SP" ->  |     R4     |                        |
                  |------------|                        |
                  |     R5     |                        |
                  |------------|                        |
                  |     R6     |                        |
                  |------------|                        |
                  |     R7     |                      size
                  |------------|                   (���� ũ��)
                  |     R8     |                        |
                  |------------|                        |
                  |     R9     |                        |
                  |------------|                        |
                  |     R10    |                        |
                  |------------|                        |
                  |     R11    |                        |
                  |============|                        |
                  |  R0(p_arg) |                        |
                  |------------|                        |
                  |     R1     |                        |
                  |------------|                        |
                  |     R2     |                        |
                  |------------|                        |
                  |     R3     |                        |
                  |------------|                        |
                  |     R12    |                        |
                  |------------|                        |
                  |     LR     |                        |
                  |------------|                        |
                  |  PC(Entry) |                        |
                  |------------|                        |
                  |     PSR    |                        |
                  |============|                        |
                  | 0xBBBBBBBB | <-  sp_base[size-2]    |
                  | 0xBBBBBBBB | <-  sp_base[size-1]    |
                  |############|------------------------+
                  | ����  ���� |
                  |------------|
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/

/*===========================================================================
    _ B O S S _ S T K _ I N I T
---------------------------------------------------------------------------*/
boss_stk_t *_Boss_stk_init( void (*task)(void *p_arg), void *p_arg,
                                boss_stk_t *sp_base,  boss_uptr_t stk_bytes)
{
  boss_uptr_t size  = stk_bytes / sizeof(boss_stk_t);
  boss_stk_t  *sp   = &sp_base[size];     /* FD(Full Descending) Stack */
 
  #if 1 /* Stack Debugging */
  boss_uptr_t i;
  for(i = 0; i < size; i++) {
    sp_base[i] = (boss_stk_t)0xEEEEEEEE;      // ����       [E] empty
  }
  sp = sp - 2;
  #endif

  /* FD(Full Descending) Stack */
  --sp;   *sp = 0x01000000L;          /* PSR  */
  --sp;   *sp = (boss_stk_t)task;     /* PC : Task Entry Point */
  --sp;   *sp = 0x00000014L;          /* LR   */ 
  --sp;   *sp = 0x00000012L;          /* R12  */
  --sp;   *sp = 0x00000003L;          /* R3   */
  --sp;   *sp = 0x00000002L;          /* R2   */
  --sp;   *sp = 0x00000001L;          /* R1   */
  --sp;   *sp = (boss_stk_t)p_arg;    /* R0 : Argument */
  --sp;   *sp = 0x00000011L;          /* R11  */
  --sp;   *sp = 0x00000010L;          /* R10  */
  --sp;   *sp = 0x00000009L;          /* R9   */
  --sp;   *sp = 0x00000008L;          /* R8   */
  --sp;   *sp = 0x00000007L;          /* R7   */
  --sp;   *sp = 0x00000006L;          /* R6   */
  --sp;   *sp = 0x00000005L;          /* R5   */
  --sp;   *sp = 0x00000004L;          /* R4   */
  
  return sp;
}


/*===========================================================================
    _   B O S S _ S T A R T _ S C H E D U L E
---------------------------------------------------------------------------*/
__ASM void _Boss_start_schedule(void)
{
  SVC     0         /* SVC 0 ȣ�� (SVC_Handler() ����)  */
}


/*===========================================================================
    S   V   C _   H A N D L E R
---------------------------------------------------------------------------*/
__ASM void SVC_Handler(void)
{
  IMPORT  _Boss_start_tcb_sp

  CPSID   I                   /* [ IRQ ��Ȱ��ȭ ] */
  
  LDR     R0, =0xE000ED08     /* NVIC Vector Table Offset Register  */
  LDR     R0, [R0]            /* Vector Table ���� ����             */
  LDR     R0, [R0, #0]        /* �ֱ� ���� (__initial_sp)           */
  MSR     MSP, R0             /* "MSP"�� �ʱ� ���� ������ �ʱ�ȭ    */

  BL      _Boss_start_tcb_sp  /* ���ϰ� : "R0"�� start_tcb_sp       */
  
  LDMIA   R0!, {R4-R11}
  MSR     PSP, R0

  LDR     LR, =0xFFFFFFFD     /* ������ Ư�� ���, PSP ���         */
  CPSIE   I                   /* [ IRQ Ȱ��ȭ ]   */
  BX      LR                  /* ���� SVC (R0-R3, R12, PC, PSR ����)*/

  ALIGN
}


/*===========================================================================
    _ B O S S _ C O N T E X T _ S W I T C H
---------------------------------------------------------------------------*/
void _Boss_context_switch(void)
{
  SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;   /* PendSV_Handler ȣ�� */
}


/*===========================================================================
    P E N D S V _ H A N D L E R
---------------------------------------------------------------------------*/
__ASM void PendSV_Handler(void)
{
  IMPORT  _Boss_switch_current_tcb

  MRS     R0, PSP         // R0-R3, R12, PC, PSR ����Ǿ� ����
  STMDB   R0!, {R4-R11}   // R4-R11 ����
  MOV     R4, LR          // LR �ӽ����� (BL ����� ����)

  CPSID   I                   /* [ IRQ ��Ȱ��ȭ ] */
  /*
  ** void *_Boss_switch_current_tcb(void *cur_task_sp)
  ** �Ű����� : "R0"�� �������� �½�ũ ���� ������
  ** ���ϰ�   : "R0"�� ������ �½�ũ ���� ������
  */
  BL      _Boss_switch_current_tcb

  CPSIE   I                   /* [ IRQ Ȱ��ȭ ]   */

  MOV     LR, R4          // LR �ӽ����� (����)
  LDMIA   R0!, {R4-R11}   // R4-R11 ����
  MSR     PSP, R0
  
  BX      LR              // ���� PendSV (R0-R3, R12, PC, PSR ����)

  ALIGN
}

