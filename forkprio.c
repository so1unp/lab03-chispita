#define _POSIX_C_SOURCE 200809L // para struct sigaction
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/types.h> // para pid_t
#include <sys/times.h>      // times()
#include <sys/resource.h>
#include <sys/wait.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigterm(int sig){
    keep_running = 0;
}

int busywork(void)
{
    struct tms buf;
    for (; keep_running ;) {
        times(&buf);
    }
}

int main(int argc, char *argv[])
{
    if(argc < 4){
        fprintf(stderr, "Uso %s <n_hijos> <segundos> <cambiar_prio>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_hijos = atoi(argv[1]);
    int segs = atoi(argv[2]);
    int cambiar_prio = atoi(argv[3]);
    pid_t pids[n_hijos];

    struct sigaction sa;
    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    for(int i = 0; i < n_hijos; i++){
        pids[i] = fork();

        if(pids[i] < 0){
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }

        if(pids[i] == 0){
            if(cambiar_prio){
                setpriority(PRIO_PROCESS, 0, i);
            }
            int prio_actual = getpriority(PRIO_PROCESS, 0);
            busywork();

            struct rusage usage;
            getrusage(RUSAGE_SELF, &usage);

            long total_cpu_time = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec;

            printf("Hijo %d (nice %2d): \t%3li\n", getpid(), prio_actual, total_cpu_time);
            exit(EXIT_SUCCESS);
        }
    }
    if(segs > 0){
        sleep(segs);
    }else{
        while(1){
            pause();
        }
    }
    for(int i = 0; i < n_hijos; i++){
        kill(pids[i], SIGTERM);
    }
    for(int i = 0; i < n_hijos; i++){
        wait(NULL);
    }

    exit(EXIT_SUCCESS);
}