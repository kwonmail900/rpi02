// random_simulator.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main() {
    srand(time(NULL));
    int base_value = 2400;
    int fluctuation = 20;

    // 출력이 버퍼에 남지 않고 바로바로 전송되도록 설정합니다. (중요)
    setbuf(stdout, NULL);

    while (1) {
        int random_offset = (rand() % (fluctuation * 2 + 1)) - fluctuation;
        int current_sensor_value = base_value + random_offset;
        printf("%d\n", current_sensor_value);
        usleep(500000); // 0.5초 대기
    }
    return 0;
}

