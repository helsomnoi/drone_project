#include "drone_control.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace mavsdk;

DroneControl::DroneControl(std::shared_ptr<System> system, double dt)
    : system_(system),
      telemetry_(std::make_shared<Telemetry>(system_)),
      action_(std::make_shared<Action>(system)),
      offboard_(std::make_shared<Offboard>(system)),
      controller_(HeightControlCoef()),
      dt_(dt)
{
    if (!system_)
    {
        throw std::runtime_error("DroneControl: system pointer is null");
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[DroneControl] dt= " << dt_ << " с\n";

    telemetry_->subscribe_position([this](Telemetry::Position pos)
                                   { cached_height_ = pos.relative_altitude_m; });

    telemetry_->subscribe_velocity_ned([this](Telemetry::VelocityNed vel)
                                       { cached_vertical_speed_ = -vel.down_m_s; });

    telemetry_->subscribe_armed([this](bool armed)
                                { cached_armed_ = armed; });

    telemetry_->subscribe_in_air([this](bool in_air)
                                 { cached_in_air_ = in_air; });
}

bool DroneControl::IsArmed() const
{
    return cached_armed_;
}

bool DroneControl::IsInAir() const
{
    return cached_in_air_;
}

bool DroneControl::IsOffboardActive() const
{
    return offboard_active_;
}

double DroneControl::GetCurrentHeight() const
{
    return cached_height_;
}

void DroneControl::ArmStage()
{
    if (IsArmed())
    {
        std::cout << "[Arm] Already Armed\n";
        return;
    }

    std::cout << "Waiting for drone to be healthy..." << std::endl;
    while (!telemetry_->health_all_ok())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    auto arm_result = action_->arm();
    if (arm_result != Action::Result::Success)
    {
        throw std::runtime_error("Error ARM: " +
                                 std::to_string(static_cast<int>(arm_result)));
    }

    std::cout << "[Arm] System armed successfully!\n";

    // init
    Offboard::Attitude initial_setpoint;
    initial_setpoint.roll_deg = 0.0f;
    initial_setpoint.pitch_deg = 0.0f;
    initial_setpoint.yaw_deg = telemetry_->attitude_euler().yaw_deg;
    initial_setpoint.thrust_value = 0.1f;
    offboard_->set_attitude(initial_setpoint);

    auto offboard_result = offboard_->start();
    if (offboard_result != Offboard::Result::Success)
    {
        throw std::runtime_error("Error offboarding: " +
                                 std::to_string(static_cast<int>(offboard_result)));
    }

    offboard_active_ = true;
    std::cout << "[Arm] System connected ofboard successfuly\n";
}

void DroneControl::DisarmStage()
{
    if (!IsArmed())
    {
        std::cout << "[Disarm] Already Disarmed\n";
        return;
    }

    // stop offboard
    if (offboard_active_)
    {
        auto stop_result = offboard_->stop();
        if (stop_result != Offboard::Result::Success)
        {
            std::cerr << "[Disarm] Error stopping offboard: "
                      << static_cast<int>(stop_result) << std::endl;
        }
        offboard_active_ = false;
        std::cout << "[Disarm] Offboard stoped successuly\n";
    }

    std::cout << "Waiting for land to be detected..." << std::endl;
    while (telemetry_->in_air())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto disarm_result = action_->disarm();
    if (disarm_result != Action::Result::Success)
    {
        throw std::runtime_error("Error disarm: " +
                                 std::to_string(static_cast<int>(disarm_result)));
    }

    std::cout << "[Disarm] System disarmed successfuly\n";
}

void DroneControl::TakeoffToPosition(double target_H)
{
    if (!IsArmed())
    {
        std::cout << "[Takeoff] System not armed\n";
        ArmStage();
    }

    if (!offboard_active_)
    {
        throw std::runtime_error("[Takeoff] Offboard not active");
    }

    std::cout << "[Takeoff] Take off to " << target_H << " m\n";

    controller_.SetTargetHeight(target_H);

    int tact_count = 0;
    while (!controller_.IsTargetHeightReached())
    {
        UpdateTact();

        if (++tact_count % 25 == 0)
        {
            std::cout << "[Takeoff] Height: " << cached_height_
                      << " м, error: " << (target_H - cached_height_)
                      << " m\n";
        }
    }

    std::cout << "[Takeoff] Target height: " << target_H << " m is reached!\n";
}
void DroneControl::HoldHight()
{
    if (!offboard_active_)
    {
        throw std::runtime_error("[Hold] Offboard not active");
    }

    double hold_height = controller_.IsTargetHeightReached()
                             ? controller_.GetTargetHeight()
                             : cached_height_;

    controller_.SetTargetHeight(hold_height);

    std::cout << "[Hold] Hold height " << hold_height
              << " m during " << HOLD_TIME << " sec\n";

    int total_ticks = static_cast<int>(HOLD_TIME / dt_);
    int tenth_ticks = total_ticks / 10;

    for (int i = 0; i < total_ticks; ++i)
    {
        UpdateTact();

        // Вывод прогресса
        if (tenth_ticks > 0 && i % tenth_ticks == 0 && i > 0)
        {
            double progress = (static_cast<double>(i) / total_ticks) * 100.0;
            std::cout << "[Hold] " << static_cast<int>(progress)
                      << "%, высота: " << cached_height_ << " м\n";
        }
    }

    std::cout << "[Hold] Hold height completed\n";
}

void DroneControl::Land()
{
    if (!IsInAir())
    {
        std::cout << "[Land] Already landed\n";
        return;
    }

    if (!offboard_active_)
    {
        throw std::runtime_error("[Land] Offboard not active");
    }

    std::cout << "[Land] Landing...\n";

    controller_.SetTargetHeight(0.0);

    int tact_count = 0;
    while (IsInAir() && cached_height_ > 0.1)
    {
        UpdateTact();

        if (++tact_count % 25 == 0)
        {
            std::cout << "[Land] Height: " << cached_height_ << " m\n";
        }
    }

    std::cout << "[Land] System id landed\n";

    DisarmStage();
}

void DroneControl::UpdateTact()
{
    if (!offboard_active_)
    {
        std::cerr << "[Warning] Offboard not active\n";
        return;
    }

    double H = cached_height_;
    double VH = cached_vertical_speed_;

    double U = controller_.CalculateControl(H, VH, dt_);
    SetSystemControl(U);
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt_ * 1000)));
}

void DroneControl::SetSystemControl(double U)
{
    float thrust = DEFAULT_U + U / 2.0;

    thrust = std::max(0.2f, std::min(0.8f, thrust));

    float current_yaw = telemetry_->attitude_euler().yaw_deg;

    Offboard::Attitude setpoint;
    setpoint.roll_deg = 0.0f;
    setpoint.pitch_deg = 0.0f;
    setpoint.yaw_deg = current_yaw;
    setpoint.thrust_value = thrust;

    offboard_->set_attitude(setpoint);
}
