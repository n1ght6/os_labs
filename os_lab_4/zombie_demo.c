#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Демонстрация зомби-процессов\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        printf("Дочерний процесс завершается, но родитель не вызывает wait()\n");
        exit(0);
    } else {
        
        printf("Родительский процесс: PID = %d, создал дочерний с PID = %d\n", getpid(), pid);
        printf("Родительский процесс спит 30 секунд...\n");
        sleep(30);
        
        printf("Теперь родитель вызывает wait()...\n");
        wait(NULL);
        printf("Зомби-процесс убран\n");
        sleep(5);
    }
    
    return 0;
}
