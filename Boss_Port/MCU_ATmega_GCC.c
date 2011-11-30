/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                      RT-BOSS PORT [ ATmega GCC(WinAVR) ]                    *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Kernel.h"
#include <avr/io.h>

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define PUSH_ALL_RESIST() \
                asm volatile("push  r29"); \
                asm volatile("push  r28"); \
                asm volatile("push  r17"); \
                asm volatile("push  r16"); \
                asm volatile("push  r15"); \
                asm volatile("push  r14"); \
                asm volatile("push  r13"); \
                asm volatile("push  r12"); \
                asm volatile("push  r11"); \
                asm volatile("push  r10"); \
                asm volatile("push  r9"); \
                asm volatile("push  r8"); \
                asm volatile("push  r7"); \
                asm volatile("push  r6"); \
                asm volatile("push  r5"); \
                asm volatile("push  r4"); \
                asm volatile("push  r3"); \
                asm volatile("push  r2");

#define POP_ALL_RESIST() \
                asm volatile("pop   r2"); \
                asm volatile("pop   r3"); \
                asm volatile("pop   r4"); \
                asm volatile("pop   r5"); \
                asm volatile("pop   r6"); \
                asm volatile("pop   r7"); \
                asm volatile("pop   r8"); \
                asm volatile("pop   r9"); \
                asm volatile("pop   r10"); \
                asm volatile("pop   r11"); \
                asm volatile("pop   r12"); \
                asm volatile("pop   r13"); \
                asm volatile("pop   r14"); \
                asm volatile("pop   r15"); \
                asm volatile("pop   r16"); \
                asm volatile("pop   r17"); \
                asm volatile("pop   r28"); \
                asm volatile("pop   r29");


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
boss_reg_t  _isr_nesting = 0;


/*===========================================================================*/
/*                        [[ ATmega 스택 초기 구성 ]]                        */
/*---------------------------------------------------------------------------*/
/*
  [_Boss_stk_init()]                [ T C B  멤 버 ]
                    |------------|
                    | 하위  번지 |
                    |############|------------------------+
                    |    0xFF    | <= p_tcb->sp_base[0]   |
                    |============|                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |    0x55    |                        |
                    |------------|                        |
      리턴 "SP" ->  |Empty(sp[0])| <= p_tcb->sp           |
                    |============|                        |
                    |SREG (sp[1])|                        |
                    |------------|                        |
                    | R2 (sp[2]) |                        |
                    |------------|                        |
                    | R3 (sp[3]) |                        |
                    |------------|                        |
                    | R4 (sp[4]) |                        |
                    |------------|                        |
                    | R5 (sp[5]) |                  p_tcb->stk_size
                    |------------|                   (스택 크기)
                    | R6 (sp[6]) |                        |
                    |------------|                        |
                    | R7 (sp[7]) |                        |
                    |------------|                        |
                    | R8 (sp[8]) |                        |
                    |------------|                        |
                    | R9 (sp[9]) |                        |
                    |------------|                        |
                    | R10(sp[10])|                        |
                    |------------|                        |
                    | R11(sp[11])|                        |
                    |------------|                        |
                    | R12(sp[12])|                        |
                    |------------|                        |
                    | R13(sp[13])|                        |
                    |------------|                        |
                    | R14(sp[14])|                        |
                    |------------|                        |
                    | R15(sp[15])|                        |
                    |------------|                        |
                    | R16(sp[16])|                        |
                    |------------|                        |
                    | R17(sp[17])|                        |
                    |++++++++++++|                        |
                    | R28(sp[18])|                        |
                    |------------|                        |
                    | R29(sp[19])|                        |
                    |++++++++++++|                        |
      executer() -> |PC_H(sp[20])|                        |
                    |------------|                        |
                    |PC_L(sp[21])|                        |
                    |============|                        |
                    |    0xBB    | <= &sp_base[size - 1]  |
                    |############|------------------------+
                    | 상위  번지 |
                    |------------|   "size = p_tcb->stk_size/ sizeof(boss_stk_t)"
*/


