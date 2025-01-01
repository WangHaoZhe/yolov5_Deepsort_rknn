#ifndef PID_H
#define PID_H

class PID {
  public:
    PID() : PID(0, 0, 0, 0, 0) {}
    PID(float kp, float ki, float kd, float i_max, float out_max);

    void reset();

    float calc(float ref, float fdb);

  public:
    float kp_, ki_, kd_;
    float i_max_, out_max_;
    float output_{};

  protected:
    float ref_{}, fdb_{};
    float err_, err_sum_, last_err_;
    float pout_{}, iout_{}, dout_{};
};

#endif //PID_H
