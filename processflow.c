#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_TASKS 100
#define MAX_JOBS  100
#define MAX_ARGS  20
#define MAX_LINE  1024

typedef struct {
    char nome[50];
    char programa[100];
    char argumentos[MAX_ARGS][100];
    int  quant_args;
} Task;

typedef struct {
    int id;
    pid_t pid;
    char nome_task[50];
    int ativo;
} Job;

Task tarefas[MAX_TASKS];
Job jobs[MAX_JOBS];
int quant_tasks = 0;
int quant_jobs = 0;
int prox_job = 1;

volatile sig_atomic_t chegou_sigchld = 0;

int main(void) {
    printf("ProcessFlow iniciado.\n");
    return 0;
}
