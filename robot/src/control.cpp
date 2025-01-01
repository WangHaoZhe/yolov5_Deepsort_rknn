#include "control.h"

#include "common.h"

#include "pid.h"
#include "motor.h"
#include "chassis.h"

#include <mutex>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

extern std::mutex mtxQueueOutput;
extern std::queue<imageout_idx> queueOutput; // output queue 目标追踪输出队列
extern detect_result_group_t result;
int i2c_file;

Motor CMFL(&i2c_file, 0);
Motor CMFR(&i2c_file, 2);
Motor CMBL(&i2c_file, 1);
Motor CMBR(&i2c_file, 3);
Chassis chassis(&CMFL, &CMFR, &CMBL, &CMBR, PID(0.005, 0, 0, 1, 1));

void controlInit() {
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", 2);
    i2c_file = open(filename, O_RDWR);
    if (i2c_file < 0) {
        std::cout << "I2C device not found" << std::endl;
    }
    if (ioctl(i2c_file, I2C_SLAVE, 0x2B) < 0) {
        std::cout << "Robot connection error" << std::endl;
    }
}

int deadBand(int val, const int& min, const int& max) {
    if (val < max && val > min) {
        val = 0;
    }
    return val;
}

void controlLoop() {
    static int id = -1;
    if (id != -1) {
        if (result.count != 0) {
            for (auto det_result: result.results) {
                if (det_result.trackID == id) {
                    int x = (det_result.x1 + det_result.x2) / 2 - NET_INPUTWIDTH / 2;
                    int y = (det_result.y1 + det_result.y2) / 2 - NET_INPUTHEIGHT / 2;
                    int d = det_result.x2 - det_result.x1;
                    int h = det_result.y2 - det_result.y1;
                    int class_id = det_result.classID;

                    std::cout << "Target position: " << x << ", " << h << std::endl;

                    x = deadBand(x, -100, 100);
                    h -= 320;
                    h = deadBand(h, -50, 50);
                    chassis.follow(x, h);
                    break;
                }
                chassis.follow(0, 0);
            }
            chassis.handle();
            result.count = 0;
        }
    } else {
        std::cout << "Enter an id" << std::endl;
        std::cin >> id;
    }
}


void controlTask(int cpuid) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        std::cerr << "set thread affinity failed" << std::endl;

    printf("Bind control process to CPU %d\n", cpuid);

    controlInit();

    while (1) {
        controlLoop();
    }
}
