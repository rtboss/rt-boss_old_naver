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
		EXTERN  _Boss_start_tcb_sp

;/*===========================================================================
;    _   B O S S _ S T A R T _ S C H E D U L E
;---------------------------------------------------------------------------*/
; 	void _Boss_start_schedule(void)
		PUBLIC	_Boss_start_schedule
_Boss_start_schedule:
		SVC     0         ;/* SVC 0 ȣ�� (SVC_Handler() ����)  */





;/*===========================================================================
;    S   V   C _   H A N D L E R
;---------------------------------------------------------------------------*/
;	void SVC_Handler(void)
		PUBLIC	SVC_Handler
SVC_Handler:
		CPSID   I                   ;/* [ IRQ ��Ȱ��ȭ ] */

		LDR     R0, =0xE000ED08     ;/* NVIC Vector Table Offset Register  */
		LDR     R0, [R0]            ;/* Vector Table ���� ����             */
		LDR     R0, [R0, #0]        ;/* �ֱ� ���� (__initial_sp)           */
		MSR     MSP, R0             ;/* "MSP"�� �ʱ� ���� ������ �ʱ�ȭ    */

		BL      _Boss_start_tcb_sp  ;/* ���ϰ� : "R0"�� start_tcb_sp       */

		LDMIA   R0!, {R4-R11}
		MSR     PSP, R0

		LDR     LR, =0xFFFFFFFD     ;/* ������ Ư�� ���, PSP ���         */
		CPSIE   I                   ;/* [ IRQ Ȱ��ȭ ]   */
		BX      LR                  ;/* ���� SVC (R0-R3, R12, PC, PSR ����)*/





;/*===========================================================================
;    P E N D S V _ H A N D L E R
;---------------------------------------------------------------------------*/
;	void PendSV_Handler(void)
		PUBLIC	PendSV_Handler
PendSV_Handler:
		MRS     R0, PSP         ;// R0-R3, R12, PC, PSR ����Ǿ� ����
		STMDB   R0!, {R4-R11}   ;// R4-R11 ����
		MOV     R4, LR          ;// LR �ӽ����� (BL ����� ����)

		CPSID   I                   ;/* [ IRQ ��Ȱ��ȭ ] */
		;/*
		;** void *_Boss_switch_current_tcb(void *cur_task_sp)
		;** �Ű����� : "R0"�� �������� �½�ũ ���� ������
		;** ���ϰ�   : "R0"�� ������ �½�ũ ���� ������
		;*/
		BL      _Boss_switch_current_tcb

		CPSIE   I                   ;/* [ IRQ Ȱ��ȭ ]   */

		MOV     LR, R4          ;// LR �ӽ����� (����)
		LDMIA   R0!, {R4-R11}   ;// R4-R11 ����
		MSR     PSP, R0

		BX      LR              ;// ���� PendSV (R0-R3, R12, PC, PSR ����)



		END
