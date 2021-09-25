/*
 *  TOPPERS/HRP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      High Reliable system Profile Kernel
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2004-2020 by Embedded and Real-Time Systems Laboratory
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
 *  $Id: sample1.c 980 2020-05-25 07:06:43Z ertl-hiro $
 */

/* 
 *  サンプルプログラム(1)の本体
 *
 *  HRPカーネルの基本的な動作を確認するためのサンプルプログラム．
 *
 *  プログラムの概要:
 *
 *  ユーザインタフェースを受け持つメインタスク（MAIN_TASK）と，3つの並
 *  行実行されるタスク（TASK1〜TASK3），例外処理タスク（EXC_TASK），ア
 *  ラーム通知で起動されるタスク（ALM_TASK）の6つのタスクを用いる．こ
 *  れらの他に，システムログタスクが動作する．また，周期ハンドラ，アラー
 *  ム通知，割込みサービスルーチン，CPU例外ハンドラをそれぞれ1つ用いる．
 *  TASK1，TASK2，ALM_TASK，アラーム通知は保護ドメイン1（DOM1）に，
 *  TASK3は保護ドメイン2（DOM2）に，その他はカーネルドメインに属する．
 *
 *  システム周期を15ミリ秒とし，システム動作モードを3つ用意する．シス
 *  テム動作モード1（SOM1）では，DOM1とDOM2に各4ミリ秒のタイムウィンド
 *  ウを割り当てる．システム動作モード2（SOM2）とシステム動作モード
 *  3（SOM3）は，2つを巡回するようにし，いずれのシステム動作モードでも，
 *  DOM1の方に長いタイムウィンドウを割り当てる．
 *
 *  並行実行されるタスクは，task_loop回のループを実行する度に，タスク
 *  が実行中であることをあらわすメッセージを表示する．ループを実行する
 *  のは，プログラムの動作を確認しやすくするためである．また，低速なシ
 *  リアルポートを用いてメッセージを出力する場合に，すべてのメッセージ
 *  が出力できるように，メッセージの量を制限するという理由もある．
 *
 *  周期ハンドラ，アラーム通知で起動されるタスク，割込みサービスルーチ
 *  ンは，保護ドメイン1の3つの優先度（HIGH_PRIORITY，MID_PRIORITY，
 *  LOW_PRIORITY）のレディキューを回転させる．周期ハンドラは，プログラ
 *  ムの起動直後は停止状態になっている．
 *
 *  CPU例外ハンドラは，CPU例外からの復帰が可能な場合には，例外処理タス
 *  クを起動する．例外処理タスクは，CPU例外を起こしたタスクに対して，
 *  終了要求を行う．
 *
 *  メインタスクは，シリアルポートからの文字入力を行い（文字入力を待っ
 *  ている間は，並行実行されるタスクが実行されている），入力された文字
 *  に対応した処理を実行する．入力された文字と処理の関係は次の通り．
 *  Control-Cまたは'Q'が入力されると，プログラムを終了する．
 *
 *  '1' : 対象タスクをTASK1に切り換える（初期設定）．
 *  '2' : 対象タスクをTASK2に切り換える．
 *  '3' : 対象タスクをTASK3に切り換える．
 *  '4' : 対象タスクからTASK1をact_tskにより起動する．
 *  '5' : 対象タスクからTASK2をact_tskにより起動する．
 *  '6' : 対象タスクからTASK3をact_tskにより起動する．
 *  'a' : 対象タスクをact_tskにより起動する．
 *  'A' : 対象タスクに対する起動要求をcan_actによりキャンセルする．
 *  'e' : 対象タスクにext_tskを呼び出させ，終了させる．
 *  't' : 対象タスクをter_tskにより強制終了する．
 *  '>' : 対象タスクの優先度をHIGH_PRIORITYにする．
 *  '=' : 対象タスクの優先度をMID_PRIORITYにする．
 *  '<' : 対象タスクの優先度をLOW_PRIORITYにする．
 *  'G' : 対象タスクの優先度をget_priで読み出す．
 *  's' : 対象タスクにslp_tskを呼び出させ，起床待ちにさせる．
 *  'S' : 対象タスクにtslp_tsk(10秒)を呼び出させ，起床待ちにさせる．
 *  'w' : 対象タスクをwup_tskにより起床する．
 *  'W' : 対象タスクに対する起床要求をcan_wupによりキャンセルする．
 *  'l' : 対象タスクをrel_waiにより強制的に待ち解除にする．
 *  'u' : 対象タスクをsus_tskにより強制待ち状態にする．
 *  'm' : 対象タスクの強制待ち状態をrsm_tskにより解除する．
 *  'd' : 対象タスクにdly_tsk(10秒)を呼び出させ，時間経過待ちにさせる．
 *  'x' : 対象タスクにras_terにより終了要求する．
 *  'y' : 対象タスクにdis_terを呼び出させ，タスク終了を禁止する．
 *  'Y' : 対象タスクにena_terを呼び出させ，タスク終了を許可する．
 *  'r' : DOM1の3つの優先度（HIGH_PRIORITY，MID_PRIORITY，LOW_PRIORITY）
 *        のレディキューを回転させる．
 *  'c' : 周期ハンドラを動作開始させる．
 *  'C' : 周期ハンドラを動作停止させる．
 *  'b' : アラーム通知を5秒後に起動するよう動作開始させる．
 *  'B' : アラーム通知を動作停止させる．
 *  'j' : システム動作モードをSOM2に変更する．
 *  'J' : システム動作モードをSOM1に変更する．
 *  'z' : 対象タスクにCPU例外を発生させる（ターゲットによっては復帰可能）．
 *  'Z' : 対象タスクにCPUロック状態でCPU例外を発生させる（復帰不可能）．
 *  '@' : タスク3をacre_tskにより生成する．
 *  '!' : 対象タスクをdel_tskにより削除する．
 *  '$' : アラームハンドラをacre_almにより生成する．
 *  '#' : アラームハンドラをdel_almにより削除する．
 *  'V' : 短いループを挟んで，fch_hrtで高分解能タイマを2回読む．
 *  'v' : 発行したシステムコールを表示する（デフォルト）．
 *  'q' : 発行したシステムコールを表示しない．
 */

