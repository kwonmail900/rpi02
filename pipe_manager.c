// pipe_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipe_fd[2]; // 파이프의 파일 디스크립터를 저장할 배열. pipe_fd[0]은 읽기, pipe_fd[1]은 쓰기.
    pid_t pid;      // 프로세스 ID를 저장할 변수
    char buffer[128]; // 파이프에서 읽은 데이터를 저장할 버퍼
    ssize_t bytes_read; // 실제로 읽은 바이트 수를 저장

    // 1. 파이프 생성
    if (pipe(pipe_fd) == -1) {
        perror("pipe"); // pipe 생성 실패 시 에러 출력
        exit(EXIT_FAILURE);
    }

    // 2. 자식 프로세스 생성
    pid = fork();
    if (pid == -1) {
        perror("fork"); // fork 실패 시 에러 출력
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // --- 여기는 자식 프로세스의 코드 영역입니다 ---

        // 3. 자식은 파이프의 읽기 쪽은 사용하지 않으므로 닫아줍니다.
        close(pipe_fd[0]);

        // 4. 자식의 표준 출력을 파이프의 쓰기 쪽으로 재지정합니다.
        // 이제 자식 프로세스에서 printf 등으로 출력하는 모든 것은 화면이 아닌 파이프로 들어갑니다.
        dup2(pipe_fd[1], STDOUT_FILENO);

        // 표준 출력으로 리디렉션했으므로, 원본 쓰기 디스크립터도 닫아줍니다.
        close(pipe_fd[1]);

        // 5. 'random_simulator' 프로그램을 실행합니다.
        // execlp는 현재 프로세스를 지정된 프로그램으로 대체합니다.
        execlp("./random_simulator", "random_simulator", NULL);
        
        // execlp가 정상적으로 실행되면 아래 코드는 절대 실행되지 않습니다.
        // 만약 실행파일이 없거나 오류가 발생하면 아래 코드가 실행됩니다.
        perror("execlp");
        exit(EXIT_FAILURE);

    } else {
        // --- 여기는 부모(상위) 프로세스의 코드 영역입니다 ---

        // 6. 부모는 파이프의 쓰기 쪽은 사용하지 않으므로 닫아줍니다.
        close(pipe_fd[1]);

        //printf("상위 프로세스: 자식 프로세스로부터 센서 값을 수신합니다...\n");

        // 7. 파이프의 읽기 쪽에서 데이터가 들어올 때까지 읽습니다.
        // 자식 프로세스가 종료되어 파이프의 쓰기 쪽이 완전히 닫히면, read는 0을 반환하고 루프가 종료됩니다.
        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            // 읽어온 데이터의 끝에 NULL 문자를 추가하여 안전한 문자열로 만듭니다.
            buffer[bytes_read] = '\0';
            // 수신한 데이터를 화면에 출력합니다.
            printf("%s", buffer);
        }

        // 8. 파이프의 읽기 쪽을 닫습니다.
        close(pipe_fd[0]);
        // 자식 프로세스가 종료될 때까지 기다립니다.
        wait(NULL);
        printf("상위 프로세스: 자식 프로세스 종료됨.\n");
    }

    return 0;
}

