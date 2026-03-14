//================================================================================================
/// @file seeder.cpp
///
/// @brief This is the implementation of an example seeder application
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "seeder.hpp"

#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/twai_plugin.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"

#include "console_logger.cpp"

#include <iostream>
#include <driver/twai.h>

bool Seeder::initialize()
{
	bool retVal = true;
	

	// Automatically load the desired CAN driver based on the available drivers
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
	// Automatically load the desired CAN driver based on the available drivers
	twai_general_config_t twaiConfig = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_17, GPIO_NUM_16, TWAI_MODE_NORMAL);
	twai_timing_config_t twaiTiming = TWAI_TIMING_CONFIG_250KBITS();
	twai_filter_config_t twaiFilter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	canDriver = std::make_shared<isobus::TWAIPlugin>(&twaiConfig, &twaiTiming, &twaiFilter);

	
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return false;
	}
		//Configuracion para JD
	isobus::CANNetworkManager::CANNetwork.get_configuration().set_number_of_packets_per_dpo_message(255); 
	isobus::CANNetworkManager::CANNetwork.get_configuration().set_number_of_packets_per_cts_message(255);
	vTaskDelay(3000);
	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug); // Change this to Debug to see more information
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return false;
	}

	//std::this_thread::sleep_for(std::chrono::milliseconds(250));
	vTaskDelay(1500);
	isobus::NAME TestDeviceNAME(0);

	//! This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto InternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto PartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	VTApplication = std::make_unique<SeederVtApplication>(PartnerVT, InternalECU);
	//virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	VTApplication->initialize();

	return retVal;
}

void Seeder::terminate()
{
	if (nullptr != VTApplication)
	{
		VTApplication->VTClientInterface->terminate();
	}
	isobus::CANHardwareInterface::stop();
}

void Seeder::update()
{
	if (nullptr != VTApplication)
	{
		VTApplication->update();
	}
}
