#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

class Motor {
  public:
    Motor(const int* i2c_file, const uint8_t id);

    void reset();

    void handle();

    void setSpeed(float speed);

  public:
    const int* i2c_file_;
    uint8_t id_;
    uint8_t direction_;
    uint8_t speed_;

  private:
    uint8_t buf_[3]{};
};

#endif //MOTOR_H
