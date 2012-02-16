/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                       RT-BOSS PORT [ Cortex-M3 IAR]                         *
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
                  | 0xEEEEEEEE | <- sp_base[0]          |
                  | 0xEEEEEEEE | <- sp_base[1]          |
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
                  | 0xEEEEEEEE | <-  sp_base[size-2]    |
                  | 0xEEEEEEEE | <-  sp_base[size-1]    |
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
    sp_base[i] = (boss_stk_t)0xEEEEEEEE;      // ���� [E] empty
  }
  sp = sp - 2;    /* begin */
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
    _ B O S S _ C O N T E X T _ S W I T C H
---------------------------------------------------------------------------*/
void _Boss_context_switch(void)
{
  SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;   /* PendSV_Handler ȣ�� */
}



/*
** �Ʒ��� �Լ��� "Boss_CPU_ARM_CM3_IAR_asm.s" ����
**
** void _Boss_start_schedule(void);
** void SVC_Handler(void);
** void PendSV_Handler(void);
*/
