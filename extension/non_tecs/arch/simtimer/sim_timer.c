/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 * 
 *  Copyright (C) 2018-2020 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 * 
 *  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 * 
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 * 
 *  $Id: sim_timer.c 980 2020-05-25 07:06:43Z ertl-hiro $
 */

/*
 *		タイマドライバシミュレータ
 */

#include "kernel_impl.h"
#include "time_event.h"
#include "domain.h"
#ifdef TOPPERS_SUPPORT_OVRHDR
#include "overrun.h"
#endif /* TOPPERS_SUPPORT_OVRHDR */
#include "target_timer.h"

/*
 *  シミュレート時間の初期値
 */
#ifndef SIMTIM_INIT_CURRENT
#define SIMTIM_INIT_CURRENT		10
#endif /* SIMTIM_INIT_CURRENT */

/*
 *  高分解能タイマ割込みの受付オーバヘッド
 */
#ifndef SIMTIM_OVERHEAD_HRTINT
#define SIMTIM_OVERHEAD_HRTINT	10
#endif /* SIMTIM_OVERHEAD_HRTINT */

/*
 *  タイムウィンドウタイマ割込みの受付オーバヘッド
 */
#ifndef SIMTIM_OVERHEAD_TWDINT
#define SIMTIM_OVERHEAD_TWDINT	10
#endif /* SIMTIM_OVERHEAD_TWDINT */

#ifdef TOPPERS_SUPPORT_OVRHDR
/*
 *  オーバランタイマ割込みの受付オーバヘッド
 */
#ifndef SIMTIM_OVERHEAD_OVRINT
#define SIMTIM_OVERHEAD_OVRINT	10
#endif /* SIMTIM_OVERHEAD_OVRINT */

#endif /* TOPPERS_SUPPORT_OVRHDR */

/*
 *  シミュレーション時間のデータ型の定義
 */
typedef uint64_t	SIMTIM;

/*
 *  タイマ割込みの発生時刻の設定状況
 */
typedef struct {
	bool_t		enable;				/* 発生時刻が設定されているか？ */
	SIMTIM		simtim;				/* 発生時刻 */
	void		(*raise)(void);		/* タイマ割込みの要求 */
} INT_EVENT;

/*
 *  現在のシミュレーション時刻
 */
static SIMTIM	current_simtim;

/*
 *  最初に発生するタイマ割込みの選択
 */
static void		select_event(void);

/*
 *  高分解能タイマ割込みの発生時刻
 */
static INT_EVENT	hrt_event;

Inline SIMTIM
truncate_simtim(SIMTIM simtim)
{
	return(simtim / TSTEP_HRTCNT * TSTEP_HRTCNT);
}

Inline SIMTIM
roundup_simtim(SIMTIM simtim)
{
	return((simtim + TSTEP_HRTCNT - 1) / TSTEP_HRTCNT * TSTEP_HRTCNT);
}

/*
 *  高分解能タイマの現在のカウント値の読出し
 */
