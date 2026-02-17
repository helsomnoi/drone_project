#pragma once

#include <optional>

//Interface for PID, PI, P contollers
class Controller {
public:
    virtual ~Controller() = default;
    virtual double Update(double err, double dt) = 0;
};

class PIDController : Controller {
public:
    PIDController(double kp, double ki, double kd, double integral_max, double diff_max)
    : kp_(kp), ki_(ki), kd_(kd), integral_max_(integral_max), diff_max_(diff_max) {}

    double Update(double err, double dt) override;

    void Reset();
    
private:
    double kp_, ki_, kd_;
    double integral_max_;
    double integral_ = 0.0; // sum of errors
    double diff_max_;
    double prev_err_ = 0.0; // previous error
};

class PIController : Controller {
public:
    PIController(double kp, double ki, double integral_max)
    : kp_(kp), ki_(ki), integral_max_(integral_max) {}

    double Update(double err, double dt) override;

    void Reset();
    
private:
    double kp_, ki_;
    double integral_max_ = 0.0;
    double integral_ = 0.0; 
};

class PController : Controller {
public:
    PController(double kp)
    : kp_(kp) {}

    double Update(double err, double dt) override;

    void Reset();
    
private:
    double kp_;
};

double Saturation( double a, double a_max);