/*===========================================================================
    _ B O S S _ S T K _ I N I T
---------------------------------------------------------------------------*/
boss_stk_t *_Boss_stk_init(void (*executer)(void), boss_stk_t *sp_base, 
                                                        boss_uptr_t stk_bytes)
{
  boss_uptr_t size  = stk_bytes / sizeof(boss_stk_t);
  boss_stk_t  *sp;
  boss_uptr_t tmp;
  
  boss_uptr_t i;
  for(i = 0; i < size; i++) {
    sp_base[i] = (boss_stk_t)0x55;      // 스택       [S] stack
  }
  
  sp_base[0]      = (boss_stk_t)0xFF;   // 스택 끝    [F] finis
  sp_base[size-1] = (boss_stk_t)0xBB;   // 스택 시작  [B] begin

  sp = &sp_base[size-1];

  /* 문맥 전환시 사용할 스택을 초기화 한다. */
  /* ATmega 스택은 ED(Empty Descending) 방식으로 동작함. (TOP Stack Empty) */
  sp = sp - 22;     /* Empty(1) + 저장할 레지스트 수 (21) = 22 */

  sp[0]   = 0xED;   // Empty (TOP Stack Empty)
  
  sp[1]   = 0x80;   // SREG, (0x80 : 인터럽트 활성화)
  sp[2]   = 0x02;   // R2
  sp[3]   = 0x03;   // R3
  sp[4]   = 0x04;   // R4
  sp[5]   = 0x05;   // R5
  sp[6]   = 0x06;   // R6
  sp[7]   = 0x07;   // R7
  sp[8]   = 0x08;   // R8
  sp[9]   = 0x09;   // R9
  sp[10]  = 0x10;   // R10
  sp[11]  = 0x11;   // R11
  sp[12]  = 0x12;   // R12
  sp[13]  = 0x13;   // R13
  sp[14]  = 0x14;   // R14
  sp[15]  = 0x15;   // R15
  sp[16]  = 0x16;   // R16
  sp[17]  = 0x17;   // R17
  sp[18]  = 0x28;   // R28
  sp[19]  = 0x29;   // R29
  
  tmp = (boss_uptr_t)executer;
  sp[20]  = (boss_stk_t)(tmp >> 8);   // PC(H)
  sp[21]  = (boss_stk_t)tmp;          // PC(L)

  return sp;
}


/*===========================================================================
    _ B O S S _ C O N T E X T _ S W I T C H
---------------------------------------------------------------------------*/
extern boss_stk_t *_Boss_switch_current_tcb(boss_stk_t *cur_task_sp);
void _Boss_context_switch(void) __attribute__ ( ( naked ) );
void _Boss_context_switch(void)
{
  PUSH_ALL_RESIST();
  asm volatile("in    R0, __SREG__");
  asm volatile("push  R0");
  asm volatile("cli");                /* 인터럽트 비활성화 */
  
  asm volatile("in    R25, __SP_H__");            /* 매개변수 : "R25:R24"는 실행중인 태스크 스택 포인터  */
  asm volatile("in    R24, __SP_L__");
  
  asm volatile("call  _Boss_switch_current_tcb"); /* void *_Boss_switch_current_tcb(void *cur_task_sp) 호출*/
  
  asm volatile("out   __SP_H__, R25");            /* 리턴값 : "R25:R24"는 실행할 태스크 스택 포인터 */
  asm volatile("out   __SP_L__, R24");

  asm volatile("pop   R0");
  asm volatile("out   __SREG__, R0");
  POP_ALL_RESIST();
  
  asm volatile("ret");
}


/*===========================================================================
    _   B O S S _ S T A R T _ S C H E D U L E
---------------------------------------------------------------------------*/
void _Boss_start_schedule(boss_stk_t *cur_task_sp) __attribute__ ( ( naked ) );
void _Boss_start_schedule(boss_stk_t *cur_task_sp)
{
  asm volatile("out   __SP_H__, R25");
  asm volatile("out   __SP_L__, R24");
  
  asm volatile("pop   R0");
  asm volatile("out   __SREG__, R0");
  POP_ALL_RESIST();
  
  asm volatile("ret");
}


/*===========================================================================
    _ M C U _ I S R _ B E G I N
---------------------------------------------------------------------------*/
void _mcu_isr_begin(void)
{
  BOSS_ASSERT(_isr_nesting < 35);
  
  _isr_nesting++;
}


/*===========================================================================
    _ M C U _ I S R _ F I N I S
---------------------------------------------------------------------------*/
void _mcu_isr_finis(void)
{
  BOSS_ASSERT(_isr_nesting != 0);
  
  _isr_nesting--;
}


/*===========================================================================
    _ M C U _ I S R _ R U N N I N G
---------------------------------------------------------------------------*/
boss_reg_t _mcu_isr_running(void)
{
  /* ISR (Interrupt Service Routine) 실행 상태 */
  
  return _isr_nesting;
}


/*===========================================================================
    _ M C U _ I R Q _ H O L D
---------------------------------------------------------------------------*/
boss_reg_t _mcu_irq_hold(void)
{
  boss_reg_t irq_hold = _BOSS_TRUE;

  if( SREG & (1 << SREG_I) )    /* IRQ (Interrupt Request) 상태 */
  {
    irq_hold = _BOSS_FALSE;
  }

  return irq_hold;
}


/*===========================================================================
    _ M C U _ I R Q _ L O C K
---------------------------------------------------------------------------*/
boss_reg_t _mcu_irq_lock(void)
{
  boss_reg_t _irq_store_;

  _irq_store_ = SREG;
  asm volatile("cli");    /* [ IRQ 비활성화 ] */
  
  return _irq_store_;
}


/*===========================================================================
    _ M C U _ I R Q _ F R E E
---------------------------------------------------------------------------*/
void _mcu_irq_free(boss_reg_t _irq_store_)
{
  SREG = _irq_store_;   /* [ IRQ 복원 ] */
}

