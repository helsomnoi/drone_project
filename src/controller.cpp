#include "controller.h"

#include <cmath>
#include <algorithm>

double PIDController::Update(double err, double dt)
{
    integral_ += err * dt;
    integral_ = Saturation(integral_, integral_max_);

    double derivative = (err - prev_err_) / dt;
    prev_err_ = err;

    return kp_ * err + ki_ * integral_ + kd_ * derivative;
}

void PIDController::Reset()
{
    integral_ = 0.0;
    prev_err_ = 0.0;
}

double PIController::Update(double err, double dt)
{
    integral_ += err * dt;
    integral_ = Saturation(integral_, integral_max_);

    return kp_ * err + ki_ * integral_;
}

void PIController::Reset()
{
    integral_ = 0.0;
}

double PController::Update(double err, double dt)
{
    return kp_ * err;
}

double Saturation( double a, double a_max){
    return std::max(std::min(a, a_max), -a_max);
}

