#include "chassis.h"

const float wheel_radius = 0.03f;

Chassis::Chassis(Motor* cmfl, Motor* cmfr, Motor* cmbl, Motor* cmbr, const PID& pid)
    : cmfl_(cmfl),
      cmfr_(cmfr),
      cmbl_(cmbl),
      cmbr_(cmbr),
      pid_(pid) {}

void Chassis::follow(float x, float h) {
    status_.vx = pid_.calc(0, h);
    status_.vy = pid_.calc(0, x);
}

void Chassis::handle() {
    ikine();

    cmfl_->setSpeed(status_.wheel_speed.fl);
    cmfr_->setSpeed(status_.wheel_speed.fr);
    cmbl_->setSpeed(status_.wheel_speed.bl);
    cmbr_->setSpeed(status_.wheel_speed.br);

    cmfl_->handle();
    cmfr_->handle();
    cmbl_->handle();
    cmbr_->handle();
}

void Chassis::ikine() {
    status_.wheel_speed.fl = (-status_.wz + status_.vx - status_.vy) / wheel_radius;
    status_.wheel_speed.fr = -(-status_.wz - status_.vx - status_.vy) / wheel_radius;
    status_.wheel_speed.bl = (-status_.wz + status_.vx + status_.vy) / wheel_radius;
    status_.wheel_speed.br = -(-status_.wz - status_.vx + status_.vy) / wheel_radius;
}