#include <kernel.h>
#include <t_syslog.h>
#include <t_stdlib.h>
#include "syssvc/serial.h"
#include "syssvc/syslog.h"
#include "kernel_cfg.h"
#include "sample1.h"

/*
 *  サービスコールのエラーのログ出力
 */
Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
	if (ercd < 0) {
		t_perror(LOG_ERROR, file, line, expr, ercd);
	}
}

#define	SVC_PERROR(expr)	svc_perror(__FILE__, __LINE__, #expr, (expr))

/*
 *  プロセッサ時間の消費
 *
 *  ループによりプロセッサ時間を消費する．最適化ができないように，ルー
 *  プ内でvolatile変数を読み込む．
 */
static volatile long_t	volatile_var;

static void
consume_time(ulong_t ctime)
{
	ulong_t		i;

	for (i = 0; i < ctime; i++) {
		(void) volatile_var;
	}
}

/*
 *  並行実行されるタスクへのメッセージ領域
 */
char	message[3];

/*
 *  ループ回数
 */
ulong_t	task_loop;		/* タスク内でのループ回数 */

/*
 *  動的生成するオブジェクトのID
 */
ID		TASK3 = -1;
ID		ALMHDR1 = -1;

/*
 *  TASK3のユーザスタック領域（ターゲット依存，GCC依存）
 */
char user_stack_TASK3[STACK_SIZE]
		__attribute__((section(".noinit_DOM2"),nocommon,aligned(4096)));

/*
 *  並行実行されるタスク
 */
