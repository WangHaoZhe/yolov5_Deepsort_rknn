#ifndef CHASSIS_H
#define CHASSIS_H

#include "motor.h"
#include "pid.h"

typedef struct ChassisStatus {
  float vx = 0;  // x方向速度(m/s)
  float vy = 0;  // y方向速度(m/s)
  float wz = 0;  // 旋转角速度(dps)

  struct WheelSpeed {  // 车轮转速(dps)
    float fl = 0;
    float fr = 0;
    float bl = 0;
    float br = 0;
  } wheel_speed;
} ChassisStatus_t;

class Chassis {
  public:
    Chassis(Motor* cmfl, Motor* cmfr, Motor* cmbl, Motor* cmbr, const PID& pid);

    void follow(float x, float h);

    void handle();

    void ikine();

  public:
    Motor *cmfl_, *cmfr_, *cmbl_, *cmbr_;
    PID pid_;
    ChassisStatus status_;
};

#endif //CHASSIS_H
