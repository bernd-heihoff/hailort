/**
 * Copyright (c) 2020-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the MIT license (https://opensource.org/licenses/MIT)
 **/
/**
 * @file device.hpp
 * @brief Hailo device representation
 **/

#ifndef _HAILO_DEVICE_HPP_
#define _HAILO_DEVICE_HPP_

#include "hailo/hailort.h"
#include "hailo/expected.hpp"
#include "hailo/hef.hpp"
#include "hailo/network_group.hpp"

#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace hailort
{

typedef enum {
    BOOTLOADER_VERSION_HAILO8_B0_UNSIGNED = 0,
    BOOTLOADER_VERSION_HAILO8_B0_SIGNED
} hailo_bootloader_version_t;

/** @defgroup group_type_definitions HailoRT CPP API definitions
 *  @{
 */

class Device;
using NotificationCallback = std::function<void(Device &device, const hailo_notification_t &notification, void *opaque)>;

/** @} */ // end of group_type_definitions

/*! Represents the Hailo device (chip). */
class HAILORTAPI Device
{
public:

    /** The device type */
    enum class Type {
        PCIE = 0,
        ETH,
        CORE
    };

    /**
     * Returns information on all available pcie devices in the system.
     * 
     * @return Upon success, returns Expected of a vector of ::hailo_pcie_device_info_t containing the information.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::vector<hailo_pcie_device_info_t>> scan_pcie();

    /**
     * Returns information on all available ethernet devices in the system.
     * 
     * @param[in] interface_name            The name of the network interface to scan.
     * @param[in] timeout                   The time in milliseconds to scan devices.
     * @return Upon success, returns Expected of a vector of ::hailo_eth_device_info_t containing the information.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::vector<hailo_eth_device_info_t>> scan_eth(const std::string &interface_name,
        std::chrono::milliseconds timeout);

    /**
     * Scans ethernet device by host address.
     * 
     * @param[in] host_address              The IP address of the network interface to scan.
     * @param[in] timeout                   The time in milliseconds to scan devices.
     * @return Upon success, returns Expected of a vector of ::hailo_eth_device_info_t containing the information.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::vector<hailo_eth_device_info_t>> scan_eth_by_host_address(const std::string &host_address,
        std::chrono::milliseconds timeout);

    /**
     * Creates pcie device if there is only one pcie device connected
     * 
     * @return Upon success, returns Expected of a unique_ptr to Device object.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::unique_ptr<Device>> create_pcie();

    /**
     * Creates a PCIe device by the given info.
     * 
     * @param[in] device_info    Information about the device to open.
     * @return Upon success, returns Expected of a unique_ptr to Device object.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::unique_ptr<Device>> create_pcie(const hailo_pcie_device_info_t &device_info);

    /**
     * Creates an ethernet device by the given info.
     * 
     * @param[in] device_info   Information about the device to open.
     * @return Upon success, returns Expected of a unique_ptr to Device object.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::unique_ptr<Device>> create_eth(const hailo_eth_device_info_t &device_info);

    /**
     * Creates an ethernet device by IP address.
     * 
     * @param[in] ip_addr      The device IP address.
     * @return Upon success, returns Expected of a unique_ptr to Device object.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::unique_ptr<Device>> create_eth(const std::string &ip_addr);

    /**
     * Parse PCIe device BDF string into hailo device info structure.
     * 
     * @param[in] device_info_str   BDF device info, format [\<domain\>].\<bus\>.\<device\>.\<func\>, same format as in lspci.
     * @return Upon success, returns Expected of ::hailo_pcie_device_info_t containing the information.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<hailo_pcie_device_info_t> parse_pcie_device_info(const std::string &device_info_str);

    /**
     * Returns a string of pcie device info.
     * 
     * @param[in] device_info       A ::hailo_pcie_device_info_t containing the pcie device information.
     * @return Upon success, returns Expected of a string containing the information.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    static Expected<std::string> pcie_device_info_to_string(const hailo_pcie_device_info_t &device_info);

    // For internal use only
    static bool is_core_driver_loaded();
    static Expected<std::unique_ptr<Device>> create_core_device();

    /**
     * Configure the device from an hef.
     *
     * @param[in] hef                         A reference to an Hef object to configure the device by.
     * @param[in] configure_params            A map of configured network group name and parameters.
     * @return Upon success, returns Expected of a vector of configured network groups.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    virtual Expected<ConfiguredNetworkGroupVector> configure(Hef &hef,
        const NetworkGroupsParamsMap &configure_params={}) = 0;

    /**
     * Read data from the debug log buffer.
     *
     * @param[in] buffer            A buffer that would receive the debug log data.
     * @param[in] cpu_id            The cpu source of the debug log.
     * @return Upon success, returns Expected of the number of bytes that were read.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    virtual Expected<size_t> read_log(MemoryView &buffer, hailo_cpu_id_t cpu_id) = 0;

    /**
     * Sends identify control to a Hailo device.
     *
     * @return Upon success, returns Expected of ::hailo_device_identity_t.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    Expected<hailo_device_identity_t> identify();

    /**
     * Receive information about the core cpu.
     * 
     * @return Upon success, returns Expected of ::hailo_core_information_t.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    Expected<hailo_core_information_t> core_identify();

    /**
     * Get extended device information about the Hailo device.
     *
     * @return Upon success, returns Expected of ::hailo_extended_device_information_t containing the extended information about the device.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    Expected<hailo_extended_device_information_t> get_extended_device_information();

    /**
     * Configure fw logger level and interface of sending.
     *
     * @param[in] level             The minimum logger level.
     * @param[in] interface_mask    Output interfaces (mix of ::hailo_fw_logger_interface_t).
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     */
    hailo_status set_fw_logger(hailo_fw_logger_level_t level, uint32_t interface_mask);

    /**
     * Change throttling state of temperature protection component.
     *
     * @param[in] should_activate   Should be true to enable or false to disable.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     */
    hailo_status set_throttling_state(bool should_activate);

    /**
     * Writes data to device memory.
     * 
     * @param[in] address   The address data would be written to.
     * @param[in] data      A buffer that contains the data to be written to the memory.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    hailo_status write_memory(uint32_t address, const MemoryView &data);

    /**
     * Reads data from device memory.
     * 
     * @param[in] address   The address data would be read from.
     * @param[in] data      A buffer that receives the data read from memory.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */    
    hailo_status read_memory(uint32_t address, MemoryView &data);

    /**
     * Get current throttling state of temperature protection component.
     *
     * @return Upon success, returns Expected of @a bool, indicates weather the throttling state is active or not.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    Expected<bool> get_throttling_state();

    /**
     * Enable firmware watchdog.
     *
     * @param[in] cpu_id   A @a hailo_cpu_id_t indicating which CPU WD to enable.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     * @note Advanced API. Please use with care.
     */
    hailo_status wd_enable(hailo_cpu_id_t cpu_id);

    /**
     * Disable firmware watchdog.
     *
     * @param[in] cpu_id   A @a hailo_cpu_id_t indicating which CPU WD to disable.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     * @note Advanced API. Please use with care.
     */
    hailo_status wd_disable(hailo_cpu_id_t cpu_id);

    /**
     * Configure firmware watchdog.
     *
     * @param[in] cpu_id    A @a hailo_cpu_id_t indicating which CPU WD to configure.
     * @param[in] wd_cycles Number of cycles until watchdog is triggered.
     * @param[in] wd_mode   A @a hailo_watchdog_mode_t indicating which WD mode to configure.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     * @note Advanced API. Please use with care.
     */
    hailo_status wd_config(hailo_cpu_id_t cpu_id, uint32_t wd_cycles, hailo_watchdog_mode_t wd_mode);

    /**
     * Read the FW previous system state.
     *
     * @param[in] cpu_id    A @a hailo_cpu_id_t indicating which CPU to state to read.
     * @return Upon success, returns Expected of @a uint32_t indicating the previous system state.
     *         0 indicating external reset, 1 indicating WD HW reset,
     *         2 indicating WD SW reset, 3 indicating SW control reset.
     *         Otherwise, returns an ::hailo_status error.
     * @note Advanced API. Please use with care.
     */
    Expected<uint32_t> previous_system_state(hailo_cpu_id_t cpu_id);

    /**
     * Enable/Disable Pause frames.
     *
     * @param[in] rx_pause_frames_enable  Indicating weather to enable or disable pause frames.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     */
    hailo_status set_pause_frames(bool rx_pause_frames_enable);

    /**
     *  Read data from an I2C slave.
     *
     * @param[in] slave_config          The ::hailo_i2c_slave_config_t configuration of the slave.
     * @param[in] register_address      The address of the register from which the data will be read.
     * @param[in] data                  A buffer that would store the read data.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     */
    hailo_status i2c_read(const hailo_i2c_slave_config_t &slave_config, uint32_t register_address, MemoryView &data);

    /**
     *  Write data to an I2C slave.
     *
     * @param[in] slave_config          The ::hailo_i2c_slave_config_t configuration of the slave.
     * @param[in] register_address      The address of the register to which the data will be written.
     * @param[in] data                  A buffer that contains the data to be written to the slave.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns an ::hailo_status error.
     */
    hailo_status i2c_write(const hailo_i2c_slave_config_t &slave_config, uint32_t register_address, const MemoryView &data);

    /**
     * Perform a single power measurement.
     * 
     * @param[in]   dvm                Which DVM will be measured. Default (::HAILO_DVM_OPTIONS_AUTO) will be different according to the board: <br>
     *                                 - Default (::HAILO_DVM_OPTIONS_AUTO) for EVB is an approximation to the total power consumption of the chip in PCIe setups.
     *                                 It sums ::HAILO_DVM_OPTIONS_VDD_CORE, ::HAILO_DVM_OPTIONS_MIPI_AVDD and ::HAILO_DVM_OPTIONS_AVDD_H.
     *                                 Only ::HAILO_POWER_MEASUREMENT_TYPES__POWER can measured with this option.
     *                                 - Default (::HAILO_DVM_OPTIONS_AUTO) for platforms supporting current monitoring (such as M.2 and mPCIe): OVERCURRENT_PROTECTION.
     * @param[in]   measurement_type   The type of the measurement. Choosing ::HAILO_POWER_MEASUREMENT_TYPES__AUTO
     *                                 will select the default value according to the supported features.
     * @return Upon success, returns @a uint32_t mesuremenet. Measured units are determined due to ::hailo_power_measurement_types_t.
     *         Otherwise, returns a ::hailo_status error.
     */
    Expected<float32_t> power_measurement(hailo_dvm_options_t dvm, hailo_power_measurement_types_t measurement_type);

    /**
     * Start performing a long power measurement.
     * 
     * @param[in]   unused               Unused parameter.
     *                                   This time period is sleep time of the core.
     * @param[in]   averaging_factor     Number of samples per time period, sensor configuration value.
     * @param[in]   sampling_period      Related conversion time, sensor configuration value.
     *                                   The sensor samples the power every sampling_period {ms} and averages every
     *                                   averaging_factor samples. The sensor provides a new value every: 2 * sampling_period * averaging_factor {ms}.
     *                                   The firmware wakes up every interval_milliseconds {ms} and checks the sensor.
     *                                   If there is a new value to read from the sensor, the firmware reads it.
     *                                   Note that the average calculated by the firmware is “average of averages”,
     *                                   because it averages values that have already been averaged by the sensor.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     * @note This function is deprecated. One should use 'Device::start_power_measurement(hailo_averaging_factor_t averaging_factor, hailo_sampling_period_t sampling_period)'
     */
    hailo_status start_power_measurement(uint32_t unused, hailo_averaging_factor_t averaging_factor, hailo_sampling_period_t sampling_period);

    /**
     * Set parameters for long power measurement.
     * 
     * @param[in]   index              Index of the buffer on the firmware the data would be saved at.
     * @param[in]   dvm                Which DVM will be measured. Default (::HAILO_DVM_OPTIONS_AUTO) will be different according to the board: <br>
     *                                 - Default (::HAILO_DVM_OPTIONS_AUTO) for EVB is an approximation to the total power consumption of the chip in PCIe setups.
     *                                 It sums ::HAILO_DVM_OPTIONS_VDD_CORE, ::HAILO_DVM_OPTIONS_MIPI_AVDD and ::HAILO_DVM_OPTIONS_AVDD_H.
     *                                 Only ::HAILO_POWER_MEASUREMENT_TYPES__POWER can measured with this option.
     *                                 - Default (::HAILO_DVM_OPTIONS_AUTO) for platforms supporting current monitoring (such as M.2 and mPCIe): OVERCURRENT_PROTECTION.
     * @param[in]   measurement_type   The type of the measurement. Choosing ::HAILO_POWER_MEASUREMENT_TYPES__AUTO
     *                                 will select the default value according to the supported features.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     * @note This function is deprecated. One should use 'Device::set_power_measurement(hailo_measurement_buffer_index_t buffer_index, hailo_dvm_options_t dvm, hailo_power_measurement_types_t measurement_type)'
     */
    hailo_status set_power_measurement(uint32_t index, hailo_dvm_options_t dvm, hailo_power_measurement_types_t measurement_type);

    /**
     * Read measured power from a long power measurement
     * 
     * @param[in]   index                 Index of the buffer on the firmware the data would be saved at.
     * @param[in]   should_clear          Flag indicating if the results saved at the firmware will be deleted after reading.
     * @return Upon success, returns @a hailo_power_measurement_data_t. Measured units are determined due to ::hailo_power_measurement_types_t
     *         passed to 'Device::set_power_measurement'. Otherwise, returns a ::hailo_status error.
     * @note This function is deprecated. One should use "Device::get_power_measurement(hailo_measurement_buffer_index_t buffer_index, bool should_clear)'
     */
    Expected<hailo_power_measurement_data_t> get_power_measurement(uint32_t index, bool should_clear);

    /**
     * Start performing a long power measurement.
     * 
     * @param[in]   averaging_factor     Number of samples per time period, sensor configuration value.
     * @param[in]   sampling_period      Related conversion time, sensor configuration value.
     *                                   The sensor samples the power every sampling_period {ms} and averages every
     *                                   averaging_factor samples. The sensor provides a new value every: 2 * sampling_period * averaging_factor {ms}.
     *                                   The firmware wakes up every interval_milliseconds {ms} and checks the sensor.
     *                                   If there is a new value to read from the sensor, the firmware reads it.
     *                                   Note that the average calculated by the firmware is “average of averages”,
     *                                   because it averages values that have already been averaged by the sensor.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    hailo_status start_power_measurement(hailo_averaging_factor_t averaging_factor, hailo_sampling_period_t sampling_period);


    /**
     * Set parameters for long power measurement.
     * 
     * @param[in]   buffer_index       A ::hailo_measurement_buffer_index_t represents the buffer on the firmware the data would be saved at.
     *                                 Should match the one passed to 'Device::get_power_measurement'.
     * @param[in]   dvm                Which DVM will be measured. Default (::HAILO_DVM_OPTIONS_AUTO) will be different according to the board: <br>
     *                                 - Default (::HAILO_DVM_OPTIONS_AUTO) for EVB is an approximation to the total power consumption of the chip in PCIe setups.
     *                                 It sums ::HAILO_DVM_OPTIONS_VDD_CORE, ::HAILO_DVM_OPTIONS_MIPI_AVDD and ::HAILO_DVM_OPTIONS_AVDD_H.
     *                                 Only ::HAILO_POWER_MEASUREMENT_TYPES__POWER can measured with this option.
     *                                 - Default (::HAILO_DVM_OPTIONS_AUTO) for platforms supporting current monitoring (such as M.2 and mPCIe): OVERCURRENT_PROTECTION.
     * @param[in]   measurement_type   The type of the measurement. Choosing ::HAILO_POWER_MEASUREMENT_TYPES__AUTO
     *                                 will select the default value according to the supported features.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    hailo_status set_power_measurement(hailo_measurement_buffer_index_t buffer_index, hailo_dvm_options_t dvm, hailo_power_measurement_types_t measurement_type);

    /**
     * Read measured power from a long power measurement
     * 
     * @param[in]   buffer_index          A ::hailo_measurement_buffer_index_t represents the buffer on the firmware the data would be saved at.
     *                                    Should match the one passed to 'Device::set_power_measurement'.
     * @param[in]   should_clear          Flag indicating if the results saved at the firmware will be deleted after reading.
     * @return Upon success, returns @a hailo_power_measurement_data_t. Measured units are determined due to ::hailo_power_measurement_types_t
     *         passed to 'Device::set_power_measurement'. Otherwise, returns a ::hailo_status error.
     */
    Expected<hailo_power_measurement_data_t> get_power_measurement(hailo_measurement_buffer_index_t buffer_index, bool should_clear);

    /**
     * Stop performing a long power measurement.
     * 
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    hailo_status stop_power_measurement();

    /**
     * Get temperature information on the device
     *
     * @return Upon success, returns @a hailo_chip_temperature_info_t, containing temperature information on the device.
     *         Otherwise, returns a ::hailo_status error.
     * @note Temperature in Celsius of the 2 internal temperature sensors (TS).
     */
    Expected<hailo_chip_temperature_info_t> get_chip_temperature();

    /**
     * Reset device.
     * 
     * @param[in] mode      The mode of the reset.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     * @note Calling this function while running other operations on the device (including inference) will
     * lead to unexpected results!
     * @note The object used to call this function is not to be used after calling this function!
     * A new instance should be created.
     */
    virtual hailo_status reset(hailo_reset_device_mode_t mode) = 0;

    /**
     * Sets a callback to be called when a notification with ID @a notification_id will be received.
     *
     * @param[in] func                  The callback function to be called.
     * @param[in] notification_id       The ID of the notification.
     * @param[in] opaque                User specific data.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status set_notification_callback(const NotificationCallback &func, hailo_notification_id_t notification_id,
        void *opaque) = 0;

    /**
     * Removes a previously set callback with ID @a notification_id.
     *
     * @param[in] notification_id       The ID of the notification to remove.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status remove_notification_callback(hailo_notification_id_t notification_id) = 0;

    /**
     *  Test chip memories using BIST.
     *
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     * @note Cannot be called during inference!
     */
    hailo_status test_chip_memories();

    /**
     *  Update the firmware of a Hailo device.
     * 
     * @param[in] firmware_binary       The firmware code to be updated to the device.
     * @param[in] should_reset          Bool indicating weather to reset the device after updating.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     * @note Calling this function while running other operations on the device (including inference) will
     * lead to unexpected results!
     * @note The object used to call this function is not to be used after calling this function!
     * A new instance should be created.
     */
    virtual hailo_status firmware_update(const MemoryView &firmware_binary, bool should_reset) = 0;

    /**
     *  Update the second stage binary.
     * 
     * @param[in] second_stage_binary           The SSB code to be updated to the device.
     * @param[in] second_stage_binary_length    The length of the SSB to be updated.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     * @note Calling this function while running other operations on the device (including inference) will
     * lead to unexpected results!
     * @note The object used to call this function is not to be used after calling this function!
     * A new instance should be created.
     */
    virtual hailo_status second_stage_update(uint8_t *second_stage_binary, uint32_t second_stage_binary_length) = 0;

    /**
     * Store sensor configuration to Hailo chip flash memory.
     *
     * @param[in] section_index         Flash section index to write to. [0-6]
     * @param[in] sensor_type           Sensor type.
     * @param[in] reset_config_size     Size of reset configuration.
     * @param[in] config_height         Configuration resolution height.
     * @param[in] config_width          Configuration resolution width.
     * @param[in] config_fps            Configuration FPS.
     * @param[in] config_file_path      Sensor configuration file path.
     * @param[in] config_name           Sensor configuration name.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status store_sensor_config(uint32_t section_index, hailo_sensor_types_t sensor_type,
        uint32_t reset_config_size, uint16_t config_height, uint16_t config_width, uint16_t config_fps,
        const std::string &config_file_path, const std::string &config_name) = 0;
    
    /**
     * Store sensor ISP configuration to Hailo chip flash memory.
     *
     * @param[in] reset_config_size                 Size of reset configuration.
     * @param[in] config_height                     Configuration resolution height.
     * @param[in] config_width                      Configuration resolution width.
     * @param[in] config_fps                        Configuration FPS.
     * @param[in] isp_static_config_file_path       ISP static configuration file path.
     * @param[in] isp_runtime_config_file_path      ISP runtime configuration file path.
     * @param[in] config_name                       Sensor configuration name.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status store_isp_config(uint32_t reset_config_size, uint16_t config_height, uint16_t config_width, uint16_t config_fps,
        const std::string &isp_static_config_file_path, const std::string &isp_runtime_config_file_path, const std::string &config_name) = 0;
    
    /**
     * Gets the sections information of the sensor.
     *
     * @return Upon success, returns Expected of a buffer containing the sensor's sections information.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    virtual Expected<Buffer> sensor_get_sections_info() = 0;

    /**
     * Dump config of given section index into a csv file.
     *
     * @param[in] section_index         Flash section index to load config from. [0-7]
     * @param[in] config_file_path      File path to dump section configuration into.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status sensor_dump_config(uint32_t section_index, const std::string &config_file_path) = 0;

    /**
     * Set the I2C bus to which the sensor of the specified type is connected.
     *
     * @param[in] sensor_type           The sensor type.
     * @param[in] bus_index             The I2C bus index of the sensor.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status sensor_set_i2c_bus_index(hailo_sensor_types_t sensor_type, uint32_t bus_index) = 0;

    /**
     * Load the configuration with I2C in the section index.
     *
     * @param[in] section_index         Flash section index to load config from. [0-6]
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status sensor_load_and_start_config(uint32_t section_index) = 0;

    /**
     * Reset the sensor that is related to the section index config.
     *
     * @param[in] section_index         Flash section index to load config from. [0-6]
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status sensor_reset(uint32_t section_index) = 0;

    /**
     * Set a generic I2C slave for sensor usage.
     *
     * @param[in] slave_address         The address of the i2c slave.
     * @param[in] offset_size           Slave offset size (in bytes).
     * @param[in] bus_index             The bus number the i2c slave is connected to.
     * @param[in] should_hold_bus       Should hold the bus during the read.
     * @param[in] slave_endianness      BIG_ENDIAN or LITTEL_ENDIAN.
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status sensor_set_generic_i2c_slave(uint16_t slave_address, uint8_t offset_size, uint8_t bus_index,
        uint8_t should_hold_bus, uint8_t slave_endianness) = 0;

    /**
     * Reads board configuration from device.
     * 
     * @return Upon success, returns Expected of a buffer containing the data read.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */  
    virtual Expected<Buffer> read_board_config() = 0;

    /**
     * Write board configuration to device
     * 
     * @param[in] buffer        A buffer that contains the data to be written .
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */  
    virtual hailo_status write_board_config(const MemoryView &buffer) = 0;

    /**
     * Gets firmware user configuration information from device.
     * 
     * @return Upon success, returns Expected of ::hailo_fw_user_config_information_t.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */  
    virtual Expected<hailo_fw_user_config_information_t> examine_user_config() = 0;

    /**
     * Reads firmware user configuration from device.
     * 
     * @return Upon success, returns Expected of a buffer containing the data read.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    virtual Expected<Buffer> read_user_config() = 0;

    /**
     * Write firmware user configuration to device.
     * 
     * @param[in] buffer        A buffer that contains the data to be written .
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */  
    virtual hailo_status write_user_config(const MemoryView &buffer) = 0;

    /**
     * Erase firmware user configuration from the device.
     * 
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
     */
    virtual hailo_status erase_user_config() = 0;

    /**
     * Gets the device architecture.
     * 
     * @return Upon success, returns Expected of ::hailo_device_architecture_t.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    virtual Expected<hailo_device_architecture_t> get_architecture() const = 0;

    /**
     * Gets the device type.
     * 
     * @return Upon success, returns Type. Otherwise, returns a ::hailo_status error.
     */
    Type get_type() const;

    /**
     * Gets the device id.
     * 
     * @return An identification string of the device.
     *      For Pcie device, returns the BDF.
     *      For Ethernet device, returns the IP address.
     *      For Core device, returns "Core".
     */
    virtual const char* get_dev_id() const = 0;

    /**
     * Gets the stream's default interface.
     * 
     * @return Upon success, returns Expected of ::hailo_stream_interface_t.
     *         Otherwise, returns Unexpected of ::hailo_status error.
     */
    Expected<hailo_stream_interface_t> get_default_streams_interface() const;

    /**
     * @return true if the stream's interface is supported, false otherwise.
     */
    virtual bool is_stream_interface_supported(const hailo_stream_interface_t &stream_interface) const = 0;

    virtual hailo_status direct_write_memory(uint32_t address, const void *buffer, uint32_t size);
    virtual hailo_status direct_read_memory(uint32_t address, void *buffer, uint32_t size);
    hailo_status set_overcurrent_state(bool should_activate);
    Expected<bool> get_overcurrent_state();
    Expected<hailo_health_info_t> get_health_information();
    // Returns a vector of the number of contexts per network group (preliminary + dynamic)
    // The sum of the number of contexts will fit in uint8_t
    Expected<std::vector<uint8_t>> get_number_of_contexts_per_network_group();
    Expected<Buffer> download_context_action_list(uint8_t context_index, uint32_t *base_address,
        uint32_t *batch_counter, uint16_t max_size = 10000);
    // The batch configured is reset between network groups
    hailo_status set_context_action_list_timestamp_batch(uint16_t batch_index);

    virtual ~Device() = default;
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
    Device(Device &&) = delete;
    Device &operator=(Device &&other) = delete;

protected:
    Device(Type type);

    virtual hailo_status wait_for_wakeup() = 0;
    virtual void increment_control_sequence() = 0;
    hailo_status fw_interact(uint8_t *request_buffer, size_t request_size, uint8_t *response_buffer, size_t *response_size);
    virtual hailo_status fw_interact_impl(uint8_t *request_buffer, size_t request_size, uint8_t *response_buffer, 
                                          size_t *response_size, hailo_cpu_id_t cpu_id) = 0;
    
    // Update the state of the fw, as seen by this device
    hailo_status update_fw_state();

    Type m_type;
    uint32_t m_control_sequence;
    bool m_is_control_version_supported;
    hailo_device_architecture_t m_device_architecture;

private:
    uint32_t get_control_sequence();
    bool is_control_version_supported();

    friend class Control;
};

} /* namespace hailort */

#endif /* _HAILO_DEVICE_HPP_ */