void
task(EXINF exinf)
{
	int_t		n = 0;
	int_t		tskno = (int_t) exinf;
	const char	*graph[] = { "@    |", "  +  |", "     |  *" };
	char		c;

	while (true) {
		syslog(LOG_NOTICE, "task%d is running (%03d).   %s",
										tskno, ++n, graph[tskno-1]);
		consume_time(task_loop);
		c = message[tskno-1];
		message[tskno-1] = 0;
		switch (c) {
		case '4':
			syslog(LOG_INFO, "#%d#act_tsk(TASK1)", tskno);
			SVC_PERROR(act_tsk(TASK1));
			break;
		case '5':
			syslog(LOG_INFO, "#%d#act_tsk(TASK2)", tskno);
			SVC_PERROR(act_tsk(TASK2));
			break;
		case '6':
			syslog(LOG_INFO, "#%d#act_tsk(TASK3)", tskno);
			SVC_PERROR(act_tsk(TASK3));
			break;
		case 'e':
			syslog(LOG_INFO, "#%d#ext_tsk()", tskno);
			SVC_PERROR(ext_tsk());
			assert(0);
		case 's':
			syslog(LOG_INFO, "#%d#slp_tsk()", tskno);
			SVC_PERROR(slp_tsk());
			break;
		case 'S':
			syslog(LOG_INFO, "#%d#tslp_tsk(10000000)", tskno);
			SVC_PERROR(tslp_tsk(10000000));
			break;
		case 'd':
			syslog(LOG_INFO, "#%d#dly_tsk(10000000)", tskno);
			SVC_PERROR(dly_tsk(10000000));
			break;
		case 'y':
			syslog(LOG_INFO, "#%d#dis_ter()", tskno);
			SVC_PERROR(dis_ter());
			break;
		case 'Y':
			syslog(LOG_INFO, "#%d#ena_ter()", tskno);
			SVC_PERROR(ena_ter());
			break;
#ifdef CPUEXC1
		case 'z':
			syslog(LOG_NOTICE, "#%d#raise CPU exception", tskno);
			RAISE_CPU_EXCEPTION;
			break;
		case 'Z':
			SVC_PERROR(loc_cpu());
			syslog(LOG_NOTICE, "#%d#raise CPU exception", tskno);
			RAISE_CPU_EXCEPTION;
			SVC_PERROR(unl_cpu());
			break;
#endif /* CPUEXC1 */
		default:
			break;
		}
	}
}

/*
 *  割込みサービスルーチン
 *
 *  DOM1のHIGH_PRIORITY，MID_PRIORITY，LOW_PRIORITY の各優先度のレディ
 *  キューを回転させる．
 */
#ifdef INTNO1

void
intno1_isr(EXINF exinf)
{
	intno1_clear();
	SVC_PERROR(mrot_rdq(DOM1, HIGH_PRIORITY));
	SVC_PERROR(mrot_rdq(DOM1, MID_PRIORITY));
	SVC_PERROR(mrot_rdq(DOM1, LOW_PRIORITY));
}

#endif /* INTNO1 */

/*
 *  CPU例外ハンドラ
 */
ID	cpuexc_tskid;		/* CPU例外を起こしたタスクのID */

#ifdef CPUEXC1

void
cpuexc_handler(void *p_excinf)
{
	syslog(LOG_NOTICE, "CPU exception handler (p_excinf = %08p).", p_excinf);
	if (sns_ctx() != true) {
		syslog(LOG_WARNING,
					"sns_ctx() is not true in CPU exception handler.");
	}
	if (sns_dpn() != true) {
		syslog(LOG_WARNING,
					"sns_dpn() is not true in CPU exception handler.");
	}
	syslog(LOG_INFO, "sns_loc = %d, sns_dsp = %d, xsns_dpn = %d",
								sns_loc(), sns_dsp(), xsns_dpn(p_excinf));

	if (xsns_dpn(p_excinf)) {
		syslog(LOG_NOTICE, "Sample program ends with exception.");
		SVC_PERROR(ext_ker());
		assert(0);
	}

	SVC_PERROR(get_tid(&cpuexc_tskid));
	SVC_PERROR(act_tsk(EXC_TASK));
}

#endif /* CPUEXC1 */

