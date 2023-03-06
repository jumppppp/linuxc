#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mytbf.h"
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
// typedef void (*sighandler_t)(int);
static struct mytbf_st *job[MYTBF_MAX];
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER;
static int inited = 0;
static pthread_t tid_alarm;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;
// static sighandler_t alarm_save;
// struct sigaction al_save;
struct mytbf_st
{

	int cps;
	int burst;
	int token;
	int pos;
	pthread_mutex_t mut;
};
static int get_free_pos_unlocked(void)
{

	for (int i = 0; i < MYTBF_MAX; i++)
	{
		if (job[i] == NULL)
		{
			return i;
		}
	}
	return -1;
}

static void module_unload(void)
{

	// signal(SIGALRM, alarm_save);
	// alarm(0);
	// struct sigaction sa;
	// struct itimerval itv;
	// int ret;
	// ret = sigaction(SIGALRM, &al_save, NULL);
	// if (ret < 0)
	// {
	// 	fprintf(stderr, "sigaction()");
	// }
	// /* if error*/
	// itv.it_interval.tv_sec = 0;
	// itv.it_interval.tv_usec = 0;
	// itv.it_value.tv_sec = 0;
	// itv.it_value.tv_usec = 0;
	// ret = setitimer(ITIMER_REAL, &itv, NULL);
	// if (ret < 0)
	// {
	// 	fprintf(stderr, "sigemptyset()");
	// }
	pthread_cancel(tid_alarm);
	pthread_join(tid_alarm, NULL);
	for (int i = 0; i < MYTBF_MAX; i++)
	{
		if (job[i] != NULL)
		{
			pthread_mutex_destroy(&job[i]->mut);
			free(job[i]);
		}
	}
	pthread_mutex_destroy(&mut_job);
}

static void *thr_alarm(void *p)
{
	// alarm(1);
	// if (infop->si_code != SI_KERNEL)
	// 	return;

	while (1)
	{
		for (int i = 0; i < MYTBF_MAX; i++)
		{
			pthread_mutex_lock(&mut_job);
			if (job[i] != NULL)
			{
				pthread_mutex_lock(&job[i]->mut);
				job[i]->token += job[i]->cps;
				if (job[i]->token > job[i]->burst)
				{
					job[i]->token = job[i]->burst;
				}
				pthread_mutex_unlock(&job[i]->mut);
			}
			pthread_mutex_unlock(&mut_job);
		}
		sleep(1);
	}
}
static void module_load(void)
{

	int err;
	err = pthread_create(&tid_alarm, NULL, thr_alarm, NULL);
	if (err)
	{
		fprintf(stderr, "pthread_create():%s\n", strerror(err));
		exit(1);
	}
	// struct sigaction sa;
	// struct itimerval itv;
	// int ret;
	// sa.sa_sigaction = h1;
	// sigemptyset(&sa.sa_mask);
	// sa.sa_flags = SA_SIGINFO;
	// ret = sigaction(SIGALRM, &sa, &al_save);
	// if (ret < 0)
	// {
	// 	fprintf(stderr, "sigaction()");
	// }
	// itv.it_interval.tv_sec = 1;
	// itv.it_interval.tv_usec = 0;
	// itv.it_value.tv_sec = 1;
	// itv.it_value.tv_usec = 0;
	// ret = setitimer(ITIMER_REAL, &itv, NULL);
	// if (ret < 0)
	// {
	// 	fprintf(stderr, "setitimer");
	// }
	// alarm_save = signal(SIGALRM, h1);
	// alarm(1);
	atexit(module_unload);
}

mytbf_t *mytbf_init(int cps, int burst)
{
	// if (!inited)
	// {
	// 	module_load();
	// 	inited = 1;
	// }
	pthread_once(&init_once, module_load);
	int pos;
	struct mytbf_st *me;
	me = malloc(sizeof(*me));
	if (me == NULL)
	{
		return NULL;
	}
	me->token = 0;
	me->cps = cps;
	me->burst = burst;
	me->pos = pos;
	pthread_mutex_init(&me->mut, NULL);
	pthread_mutex_lock(&mut_job);
	pos = get_free_pos_unlocked();
	if (pos < 0)
	{
		free(me);
		pthread_mutex_unlock(&mut_job);
		return NULL;
	}
	job[pos] = me;
	pthread_mutex_unlock(&mut_job);
	return me;
}

static int min(int a, int b)
{

	if (a < b)
		return a;
	return b;
}
int mytbf_fetchtoken(mytbf_t *ptr, int size)
{
	struct mytbf_st *me = ptr;
	int n;
	if (size <= 0)
	{
		return -EINVAL;
	}
	pthread_mutex_lock(&me->mut);
	while (me->token <= 0)
	{
		pthread_mutex_unlock(&me->mut);
		sched_yield();
		// pause();
		pthread_mutex_lock(&me->mut);
	}

	n = min(me->token, size);
	me->token -= n;
	pthread_mutex_unlock(&me->mut);
	return n;
}
int mytbf_returntoken(mytbf_t *ptr, int size)
{
	struct mytbf_st *me = ptr;

	if (size <= 0)
	{
		return -EINVAL;
	}
	pthread_mutex_lock(&me->mut);
	me->token += size;
	if (me->token > me->burst)
	{
		me->token = me->burst;
	}
	pthread_mutex_unlock(&me->mut);
	return size;
}

int mytbf_destroy(mytbf_t *ptr)
{

	struct mytbf_st *me = ptr;
	pthread_mutex_lock(&mut_job);
	job[me->pos] = NULL;
	pthread_mutex_unlock(&mut_job);
	pthread_mutex_destroy(&me->mut);
	free(ptr);
	return 0;
}