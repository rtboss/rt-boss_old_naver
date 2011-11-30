;/*
;*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
;*                                                                             *
;*                       RT-BOSS PORT [ Cortex-M3 IAR ASM ]                    *
;*                                                                             *
;*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
;*/
		RSEG    CODE:CODE(2)
		thumb


		EXTERN  _Boss_switch_current_tcb
		
;/*===========================================================================
;    P E N D S V _ H A N D L E R
;---------------------------------------------------------------------------*/
;	void PendSV_Handler(void)
		PUBLIC	PendSV_Handler
PendSV_Handler:
		CPSID   I              	;/* [ IRQ ��Ȱ��ȭ ] */
		MRS     R0, PSP       	;// R0-R3, R12, PC, PSR ����Ǿ� ����
		STMDB   R0!, {R4-R11}   ;// R4-R11 ����
		MOV     R4, LR          ;// LR �ӽ����� (BL ����� ����)

		;/*
		;** void *_Boss_switch_current_tcb(void *cur_task_sp)
		;** �Ű����� : "R0"�� �������� �½�ũ ���� ������
		;** ���ϰ�   : "R0"�� ������ �½�ũ ���� ������
		;*/
		BL      _Boss_switch_current_tcb

		MOV     LR, R4          ;// LR �ӽ����� (����)
		LDMIA   R0!, {R4-R11}   ;// R4-R11 ����
		MSR     PSP, R0
		CPSIE   I              	;/* [ IRQ Ȱ��ȭ ]   */
		BX      LR              ;// ���� PendSV (R0-R3, R12, PC, PSR ����)




;/*===========================================================================
;    _   B O S S _ S T A R T _ S C H E D U L E
;---------------------------------------------------------------------------*/
; 	void _Boss_start_schedule(boss_stk_t *cur_task_sp)
		PUBLIC	_Boss_start_schedule
_Boss_start_schedule:
		CPSIE   I     ;/* ���ͷ�Ʈ Ȱ��ȭ (SVC ȣ��� ��Ʈ��Ʈ ���� �����̸� ����) */
		SVC     0     ;/* SVC 0 ȣ�� (_SVC_Boss_start_schedule() �Լ� ����)        */



;/*===========================================================================
;    _   S   V   C _   B O S S _ S T A R T _ S C H E D U L E
;---------------------------------------------------------------------------*/
;	void _SVC_Boss_start_schedule(boss_stk_t *cur_task_sp)
		PUBLIC	_SVC_Boss_start_schedule
_SVC_Boss_start_schedule:
		CPSID   I                   ;/* [ IRQ ��Ȱ��ȭ ] */
		LDMIA   R0!, {R4-R11}       ;/* R0 : cur_task_sp */
		MSR     PSP, R0

		LDR     R0, =0xE000ED08     ;/* NVIC Vector Table Offset Register  */
		LDR     R0, [R0]            ;/* Vector Table ���� ����             */
		LDR     R0, [R0, #0]        ;/* �ֱ� ���� (__initial_sp)           */
		MSR     MSP, R0             ;/* "MSP"�� �ʱ� ���� ������ �ֱ�ȭ    */

		LDR     LR, =0xFFFFFFFD     ;/* ������ Ư�� ���, PSP ���           */
		CPSIE   I                   ;/* [ IRQ Ȱ��ȭ ]   */
		BX      LR                  ;/* ���� SVC (R0-R3, R12, PC, PSR ����) */



;/*===========================================================================
;    _ M C U _ I S R _ R U N N I N G
;---------------------------------------------------------------------------*/
;	boss_reg_t _mcu_isr_running(void)
		PUBLIC	_mcu_isr_running
_mcu_isr_running:
		;/* ISR (Interrupt Service Routine) ���� ���� */
		MRS     R0, IPSR    ;/* 0 = ISR �ƴ�  / !0 = ISR ������ */
		BX      LR



;/*===========================================================================
;    _ M C U _ I R Q _ H O L D
;---------------------------------------------------------------------------*/
;	boss_reg_t _mcu_irq_hold(void)
		PUBLIC	_mcu_irq_hold
_mcu_irq_hold:
		;/* IRQ (Interrupt Request) ���� */
		MRS     R0, PRIMASK   ;/* 0 = IRQ Enable  /  !0 = IRQ Disable  */
		BX      LR



;/*===========================================================================
;    _ M C U _ I R Q _ L O C K
;---------------------------------------------------------------------------*/
;	boss_reg_t _mcu_irq_lock(void)
		PUBLIC	_mcu_irq_lock
_mcu_irq_lock:
		MRS     R0, PRIMASK   ;/* IRQ ���¸� ���� (_irq_store_)  */
		CPSID   I             ;/* IRQ ��Ȱ��ȭ �Ѵ�.             */
		BX      LR



;/*===========================================================================
;    _ M C U _ I R Q _ F R E E
;---------------------------------------------------------------------------*/
;	void _mcu_irq_free(boss_reg_t _irq_store_)
		PUBLIC	_mcu_irq_free
_mcu_irq_free:
		MSR     PRIMASK, R0   ;/* IRQ ���¸� "_irq_store_"���·� �Ѵ�. */
		BX      LR



	END
