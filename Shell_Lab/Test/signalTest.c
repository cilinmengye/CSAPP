#include <stdio.h>
#include <signal.h>
#include <unistd.h> // 或者 #include <windows.h>（根据您的操作系统选择）

// SIGINT 信号处理函数
void sigint_handler(int signum) {
    printf("Caught SIGINT signal\n");
    // 这里可以添加您想要执行的操作，比如关闭文件、释放资源等
    // 请注意，信号处理函数应该尽量保持简单和快速，避免执行耗时操作
}

int main() {
    // 注册 SIGINT 信号处理函数
    signal(SIGINT, sigint_handler);

    printf("Waiting for SIGINT signal...\n");
    // 等待信号到来
    while (1) {
        // 这里可以添加其他任务
        printf("signal have come?\n");
        sleep(1); // 或者 Sleep(1000);（根据您的操作系统选择）
    }

    return 0;
}