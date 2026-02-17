#pragma once

#include "controller.h"

const double THRESHOLD_HIGHT_ERR = 0.1; 
const double DEFAULT_U = 0.5;
const double MAX_U = 1;

struct HeightControlCoef
{
    double pi_H_kp = 1.5;
    double pi_H_ki = 0.2;
    double pi_H_integral_max = 3.0;  // [m]
    
    double p_Vh_kp = 0.8;
    
    double max_Vh = 4.0;  // [m/s]
    double U_max = 1.0;    
};


class HeightControler {
public:

    HeightControler(const HeightControlCoef& ctrl_coef, double target_H = 0.0);

    double GetTargetHeight ();

    void SetTargetHeight (double target_H);

    bool IsTargetHeightReached();

    double CalculateControl(double H, double Vh, double dt);


private:
    PIController pi_controller_H_;
    PController p_controller_Vh_;
    double Vh_satur_;
    double target_H_;
    double H_;
    double U_satur_;
};
