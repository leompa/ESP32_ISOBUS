//================================================================================================
/// @file vt_application.cpp
///
/// @brief This is the implementation of the VT portion of the seeder example
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "vt_application.hpp"

#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/utility/iop_file_interface.hpp"
#include "isobus/utility/system_timing.hpp"
#include "object_pool.hpp"

#include <cassert>
#include <iostream>
//#include "pca9685_handler.hpp"

SeederVtApplication::SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::InternalControlFunction> source) :
  VTClientInterface(std::make_shared<isobus::VirtualTerminalClient>(VTPartner, source)),
  VTClientUpdateHelper(VTClientInterface)
{
	alarms[AlarmType::NoMachineSpeed] = Alarm(10000); // 10 seconds
	alarms[AlarmType::NoPCA] = Alarm(10000); // 10 seconds
	alarms[AlarmType::NoTaskController] = Alarm(30000); // 30 seconds, TC can take a while to connect
}

extern "C" const uint8_t object_pool_start[] asm("_binary_object_pool_iop_start");
extern "C" const uint8_t object_pool_end[] asm("_binary_object_pool_iop_end");

bool SeederVtApplication::initialize()
{
	/*
	objectPool = isobus::IOPFileInterface::read_iop_file("BasePool.iop");

	if (objectPool.empty())
	{
		std::cout << "Failed to load object pool from BasePool.iop" << std::endl;
		return false;
	}
	std::cout << "Loaded object pool from BasePool.iop" << std::endl;

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(objectPool);
*/
//	virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	
	const std::uint8_t *testPool = object_pool_start;
	VTClientInterface->set_object_pool(0, testPool, (object_pool_end - object_pool_start) -1,"FerinekVoltIgaza");

//	VTClientInterface->set_object_pool(0, objectPool.data(), static_cast<std::uint32_t>(objectPool.size()), objectPoolHash);
	VTClientInterface->get_vt_soft_key_event_dispatcher().add_listener([this](const isobus::VirtualTerminalClient::VTKeyEvent &event) { this->handle_vt_key_events(event); });
	VTClientInterface->get_vt_button_event_dispatcher().add_listener([this](const isobus::VirtualTerminalClient::VTKeyEvent &event) { this->handle_vt_key_events(event); });
	VTClientInterface->get_vt_change_numeric_value_event_dispatcher().add_listener([this](const isobus::VirtualTerminalClient::VTChangeNumericValueEvent &event) { this->handle_numeric_value_events(event); });
	VTClientInterface->initialize(true);

	// Track the numeric values we want to update
	VTClientUpdateHelper.add_tracked_numeric_value(enableAlarms_VarNum, true);
	VTClientUpdateHelper.add_tracked_numeric_value(autoManual_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(statisticsSelection_VarNum, 1);
	VTClientUpdateHelper.add_tracked_numeric_value(selectedStatisticsContainer_ObjPtr, canStatistics_Container);
	VTClientUpdateHelper.add_tracked_numeric_value(canAddress_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(utAddress_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(busload_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(speedUnits_ObjPtr, unitKph_OutStr);
	VTClientUpdateHelper.add_tracked_numeric_value(tcAddress_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(tcNumberBoomsSupported_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(tcControlChannels_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(tcSupportedSections_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(tcVersion_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(section1EnableState_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(section2EnableState_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(section3EnableState_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(section4EnableState_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(section5EnableState_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(section6EnableState_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(currentSpeedMeter_VarNum, 16);
	VTClientUpdateHelper.add_tracked_numeric_value(currentSpeedReadout_VarNum, 16);
	VTClientUpdateHelper.add_tracked_numeric_value(utVersion_VarNum);
	VTClientUpdateHelper.add_tracked_numeric_value(currentAlarms1_ObjPtr);
	VTClientUpdateHelper.add_tracked_numeric_value(currentAlarms2_ObjPtr);

	// Track the attribute values we want to update
	VTClientUpdateHelper.add_tracked_attribute(section1Status_OutRect, 5, (std::uint32_t)solidGreen_FillAttr);
	VTClientUpdateHelper.add_tracked_attribute(section2Status_OutRect, 5, (std::uint32_t)solidYellow_FillAttr);
	VTClientUpdateHelper.add_tracked_attribute(section3Status_OutRect, 5, (std::uint32_t)solidRed_FillAttr);
	VTClientUpdateHelper.add_tracked_attribute(section4Status_OutRect, 5, (std::uint32_t)solidRed_FillAttr);
	VTClientUpdateHelper.add_tracked_attribute(section5Status_OutRect, 5, (std::uint32_t)solidYellow_FillAttr);
	VTClientUpdateHelper.add_tracked_attribute(section6Status_OutRect, 5, (std::uint32_t)solidGreen_FillAttr);

	VTClientUpdateHelper.initialize();

	// Update the objects to their initial state, we should try to minimize this
	VTClientUpdateHelper.set_numeric_value(autoManual_ObjPtr, manualMode_Container);
	return true;
}

void SeederVtApplication::handle_vt_key_events(const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
	if (event.keyNumber == 0)
	{
		// We have the alarm ACK code, so let's check if an alarm is active
		for (auto &alarm : alarms)
		{
			if (alarm.second.is_active())
			{
				alarm.second.acknowledge();
				update_alarms();
				break;
			}
		}
	}

	if (isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased == event.keyEvent)
	{
		switch (event.objectID)
		{
			case home_Key:
			{
				VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
			}
			break;

			case settings_Key:
			{
				VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, settingsRunscreen_DataMask);
			}
			break;

			case statistics_Key:
			{
				VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, statisticsRunscreen_DataMask);
			}
			break;

			case alarms_Key:
			{
				VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, alarmsRunscreen_DataMask);
			}
			break;

			case acknowledgeAlarm_SoftKey:
			{
				// Acknowledge the first active alarm
				for (auto &alarm : alarms)
				{
					if (alarm.second.is_active())
					{
						alarm.second.acknowledge();
						update_alarms();
						break;
					}
				}
				break;
			}
			break;

			case autoManualToggle_Button:
			{
				VTClientUpdateHelper.set_numeric_value(autoManual_ObjPtr, manualMode_Container);
				for (std::uint8_t i = 0; i < NUMBER_ONSCREEN_SECTIONS; ++i)
				{
					update_section_objects(i);
				}
			}
			break;

			case section1Toggle_Button:
			{
				toggle_section(0);
			}
			break;

			case section2Toggle_Button:
			{
				toggle_section(1);
			}
			break;

			case section3Toggle_Button:
			{
				toggle_section(2);
			}
			break;

			case section4Toggle_Button:
			{
				toggle_section(3);
			}
			break;

			case section5Toggle_Button:
			{
				toggle_section(4);
			}
			break;

			case section6Toggle_Button:
			{
				toggle_section(5);
			}
			break;

			default:
				break;
		}
	}
}

void SeederVtApplication::handle_numeric_value_events(const isobus::VirtualTerminalClient::VTChangeNumericValueEvent &event)
{
	switch (event.objectID)
	{
		case statisticsSelection_VarNum:
		{
			// Update the frame to show the newly selected statistic
			std::uint16_t targetContainer;
			switch (event.value)
			{
				case 1:
				{
					targetContainer = canStatistics_Container;
				}
				break;

				case 2:
				{
					targetContainer = utStatistics_Container;
				}
				break;

				case 3:
				{
					targetContainer = tcStatistics_Container;
				}
				break;

				case 4:
				{
					targetContainer = credits_Container;
				}
				break;

				default:
				{
					targetContainer = UNDEFINED;
				}
				break;
			}
			VTClientUpdateHelper.set_numeric_value(selectedStatisticsContainer_ObjPtr, targetContainer);
		}
		break;

		default:
			break;
	}
}


void SeederVtApplication::update()
{
	// Update some polled data or other things that don't need as frequent updates
	if (isobus::SystemTiming::time_expired_ms(slowUpdateTimestamp_ms, 1000))
	{
		auto VTClientControlFunction = VTClientInterface->get_internal_control_function();
		auto VTControlFunction = VTClientInterface->get_partner_control_function();
		if (nullptr != VTClientControlFunction)
		{
			// These are used for displaying to the user. Address is not really needed to be known.
			VTClientUpdateHelper.set_numeric_value(canAddress_VarNum, VTClientControlFunction->get_address());
			VTClientUpdateHelper.set_numeric_value(utAddress_VarNum, VTControlFunction->get_address());
			if (!languageDataRequested)
			{
				languageDataRequested = VTClientInterface->languageCommandInterface.send_request_language_command();
			}
		}
		if (get_is_object_shown(busload_VarNum))
		{
			auto busload = isobus::CANNetworkManager::CANNetwork.get_estimated_busload(0);
			VTClientUpdateHelper.set_numeric_value(busload_VarNum, static_cast<std::uint32_t>(busload * 100.0f));
		}
		update_ut_version_objects(VTClientInterface->get_connected_vt_version());

		update_alarms();
		slowUpdateTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();

		update_pca();
	}
	for (std::uint8_t i = 0; i < NUMBER_ONSCREEN_SECTIONS; ++i)
	{
		update_section_objects(i);
	}
	VTClientUpdateHelper.set_numeric_value(autoManual_ObjPtr, manualMode_Container);
}

void SeederVtApplication::update_pca()
{
	
}

void SeederVtApplication::toggle_section(std::uint8_t sectionIndex)
{
	update_section_objects(sectionIndex);
}

void SeederVtApplication::update_section_objects(std::uint8_t sectionIndex)
{
	std::uint16_t newObject = offButtonSliderSmall_OutPict;
	if (true)//aca manejo que color quiero dar 
	{
		newObject = onButtonSliderSmall_OutPict;
	}

	std::uint32_t fillAttribute = solidRed_FillAttr;
	if (false)
	{
		fillAttribute = solidGreen_FillAttr;
	}
	else if (false)
	{
		fillAttribute = solidYellow_FillAttr;
	}
	else
	{
		fillAttribute = solidRed_FillAttr;
	}

	std::uint16_t switchPointerId = UNDEFINED;
	std::uint16_t statusRectangleId = UNDEFINED;
	switch (sectionIndex)
	{
		case 0:
		{
			switchPointerId = section1EnableState_ObjPtr;
			statusRectangleId = section1Status_OutRect;
		}
		break;

		case 1:
		{
			switchPointerId = section2EnableState_ObjPtr;
			statusRectangleId = section2Status_OutRect;
		}
		break;

		case 2:
		{
			switchPointerId = section3EnableState_ObjPtr;
			statusRectangleId = section3Status_OutRect;
		}
		break;

		case 3:
		{
			switchPointerId = section4EnableState_ObjPtr;
			statusRectangleId = section4Status_OutRect;
		}
		break;

		case 4:
		{
			switchPointerId = section5EnableState_ObjPtr;
			statusRectangleId = section5Status_OutRect;
		}
		break;

		case 5:
		{
			switchPointerId = section6EnableState_ObjPtr;
			statusRectangleId = section6Status_OutRect;
		}
		break;

		default:
			break;
	}
	VTClientUpdateHelper.set_numeric_value(switchPointerId, newObject);
	VTClientUpdateHelper.set_attribute(statusRectangleId, 5, fillAttribute); // 5 Is the attribute ID of the fill attribute
}


bool SeederVtApplication::get_is_object_shown(std::uint16_t objectID) const
{
	//! TODO: add this functionality to the VTClientStateTracker

	if (!VTClientUpdateHelper.is_working_set_active())
	{
		return false;
	}
	bool retVal = false;

	switch (objectID)
	{
		case section1Status_OutRect:
		case section2Status_OutRect:
		case section3Status_OutRect:
		case section4Status_OutRect:
		case section5Status_OutRect:
		case section6Status_OutRect:
		case autoManual_Container:
		case autoManual_ObjPtr:
		case mainRunscreen_SoftKeyMask:
		case Title_OutStr:
		case planterRunscreenStatus_Container:
		case planter_OutPict:
		case sectionButtons_Container:
		case section1Switch_Container:
		case section2Switch_Container:
		case section3Switch_Container:
		case section4Switch_Container:
		case section5Switch_Container:
		case section6Switch_Container:
		case speed_OutNum:
		case speedReadout_Container:
		case speedUnits_ObjPtr:
		case currentSpeedReadout_VarNum:
		case currentSpeedMeter_VarNum:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() == mainRunscreen_DataMask);
		}
		break;

		case statisticsHeader_OutStr:
		case statisticsDropdown_Container:
		case statistics_InList:
		case selectedStatisticsContainer_ObjPtr:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() == statisticsRunscreen_DataMask);
		}
		break;

		case returnHome_SKeyMask:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() != mainRunscreen_DataMask);
		}
		break;

		case busload_VarNum:
		case canAddress_VarNum:
		{
			retVal = ((VTClientUpdateHelper.get_active_mask() == statisticsRunscreen_DataMask) &&
			          (VTClientUpdateHelper.get_numeric_value(selectedStatisticsContainer_ObjPtr) == canStatistics_Container));
		}
		break;

		case utAddress_VarNum:
		case utVersion_VarNum:
		{
			retVal = ((VTClientUpdateHelper.get_active_mask() == statisticsRunscreen_DataMask) &&
			          (VTClientUpdateHelper.get_numeric_value(selectedStatisticsContainer_ObjPtr) == utStatistics_Container));
		}
		break;

		case tcVersion_VarNum:
		case tcAddress_VarNum:
		case tcNumberBoomsSupported_VarNum:
		case tcSupportedSections_VarNum:
		case tcControlChannels_VarNum:
		{
			retVal = ((VTClientUpdateHelper.get_active_mask() == statisticsRunscreen_DataMask) &&
			          (VTClientUpdateHelper.get_numeric_value(selectedStatisticsContainer_ObjPtr) == tcStatistics_Container));
		}
		break;

		case machineSpeedNotDetectedSummary_OutStr:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() == noSpeed_AlarmMask);
		}
		break;

		case TCNotConnectedSummary_OutStr:
		case noTCTitle_OutStr:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() == noTaskController_AlarmMask);
		}
		break;

		case warning_OutPict:
		case alarm_SKeyMask:
		{
			retVal = ((VTClientUpdateHelper.get_active_mask() == noSpeed_AlarmMask) ||
			          (VTClientUpdateHelper.get_active_mask() == noTaskController_AlarmMask));
		}
		break;

		case currentAlarms1_ObjPtr:
		case currentAlarms2_ObjPtr:
		case currentAlarmsHeader_OutStr:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() == alarmsRunscreen_DataMask);
		}
		break;

		case enableAlarms_VarNum:
		case enableAlarms_Container:
		case enableAlarms_InBool:
		case enableAlarms_OutStr:
		{
			retVal = (VTClientUpdateHelper.get_active_mask() == settingsRunscreen_DataMask);
		}
		break;

		default:
		{
			retVal = true;
		}
		break;
	}
	return retVal;
}

