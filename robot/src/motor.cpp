#include "motor.h"
#include <cstring>
#include <cmath>
#include "i2c/smbus.h"

Motor::Motor(const int* i2c_file, const uint8_t id)
    : i2c_file_(i2c_file),
      id_(id) {
    direction_ = 0;
    speed_ = 0;
}

void Motor::reset() {
    direction_ = 0;
    speed_ = 0;
}

void Motor::handle() {
    buf_[0] = id_;
    buf_[1] = direction_;
    buf_[2] = speed_;
    i2c_smbus_write_i2c_block_data(*i2c_file_, 0x01, 3, buf_);
}

void Motor::setSpeed(float speed) {
    direction_ = speed > 0 ? 0 : 1;
    speed_ = abs(speed);
}