HRTCNT
target_hrt_get_current(void)
{
#ifdef TCYC_HRTCNT
	return((HRTCNT)(truncate_simtim(current_simtim) % TCYC_HRTCNT));
#else /* TCYC_HRTCNT */
	return((HRTCNT) truncate_simtim(current_simtim));
#endif /* TCYC_HRTCNT */
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 */
void
target_hrt_set_event(HRTCNT hrtcnt)
{
#ifdef HOOK_HRT_EVENT
	hook_hrt_set_event(hrtcnt);
#endif /* HOOK_HRT_EVENT */

	hrt_event.enable = true;
	hrt_event.simtim = roundup_simtim(current_simtim + hrtcnt);
	select_event();
}

/*
 *  高分解能タイマへの割込みタイミングのクリア
 */
#ifdef USE_64BIT_HRTCNT

void
target_hrt_clear_event(void)
{
#ifdef HOOK_HRT_EVENT
	hook_hrt_clear_event();
#endif /* HOOK_HRT_EVENT */

	hrt_event.enable = false;
	select_event();
}

#endif /* USE_64BIT_HRTCNT */

/*
 *  高分解能タイマ割込みの要求
 */
void
target_hrt_raise_event(void)
{
#ifdef HOOK_HRT_EVENT
	hook_hrt_raise_event();
#endif /* HOOK_HRT_EVENT */

	target_raise_hrt_int();
}

/*
 *  シミュレートされた高分解能タイマ割込みハンドラ
 */
void
target_hrt_handler(void)
{
	current_simtim += SIMTIM_OVERHEAD_HRTINT;
	signal_time();
}

/*
 *  タイムウィンドウタイマ割込みの発生時刻
 */
static INT_EVENT	twd_event;

/*
 *  タイムウィンドウタイマの動作開始
 */
void
target_twdtimer_start(PRCTIM twdtim)
{
	if (twdtim == 0) {
		twd_event.enable = false;
		select_event();
		target_raise_twd_int();
	}
	else {
		twd_event.enable = true;
		twd_event.simtim = current_simtim + twdtim;
		select_event();
	}
}

/*
 *  タイムウィンドウタイマの停止
 *
 *  ここでタイムウィンドウタイマ割込み要求をクリアすると，割込み源の特
 *  定に失敗する（QEMUで確認．QEMUだけの問題か，実機にもある問題かは未
 *  確認）ため，クリアしない．
 */
PRCTIM
target_twdtimer_stop(void)
{
	PRCTIM	twdtim;

	if (twd_event.simtim <= current_simtim) {
		twdtim = 0U;
	}
	else {
		twdtim = twd_event.simtim - current_simtim;
	}
	twd_event.enable = false;
	select_event();
	target_clear_twd_int();
	return(twdtim);
}

/*
 *  タイムウィンドウタイマの現在値の読出し
 */
PRCTIM
target_twdtimer_get_current(void)
{
	if (twd_event.simtim <= current_simtim) {
		return(0U);
	}
	else {
		return(twd_event.simtim - current_simtim);
	}
}

/*
 *  シミュレートされたタイムウィンドウタイマ割込みハンドラ
 */
void
target_twdtimer_handler(void)
{
	current_simtim += SIMTIM_OVERHEAD_TWDINT;
	twd_switch();
}

#ifdef TOPPERS_SUPPORT_OVRHDR
/*
 *  オーバランタイマ割込みの発生時刻
 */
static INT_EVENT	ovr_event;

/*
 *  オーバランタイマの動作開始
 */
void
target_ovrtimer_start(PRCTIM ovrtim)
{
	if (ovrtim == 0) {
		ovr_event.enable = false;
		select_event();
		target_raise_ovr_int();
	}
	else {
		ovr_event.enable = true;
		ovr_event.simtim = current_simtim + ovrtim;
		select_event();
	}
}

/*
 *  オーバランタイマの停止
 *
 *  ここでオーバランタイマ割込み要求をクリアすると，割込み源の特定に失
 *  敗する（QEMUで確認．QEMUだけの問題か，実機にもある問題かは未確認）
 *  ため，クリアしない．
 */
PRCTIM
target_ovrtimer_stop(void)
{
	PRCTIM	ovrtim;

	if (ovr_event.simtim <= current_simtim) {
		ovrtim = 0U;
	}
	else {
		ovrtim = ovr_event.simtim - current_simtim;
	}
	ovr_event.enable = false;
	select_event();
	target_clear_ovr_int();
	return(ovrtim);
}

/*
 *  オーバランタイマの現在値の読出し
 */
PRCTIM
target_ovrtimer_get_current(void)
{
	if (ovr_event.simtim <= current_simtim) {
		return(0U);
	}
	else {
		return(ovr_event.simtim - current_simtim);
	}
}

/*
 *  シミュレートされたオーバランタイマ割込みハンドラ
 */
void
target_ovrtimer_handler(void)
{
	current_simtim += SIMTIM_OVERHEAD_OVRINT;
	call_ovrhdr();
}

#endif /* TOPPERS_SUPPORT_OVRHDR */

/*
 *  最初に発生するタイマ割込みの情報
 */
static INT_EVENT	*p_next_event;

/*
 *  タイマの起動処理
 */
void
target_timer_initialize(EXINF exinf)
{
	current_simtim = SIMTIM_INIT_CURRENT;
	hrt_event.enable = false;
	hrt_event.raise = &target_raise_hrt_int;
	twd_event.enable = false;
	twd_event.raise = &target_raise_twd_int;
#ifdef TOPPERS_SUPPORT_OVRHDR
	ovr_event.enable = false;
	ovr_event.raise = &target_raise_ovr_int;
#endif /* TOPPERS_SUPPORT_OVRHDR */
	p_next_event = NULL;
}

/*
 *  タイマの停止処理
 */
void
target_timer_terminate(EXINF exinf)
{
	hrt_event.enable = false;
	twd_event.enable = false;
#ifdef TOPPERS_SUPPORT_OVRHDR
	ovr_event.enable = false;
#endif /* TOPPERS_SUPPORT_OVRHDR */
}

/*
 *  最初に発生するタイマ割込みの選択
 */
static void
select_event(void)
{
	if (hrt_event.enable) {
		p_next_event = &hrt_event;
	}
	else {
		p_next_event = NULL;
	}

	if (twd_event.enable && (p_next_event == NULL
								|| twd_event.simtim <= p_next_event->simtim)) {
		p_next_event = &twd_event;
	}

#ifdef TOPPERS_SUPPORT_OVRHDR
	if (ovr_event.enable && (p_next_event == NULL
								|| ovr_event.simtim <= p_next_event->simtim)) {
		p_next_event = &ovr_event;
	}
#endif /* TOPPERS_SUPPORT_OVRHDR */
}

/*
 *  カーネルのアイドル処理
 */
void
target_custom_idle(void)
{
	lock_cpu();
	if (p_next_event != NULL) {
		current_simtim = p_next_event->simtim;
		p_next_event->enable = false;
		(*(p_next_event->raise))();
		select_event();
	}
	unlock_cpu();
}

/*
 *  シミュレーション時刻を進める（テストプログラム用）
 */
void
simtim_advance(uint_t time)
{
	bool_t	locked;

	locked = sns_loc();
	if (!locked) {
		loc_cpu();
	}

	while (p_next_event != NULL
					&& p_next_event->simtim <= current_simtim + time) {
		/*
		 *  時刻をtime進めると，タイマ割込みの発生時刻を過ぎる場合
		 */
		if (current_simtim < p_next_event->simtim) {
			time -= (p_next_event->simtim - current_simtim);
			current_simtim = p_next_event->simtim;
		}
		p_next_event->enable = false;
		(*(p_next_event->raise))();
		select_event();

		/*
		 *  ここで割込みを受け付ける．
		 */
		if (!locked) {
			unl_cpu();
			delay_for_interrupt();
			loc_cpu();
		}
	}	 
	current_simtim += time;

	if (!locked) {
		unl_cpu();
	}
}

/*
 *  シミュレーション時刻を強制的に進める（テストプログラム用）
 */
void
simtim_add(uint_t time)
{
	bool_t	locked;

	locked = sns_loc();
	if (!locked) {
		loc_cpu();
	}

	current_simtim += time;

	if (!locked) {
		unl_cpu();
	}
}

/*
 *  拡張サービスコールとして登録するための関数
 */
#ifdef TOPPERS_OMIT_TECS

ER_UINT
extsvc_simtim_advance(intptr_t time, intptr_t par2, intptr_t par3,
							intptr_t par4, intptr_t par5, ID cdmid)
{
	simtim_advance((uint_t) time);
	return((ER_UINT) 0U);
}

ER_UINT
extsvc_simtim_add(intptr_t time, intptr_t par2, intptr_t par3,
							intptr_t par4, intptr_t par5, ID cdmid)
{
	simtim_add((uint_t) time);
	return((ER_UINT) 0U);
}

#endif /* TOPPERS_OMIT_TECS */