/*
 *  周期ハンドラ
 *
 *  DOM1のHIGH_PRIORITY，MID_PRIORITY，LOW_PRIORITY の各優先度のレディ
 *  キューを回転させる．
 */
void
cyclic_handler(EXINF exinf)
{
	SVC_PERROR(mrot_rdq(DOM1, HIGH_PRIORITY));
	SVC_PERROR(mrot_rdq(DOM1, MID_PRIORITY));
	SVC_PERROR(mrot_rdq(DOM1, LOW_PRIORITY));
}

/*
 *  アラーム通知で起動されるタスク
 *
 *  自ドメイン（DOM1）のHIGH_PRIORITY，MID_PRIORITY，LOW_PRIORITY の各
 *  優先度のレディキューを回転させる．
 */
void
alarm_task(EXINF exinf)
{
	SVC_PERROR(rot_rdq(HIGH_PRIORITY));
	SVC_PERROR(rot_rdq(MID_PRIORITY));
	SVC_PERROR(rot_rdq(LOW_PRIORITY));
}

/*
 *  例外処理タスク
 */
void
exc_task(EXINF exinf)
{
	SVC_PERROR(ras_ter(cpuexc_tskid));
}

/*
 *  メインタスク
 */
void
main_task(EXINF exinf)
{
	char	c;
	ID		tskid = TASK1;
	int_t	tskno = 1;
	ER_UINT	ercd;
	PRI		tskpri;
#ifndef TASK_LOOP
	SYSTIM	stime1, stime2;
#endif /* TASK_LOOP */
	HRTCNT	hrtcnt1, hrtcnt2;
	T_CTSK	ctsk;
	T_CALM	calm;

	SVC_PERROR(syslog_msk_log(LOG_UPTO(LOG_INFO), LOG_UPTO(LOG_EMERG)));
	syslog(LOG_NOTICE, "Sample program starts (exinf = %d).", (int_t) exinf);

	/*
	 *  シリアルポートの初期化
	 *
	 *  システムログタスクと同じシリアルポートを使う場合など，シリアル
	 *  ポートがオープン済みの場合にはここでE_OBJエラーになるが，支障は
	 *  ない．
	 */
	ercd = serial_opn_por(TASK_PORTID);
	if (ercd < 0 && MERCD(ercd) != E_OBJ) {
		syslog(LOG_ERROR, "%s (%d) reported by `serial_opn_por'.",
									itron_strerror(ercd), SERCD(ercd));
	}
	SVC_PERROR(serial_ctl_por(TASK_PORTID,
							(IOCTL_CRLF | IOCTL_FCSND | IOCTL_FCRCV)));

	/*
 	 *  ループ回数の設定
	 *
	 *  並行実行されるタスク内でのループの回数（task_loop）は，ループ
	 *  の実行時間が約0.2秒になるように設定する．この設定のために，
	 *  LOOP_REF回のループの実行時間を，その前後でget_timを呼ぶことで
	 *  測定し，その測定結果から空ループの実行時間が0.2秒になるループ
	 *  回数を求め，task_loopに設定する．
	 *
	 *  LOOP_REFは，デフォルトでは1,000,000に設定しているが，想定した
	 *  より遅いプロセッサでは，サンプルプログラムの実行開始に時間がか
	 *  かりすぎるという問題を生じる．逆に想定したより速いプロセッサで
	 *  は，LOOP_REF回のループの実行時間が短くなり，task_loopに設定す
	 *  る値の誤差が大きくなるという問題がある．そこで，そのようなター
	 *  ゲットでは，target_test.hで，LOOP_REFを適切な値に定義すること
	 *  とする．
	 *
	 *  また，task_loopの値を固定したい場合には，その値をTASK_LOOPにマ
	 *  クロ定義する．TASK_LOOPがマクロ定義されている場合，上記の測定
	 *  を行わずに，TASK_LOOPに定義された値をループの回数とする．
	 *
	 *  ターゲットによっては，ループの実行時間の1回目の測定で，本来より
	 *  も長めになるものがある．このようなターゲットでは，MEASURE_TWICE
	 *  をマクロ定義することで，1回目の測定結果を捨てて，2回目の測定結
	 *  果を使う．
	 */
#ifdef TASK_LOOP
	task_loop = TASK_LOOP;
#else /* TASK_LOOP */

#ifdef MEASURE_TWICE
	SVC_PERROR(get_tim(&stime1));
	consume_time(LOOP_REF);
	SVC_PERROR(get_tim(&stime2));
#endif /* MEASURE_TWICE */

	SVC_PERROR(get_tim(&stime1));
	consume_time(LOOP_REF);
	SVC_PERROR(get_tim(&stime2));
	task_loop = LOOP_REF * 200LU / (ulong_t)(stime2 - stime1) * 1000LU;

#endif /* TASK_LOOP */

	/*
 	 *  タスクの起動
	 */
	SVC_PERROR(act_tsk(TASK1));
	SVC_PERROR(act_tsk(TASK2));
	SVC_PERROR(chg_som(SOM1));

	/*
 	 *  メインループ
	 */
	do {
		SVC_PERROR(serial_rea_dat(TASK_PORTID, &c, 1));
		switch (c) {
		case '4':
		case '5':
		case '6':
		case 'e':
		case 's':
		case 'S':
		case 'd':
		case 'y':
		case 'Y':
		case 'z':
		case 'Z':
			message[tskno-1] = c;
			break;
		case '1':
			tskno = 1;
			tskid = TASK1;
			break;
		case '2':
			tskno = 2;
			tskid = TASK2;
			break;
		case '3':
			tskno = 3;
			tskid = TASK3;
			break;
		case 'a':
			syslog(LOG_INFO, "#act_tsk(%d)", tskno);
			SVC_PERROR(act_tsk(tskid));
			break;
		case 'A':
			syslog(LOG_INFO, "#can_act(%d)", tskno);
			SVC_PERROR(ercd = can_act(tskid));
			if (ercd >= 0) {
				syslog(LOG_NOTICE, "can_act(%d) returns %d", tskno, ercd);
			}
			break;
		case 't':
			syslog(LOG_INFO, "#ter_tsk(%d)", tskno);
			SVC_PERROR(ter_tsk(tskid));
			break;
		case '>':
			syslog(LOG_INFO, "#chg_pri(%d, HIGH_PRIORITY)", tskno);
			SVC_PERROR(chg_pri(tskid, HIGH_PRIORITY));
			break;
		case '=':
			syslog(LOG_INFO, "#chg_pri(%d, MID_PRIORITY)", tskno);
			SVC_PERROR(chg_pri(tskid, MID_PRIORITY));
			break;
		case '<':
			syslog(LOG_INFO, "#chg_pri(%d, LOW_PRIORITY)", tskno);
			SVC_PERROR(chg_pri(tskid, LOW_PRIORITY));
			break;
		case 'G':
			syslog(LOG_INFO, "#get_pri(%d, &tskpri)", tskno);
			SVC_PERROR(ercd = get_pri(tskid, &tskpri));
			if (ercd >= 0) {
				syslog(LOG_NOTICE, "priority of task %d is %d", tskno, tskpri);
			}
			break;
		case 'w':
			syslog(LOG_INFO, "#wup_tsk(%d)", tskno);
			SVC_PERROR(wup_tsk(tskid));
			break;
		case 'W':
			syslog(LOG_INFO, "#can_wup(%d)", tskno);
			SVC_PERROR(ercd = can_wup(tskid));
			if (ercd >= 0) {
				syslog(LOG_NOTICE, "can_wup(%d) returns %d", tskno, ercd);
			}
			break;
		case 'l':
			syslog(LOG_INFO, "#rel_wai(%d)", tskno);
			SVC_PERROR(rel_wai(tskid));
			break;
		case 'u':
			syslog(LOG_INFO, "#sus_tsk(%d)", tskno);
			SVC_PERROR(sus_tsk(tskid));
			break;
		case 'm':
			syslog(LOG_INFO, "#rsm_tsk(%d)", tskno);
			SVC_PERROR(rsm_tsk(tskid));
			break;
		case 'x':
			syslog(LOG_INFO, "#ras_ter(%d)", tskno);
			SVC_PERROR(ras_ter(tskid));
			break;
		case 'r':
			syslog(LOG_INFO, "#rot_rdq(three priorities)");
			SVC_PERROR(mrot_rdq(DOM1, HIGH_PRIORITY));
			SVC_PERROR(mrot_rdq(DOM1, MID_PRIORITY));
			SVC_PERROR(mrot_rdq(DOM1, LOW_PRIORITY));
			break;
		case 'c':
			syslog(LOG_INFO, "#sta_cyc(CYCHDR1)");
			SVC_PERROR(sta_cyc(CYCHDR1));
			break;
		case 'C':
			syslog(LOG_INFO, "#stp_cyc(CYCHDR1)");
			SVC_PERROR(stp_cyc(CYCHDR1));
			break;
		case 'b':
			syslog(LOG_INFO, "#sta_alm(ALMHDR1, 5000000)");
			SVC_PERROR(sta_alm(ALMHDR1, 5000000));
			break;
		case 'B':
			syslog(LOG_INFO, "#stp_alm(ALMHDR1)");
			SVC_PERROR(stp_alm(ALMHDR1));
			break;
		case 'j':
			syslog(LOG_INFO, "#chg_som(SOM2)");
			SVC_PERROR(chg_som(SOM2));
			break;
		case 'J':
			syslog(LOG_INFO, "#chg_som(SOM1)");
			SVC_PERROR(chg_som(SOM1));
			break;
		case '@':
			ctsk.tskatr = TA_DOM(DOM2);
			ctsk.exinf = 3;
			ctsk.task = task;
			ctsk.itskpri = MID_PRIORITY;
			ctsk.stksz = STACK_SIZE;
			ctsk.stk = (STK_T *) user_stack_TASK3;
			ctsk.sstksz = STACK_SIZE;
			ctsk.sstk = NULL;
			SVC_PERROR(ercd = acre_tsk(&ctsk));
			if (ercd >= 0) {
				TASK3 = ercd;
				syslog(LOG_NOTICE, "task3 is created with tskid = %d.",
														(int_t) TASK3);
			}
			break;
		case '!':
			syslog(LOG_INFO, "#del_tsk(%d)", tskno);
			SVC_PERROR(del_tsk(tskid));
			break;
		case '$':
			calm.almatr = TA_DOM(DOM1);
			calm.nfyinfo.nfymode = TNFY_ACTTSK;
			calm.nfyinfo.nfy.acttsk.tskid = ALM_TASK;
			SVC_PERROR(ercd = acre_alm(&calm));
			if (ercd >= 0) {
				ALMHDR1 = ercd;
				syslog(LOG_NOTICE, "alarm handler is created with almid = %d.",
														(int_t) ALMHDR1);
			}
			break;
		case '#':
			syslog(LOG_INFO, "#del_alm(ALMHDR1)");
			SVC_PERROR(del_alm(ALMHDR1));
			break;

		case 'V':
			hrtcnt1 = fch_hrt();
			consume_time(1000LU);
			hrtcnt2 = fch_hrt();
			syslog(LOG_NOTICE, "hrtcnt1 = %tu, hrtcnt2 = %tu",
								(uint32_t) hrtcnt1, (uint32_t) hrtcnt2);
			break;

		case 'v':
			SVC_PERROR(syslog_msk_log(LOG_UPTO(LOG_INFO),
										LOG_UPTO(LOG_EMERG)));
			break;
		case 'q':
			SVC_PERROR(syslog_msk_log(LOG_UPTO(LOG_NOTICE),
										LOG_UPTO(LOG_EMERG)));
			break;

		case '\003':
		case 'Q':
			break;

		default:
			syslog(LOG_INFO, "Unknown command: '%c'.", c);
			break;
		}
	} while (c != '\003' && c != 'Q');

	syslog(LOG_NOTICE, "Sample program ends.");
	SVC_PERROR(ext_ker());
	assert(0);
}
