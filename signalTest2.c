#include <stdio.h>
#include <signal.h>
#include <unistd.h> // 或者 #include <windows.h>（根据您的操作系统选择）

void handler(int sig){
    return ;
}

unsigned int snooze(unsigned int secs){
    unsigned int rc = sleep(secs);

    printf ("Slept for %d of %d secs\n", secs-rc, secs);
    return rc;
}

int main(int argc, char *argv[]) {
    // 注册 SIGINT 信号处理函数
    signal(SIGINT, handler);

    printf("Waiting for SIGINT signal...\n");
    // 等待信号到来
    while (1) {
        // 这里可以添加其他任务
        printf("signal have come?\n");
        sleep(1); // 或者 Sleep(1000);（根据您的操作系统选择）
    }

    return 0;
}