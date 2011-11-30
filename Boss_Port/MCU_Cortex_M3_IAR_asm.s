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
		CPSID   I              	;/* [ IRQ 비활성화 ] */
		MRS     R0, PSP       	;// R0-R3, R12, PC, PSR 저장되어 있음
		STMDB   R0!, {R4-R11}   ;// R4-R11 저장
		MOV     R4, LR          ;// LR 임시저장 (BL 사용을 위해)

		;/*
		;** void *_Boss_switch_current_tcb(void *cur_task_sp)
		;** 매개변수 : "R0"는 실행중인 태스크 스택 포인터
		;** 리턴값   : "R0"는 실행할 태스크 스택 포인터
		;*/
		BL      _Boss_switch_current_tcb

		MOV     LR, R4          ;// LR 임시저장 (복원)
		LDMIA   R0!, {R4-R11}   ;// R4-R11 복원
		MSR     PSP, R0
		CPSIE   I              	;/* [ IRQ 활성화 ]   */
		BX      LR              ;// 리턴 PendSV (R0-R3, R12, PC, PSR 복원)




;/*===========================================================================
;    _   B O S S _ S T A R T _ S C H E D U L E
;---------------------------------------------------------------------------*/
; 	void _Boss_start_schedule(boss_stk_t *cur_task_sp)
		PUBLIC	_Boss_start_schedule
_Boss_start_schedule:
		CPSIE   I     ;/* 인터럽트 활성화 (SVC 호출시 인트럽트 중지 상태이면 에러) */
		SVC     0     ;/* SVC 0 호출 (_SVC_Boss_start_schedule() 함수 실행)        */



;/*===========================================================================
;    _   S   V   C _   B O S S _ S T A R T _ S C H E D U L E
;---------------------------------------------------------------------------*/
;	void _SVC_Boss_start_schedule(boss_stk_t *cur_task_sp)
		PUBLIC	_SVC_Boss_start_schedule
_SVC_Boss_start_schedule:
		CPSID   I                   ;/* [ IRQ 비활성화 ] */
		LDMIA   R0!, {R4-R11}       ;/* R0 : cur_task_sp */
		MSR     PSP, R0

		LDR     R0, =0xE000ED08     ;/* NVIC Vector Table Offset Register  */
		LDR     R0, [R0]            ;/* Vector Table 시작 번지             */
		LDR     R0, [R0, #0]        ;/* 최기 스택 (__initial_sp)           */
		MSR     MSP, R0             ;/* "MSP"를 초기 스택 값으로 최기화    */

		LDR     LR, =0xFFFFFFFD     ;/* 스레드 특근 모드, PSP 사용           */
		CPSIE   I                   ;/* [ IRQ 활성화 ]   */
		BX      LR                  ;/* 리턴 SVC (R0-R3, R12, PC, PSR 복원) */



;/*===========================================================================
;    _ M C U _ I S R _ R U N N I N G
;---------------------------------------------------------------------------*/
;	boss_reg_t _mcu_isr_running(void)
		PUBLIC	_mcu_isr_running
_mcu_isr_running:
		;/* ISR (Interrupt Service Routine) 실행 상태 */
		MRS     R0, IPSR    ;/* 0 = ISR 아님  / !0 = ISR 실행중 */
		BX      LR



;/*===========================================================================
;    _ M C U _ I R Q _ H O L D
;---------------------------------------------------------------------------*/
;	boss_reg_t _mcu_irq_hold(void)
		PUBLIC	_mcu_irq_hold
_mcu_irq_hold:
		;/* IRQ (Interrupt Request) 상태 */
		MRS     R0, PRIMASK   ;/* 0 = IRQ Enable  /  !0 = IRQ Disable  */
		BX      LR



;/*===========================================================================
;    _ M C U _ I R Q _ L O C K
;---------------------------------------------------------------------------*/
;	boss_reg_t _mcu_irq_lock(void)
		PUBLIC	_mcu_irq_lock
_mcu_irq_lock:
		MRS     R0, PRIMASK   ;/* IRQ 상태를 리턴 (_irq_store_)  */
		CPSID   I             ;/* IRQ 비활성화 한다.             */
		BX      LR



;/*===========================================================================
;    _ M C U _ I R Q _ F R E E
;---------------------------------------------------------------------------*/
;	void _mcu_irq_free(boss_reg_t _irq_store_)
		PUBLIC	_mcu_irq_free
_mcu_irq_free:
		MSR     PRIMASK, R0   ;/* IRQ 상태를 "_irq_store_"상태로 한다. */
		BX      LR



	END
