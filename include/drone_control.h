#pragma once

#include "height_controller.h"

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/offboard/offboard.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

constexpr double DURATION_TIME = 0.1;
constexpr double HOLD_TIME = 10.0;

class DroneControl
{
public:
    DroneControl(std::shared_ptr<mavsdk::System> system, double dt = DURATION_TIME);

    bool IsArmed() const;
    bool IsInAir() const;
    bool IsOffboardActive() const;
    double GetCurrentHeight() const;

    void ArmStage();
    void DisarmStage();
    void TakeoffToPosition(double target_H);
    void HoldHight();
    void Land();

private:
    void UpdateTact();
    void SetSystemControl(double U);
    bool WaitForOffboardStart();
    void UpdateTelemetry();

    std::shared_ptr<mavsdk::System> system_;
    std::shared_ptr<mavsdk::Telemetry> telemetry_;
    std::shared_ptr<mavsdk::Action> action_;
    std::shared_ptr<mavsdk::Offboard> offboard_;
    HeightControler controller_;
    double dt_;
    bool offboard_active_{false}; 

    mutable double cached_height_{0.0};
    mutable double cached_vertical_speed_{0.0};
    mutable bool cached_armed_{false};
    mutable bool cached_in_air_{false};
};
