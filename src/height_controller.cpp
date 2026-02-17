#include "height_controller.h"

#include <cmath>
#include <algorithm>

HeightControler::HeightControler(const HeightControlCoef & ctrl_coef, double target_H)
    : pi_controller_H_(ctrl_coef.pi_H_kp, ctrl_coef.pi_H_ki, ctrl_coef.pi_H_integral_max),
    p_controller_Vh_(ctrl_coef.p_Vh_kp),
    Vh_satur_(ctrl_coef.max_Vh),
    target_H_(target_H),
    H_(0.0),
    U_satur_(ctrl_coef.U_max){
}


double HeightControler::GetTargetHeight(){
    return target_H_;
}

void HeightControler::SetTargetHeight(double target_H){
    target_H_ = target_H;
    pi_controller_H_.Reset();
}

bool HeightControler::IsTargetHeightReached(){
    return std::abs(target_H_ - H_) <= THRESHOLD_HIGHT_ERR;
}

double HeightControler::CalculateControl(double H, double Vh, double dt){
    H_ = H;
    double H_err = target_H_ - H;
    double target_Vh = pi_controller_H_.Update(H_err, dt);
    target_Vh = Saturation(target_Vh, Vh_satur_);

    double Vh_err = target_Vh - Vh;
    double U = p_controller_Vh_.Update(Vh_err, dt);
    U = Saturation(U, U_satur_);
    
    return U;
}