void SeederVtApplication::revert_to_previous_data_mask()
{
	for (std::uint16_t maskId : VTClientUpdateHelper.get_mask_history())
	{
		// Check if mask is a data mask and if it is not the current mask
		if ((maskId != noSpeed_AlarmMask) && (maskId != noTaskController_AlarmMask) && (maskId != VTClientUpdateHelper.get_active_mask()))
		{
			VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, maskId);
			return;
		}
	}
	// No previous data mask found, revert to main runscreen
	VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
}

void SeederVtApplication::update_ut_version_objects(isobus::VirtualTerminalClient::VTVersion version)
{
	std::uint8_t integerVersion = 0xFF;

	switch (version)
	{
		case isobus::VirtualTerminalClient::VTVersion::Version2OrOlder:
		{
			integerVersion = 2;
		}
		break;

		case isobus::VirtualTerminalClient::VTVersion::Version3:
		{
			integerVersion = 3;
		}
		break;

		case isobus::VirtualTerminalClient::VTVersion::Version4:
		{
			integerVersion = 4;
		}
		break;

		case isobus::VirtualTerminalClient::VTVersion::Version5:
		{
			integerVersion = 5;
		}
		break;

		case isobus::VirtualTerminalClient::VTVersion::Version6:
		{
			integerVersion = 6;
		}
		break;

		default:
			break;
	}
	VTClientUpdateHelper.set_numeric_value(utVersion_VarNum, integerVersion);
}

