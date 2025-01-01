#include "pid.h"
#include <cmath>

float limit(float val, float min, float max) {
  if (min > max)
    return val;
  if (val < min)
    val = min;
  else if (val > max)
    val = max;
  return val;
}

PID::PID(float kp, float ki, float kd, float i_max, float out_max)
    : kp_(kp),
      ki_(ki),
      kd_(kd),
      i_max_(i_max),
      out_max_(out_max){
  err_ = 0;
  err_sum_ = 0;
  last_err_ = 0;
}

void PID::reset() {
  err_ = 0;
  err_sum_ = 0;
  last_err_ = 0;
  pout_ = 0;
  iout_ = 0;
  dout_ = 0;
}

float PID::calc(float ref, float fdb) {
  ref_ = ref;
  fdb_ = fdb;

  last_err_ = err_;
  err_ = ref_ - fdb_;
  err_sum_ = limit(err_sum_ + err_, -i_max_, i_max_);

  pout_ = kp_ * err_;
  iout_ = ki_ * err_sum_;
  dout_ = kd_ * (err_ - last_err_);
  output_ = limit(pout_ + iout_ + dout_, -out_max_, out_max_);

  return output_;
}
