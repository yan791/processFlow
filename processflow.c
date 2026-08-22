#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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

void executar_parallel(Task **tasks, int n);
void cmd_jobs(char *args[], int n);
void coletar_jobs_terminados(void);

int procurar_task(char *nome) {
    for (int i = 0; i < quant_tasks; i++) {
        if (strcmp(tarefas[i].nome, nome) == 0) {
            return i;
        }
    }

    return -1;
}

int separar_argumentos(char *linha, char *args[]) {
    int n = 0;
    char *palavra = strtok(linha, " \t\n");

    while (palavra != NULL && n < MAX_ARGS - 1) {
        args[n++] = palavra;
        palavra = strtok(NULL, " \t\n");
    }

    args[n] = NULL;
    return n;
}

void cadastrar_task(char *args[], int n) {
    if (n < 3) {
        printf("Uso: task nome programa [argumentos]\n");
        return;
    }

    if (quant_tasks >= MAX_TASKS) {
        printf("Limite de tarefas atingido.\n");
        return;
    }

    if (n - 3 > MAX_ARGS - 2) {
        printf("Numero de argumentos excede o limite (%d).\n", MAX_ARGS - 1);
        return;
    }

    Task *t = &tarefas[quant_tasks];

    strncpy(t->nome, args[1], sizeof(t->nome) - 1);
    t->nome[sizeof(t->nome) - 1] = '\0';

    strncpy(t->programa, args[2], sizeof(t->programa) - 1);
    t->programa[sizeof(t->programa) - 1] = '\0';

    t->quant_args = n - 3;

    for (int i = 3; i < n; i++) {
        strncpy(t->argumentos[i - 3], args[i],
                sizeof(t->argumentos[i - 3]) - 1);
        t->argumentos[i - 3][sizeof(t->argumentos[i - 3]) - 1] = '\0';
    }

    quant_tasks++;
    printf("Tarefa '%s' cadastrada.\n", t->nome);
}

void montar_exec_args(Task *task, char *args[]) {
    args[0] = task->programa;

    for (int i = 0; i < task->quant_args; i++) {
        args[i + 1] = task->argumentos[i];
    }

    args[task->quant_args + 1] = NULL;
}

pid_t spawn(Task *task) {
    char *args[MAX_ARGS];
    montar_exec_args(task, args);

    pid_t pid = fork();

    if (pid == -1) {
        perror("Erro no fork");
        return -1;
    }

    if (pid == 0) {
        execvp(task->programa, args);
        perror("Erro ao executar tarefa");
        exit(127);
    }

    return pid;
}

void executar_task(Task *task) {
    pid_t pid = spawn(task);

    if (pid <= 0) {
        return;
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        printf("Tarefa '%s' terminou com codigo %d.\n",
               task->nome, WEXITSTATUS(status));
    } 
    else if (WIFSIGNALED(status)) {
        printf("Tarefa '%s' terminou pelo sinal %d.\n",
               task->nome, WTERMSIG(status));
    }
}

void comando_run(char *args[], int n) {
    if (n < 2) {
        printf("Uso: run tarefa\n");
        return;
    }

    if (strcmp(args[1], "sequential") == 0) {
        for (int i = 2; i < n; i++) {
            int idx = procurar_task(args[i]);

            if (idx == -1) {
                printf("Tarefa '%s' nao existe.\n", args[i]);
            } 
            else {
                executar_task(&tarefas[idx]);
            }
        }

        return;
    }

    if (strcmp(args[1], "parallel") == 0) {
        Task *lista[MAX_ARGS];
        int total = 0;

        for (int i = 2; i < n; i++) {
            int idx = procurar_task(args[i]);

            if (idx == -1) {
                printf("Tarefa '%s' nao existe.\n", args[i]);
            } 
            else {
                lista[total++] = &tarefas[idx];
            }
        }

        if (total > 0) {
            executar_parallel(lista, total);
        }

        return;
    }

    int idx = procurar_task(args[1]);

    if (idx == -1) {
        printf("Tarefa '%s' nao existe.\n", args[1]);
    } 
    else {
        executar_task(&tarefas[idx]);
    }
}

