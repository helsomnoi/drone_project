#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>
#include <chrono>
#include <thread>

#include <mavsdk/mavsdk.h>

#include "drone_control.h"


using namespace mavsdk;

int main(int argc, char *argv[])
{
    //------------------------------------------------------------------------
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <connection_url> <target_height>" << std::endl;
        std::cerr << "Example: " << argv[0] << " udp://:14540 2.5" << std::endl;
        return 1;
    }

    std::string connection_url = argv[1];
    float target_height = std::stof(argv[2]);

    mavsdk::Mavsdk uav{mavsdk::Mavsdk::Configuration{mavsdk::Mavsdk::ComponentType::GroundStation}};

    ConnectionResult connection_result = uav.add_any_connection(connection_url);
    if (connection_result != ConnectionResult::Success)
    {
        std::cerr << "Error: Connection with " << connection_url
                  << " failed: " << connection_result << std::endl;
        return 1;
    }
    std::cout << "The flight controller is successfully connected!\n";

    std::shared_ptr<System> system;
    std::cout << "Waiting connection..." << std::endl;

    auto start_time = std::chrono::steady_clock::now();
    const double timeout_s = 10.0;

    while (std::chrono::steady_clock::now() - start_time < std::chrono::duration<double>(timeout_s))
    {
        auto systems = uav.systems();
        if (!systems.empty())
        {
            system = systems[0];
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!system)
    {
        std::cerr << "Erroe: timeout" << std::endl;
        return 1;
    }

    if (!system->has_autopilot())
    {
        std::cerr << "Error: system has no autopilot" << std::endl;
        return 1;
    }

    //std::cout << "Система обнаружена. UUID: " << system->uuid() << std::endl;

    try
    {
        DroneControl drone_control(system);

        std::cout << "\n=== Starting mission ===\n";
        std::cout << "Target altitude: " << target_height << " m\n\n";

        drone_control.ArmStage();
        std::cout << "Armed\n";

        drone_control.TakeoffToPosition(target_height);
        std::cout << "Takeoff complete\n";

        drone_control.HoldHight();
        std::cout << "Hold complete\n";

        drone_control.Land();
        std::cout << "Landed\n";

        drone_control.DisarmStage();
        std::cout << "Disarmed\n";

        std::cout << "\n=== Mission completed successfully ===\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n Mission failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}