void SeederVtApplication::update_alarms()
{
	if (VTClientInterface->get_is_connected() && VTClientUpdateHelper.get_numeric_value(enableAlarms_VarNum))
	{

		// Show the first alarm that is active (i.e. highest priority)
		std::size_t activeAlarmsCount = 0;
		for (auto const &alarm : alarms)
		{
			if (alarm.second.is_active())
			{
				activeAlarmsCount++;
				switch (alarm.first)
				{
					case AlarmType::NoMachineSpeed:
					{
						if (1 == activeAlarmsCount)
						{
							VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, noSpeed_AlarmMask);
						}
						VTClientUpdateHelper.set_numeric_value(currentAlarms1_ObjPtr, NoMachineSpeed_OutStr);
					}
					break;

					case AlarmType::NoPCA:
					case AlarmType::NoTaskController:
					{
						if (1 == activeAlarmsCount)
						{
							VTClientUpdateHelper.set_active_data_or_alarm_mask(example_WorkingSet, noTaskController_AlarmMask);
						}
						VTClientUpdateHelper.set_numeric_value(1 == activeAlarmsCount ? currentAlarms1_ObjPtr : currentAlarms2_ObjPtr, NoTaskController_OutStr);
					}

					break;

					default:
						break;
				}
			}
		}

		if ((0 == activeAlarmsCount) && ((VTClientUpdateHelper.get_active_mask() == noSpeed_AlarmMask) || (VTClientUpdateHelper.get_active_mask() == noTaskController_AlarmMask)))
		{
			// No alarms active, but we're showing the alarm screen. Clear it.
			revert_to_previous_data_mask();
		}

		for (std::size_t i = activeAlarmsCount; i < static_cast<std::size_t>(AlarmType::Count); ++i)
		{
			// Clear the remaining alarm slots
			VTClientUpdateHelper.set_numeric_value(i == 0 ? currentAlarms1_ObjPtr : currentAlarms2_ObjPtr, UNDEFINED);
		}
	}
}

SeederVtApplication::Alarm::Alarm(std::uint32_t activationDelay_ms) :
  activationDelay_ms(activationDelay_ms)
{
}

bool SeederVtApplication::Alarm::is_active() const
{
	return (!acknowledged) && (timestampTriggered_ms != 0) &&
	  isobus::SystemTiming::time_expired_ms(timestampTriggered_ms, activationDelay_ms);
}

void SeederVtApplication::Alarm::trigger()
{
	if (timestampTriggered_ms == 0)
	{
		timestampTriggered_ms = isobus::SystemTiming::get_timestamp_ms();
	}
}

void SeederVtApplication::Alarm::acknowledge()
{
	acknowledged = true;
}

void SeederVtApplication::Alarm::reset()
{
	timestampTriggered_ms = 0;
	acknowledged = false;
}