void executar_parallel(Task **tasks, int n) {
    pid_t pids[MAX_ARGS];
    char *nomes[MAX_ARGS];
    int total = 0;

    for (int i = 0; i < n; i++) {
        pid_t pid = spawn(tasks[i]);

        if (pid > 0) {
            pids[total] = pid;
            nomes[total] = tasks[i]->nome;
            total++;
        }
    }

    for (int i = 0; i < total; i++) {
        int status;
        waitpid(pids[i], &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("Tarefa '%s' terminou com codigo %d.\n",nomes[i], WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            printf("Tarefa '%s' terminou pelo sinal %d.\n",nomes[i], WTERMSIG(status));
        }
    }
}

void registrar_job(pid_t pid, char *nome_task) {
    if (quant_jobs >= MAX_JOBS) {
        printf("Limite de jobs atingido.\n");
        return;
    }

    Job *j = &jobs[quant_jobs];
    j->id = prox_job;
    j->pid = pid;
    j->ativo = 1;

    strncpy(j->nome_task, nome_task, sizeof(j->nome_task) - 1);
    j->nome_task[sizeof(j->nome_task) - 1] = '\0';

    printf("[%d] %d\n", prox_job, pid);

    quant_jobs++;
    prox_job++;
}

void iniciar_task(Task *task) {
    pid_t pid = spawn(task);

    if (pid > 0) {
        registrar_job(pid, task->nome);
    }
}

void mostrar_jobs(void) {
    for (int i = 0; i < quant_jobs; i++) {
        if (jobs[i].ativo) {
            printf("[%d] %d Running %s\n",jobs[i].id,jobs[i].pid,jobs[i].nome_task);
        } 
        else {
            printf("[%d] Finalizado %s\n",jobs[i].id,jobs[i].nome_task);
        }
    }
}

void cmd_start(char *args[], int n) {
    if (n < 2) {
        printf("uso: start tarefa\n");
        return;
    }

    int idx = procurar_task(args[1]);

    if (idx == -1) {
        printf("tarefa nao existe.\n");
    } 
    else {
        iniciar_task(&tarefas[idx]);
    }
}

void cmd_jobs(char *args[], int n) {
    (void)args;
    (void)n;
    mostrar_jobs();
}

void handler_sigchld(int sig) {
    (void)sig;
    chegou_sigchld = 1;
}

void coletar_jobs_terminados(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < quant_jobs; i++) {
            if (jobs[i].pid == pid && jobs[i].ativo) {
                jobs[i].ativo = 0;

                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    printf("[%d] Finalizado (codigo %d) %s\n",
                           jobs[i].id,
                           WEXITSTATUS(status),
                           jobs[i].nome_task);
                } else if (WIFSIGNALED(status)) {
                    printf("[%d] Finalizado (sinal %d) %s\n",
                           jobs[i].id,
                           WTERMSIG(status),
                           jobs[i].nome_task);
                } else {
                    printf("[%d] Finalizado %s\n",
                           jobs[i].id,
                           jobs[i].nome_task);
                }

                break;
            }
        }
    }
}

int main(void) {
    char linha[MAX_LINE];
    char *args[MAX_ARGS];
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Erro ao configurar SIGCHLD");
        return 1;
    }

    printf("ProcessFlow iniciado.\n");

    while (1) {
        if (chegou_sigchld) {
            chegou_sigchld = 0;
            coletar_jobs_terminados();
        }

        printf("processflow> ");
        fflush(stdout);

        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            break;
        }

        int n = separar_argumentos(linha, args);

        if (n == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "task") == 0) {
            cadastrar_task(args, n);
        } 
        else if (strcmp(args[0], "run") == 0) {
            comando_run(args, n);
        } 
        else if (strcmp(args[0], "start") == 0) {
            cmd_start(args, n);
        } 
        else if (strcmp(args[0], "jobs") == 0) {
            cmd_jobs(args, n);
        } 
        else {
            printf("comando nao reconhecido.\n");
        }
    }
    return 0;
}