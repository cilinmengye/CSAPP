#include <stdio.h>
#include <signal.h>
#include <unistd.h> // 或者 #include <windows.h>（根据您的操作系统选择）


int main() {
    int i = 1;
    printf("%d\n",-i);
    return 0;
}