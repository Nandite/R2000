R2000
===============

This is an implementation of a communication interface for the [OMDxxx - R2000](https://www.pepperl-fuchs.com/global/en/R2000_Detection_laser_scanner.htm) 
sensors series of Pepperl+Fuchs in C++17 for embedded applications. 
This interface offers to quickly set up the sensor for scan acquisition in TCP or UDP channels. Advanced configuration 
for the scan acquisition is also available depending on the needs. The sensor can also be parameterized in details using
the facilities of the interface. The implementation was done based on the [Protocol Version 1.04](docs/doct3469f.pdf).
Most of the functionalities offered by this version of the communication protocol are supported by the current implementation.
However, this interface can still be used with sensors having lower protocol version. Some command will just not be available
on the sensor side and will end up in errors on execution attempt.

## Dependencies

This project depends on:
- [Boost](https://www.boost.org/) >= 1.64 
- A [compiler](https://en.cppreference.com/w/cpp/compiler_support) supporting >= C++17

## Quick setup

The below example show how to set up a data link to receive scan from the sensor synchronously using TCP:   
```cpp
const auto device{Device::R2000::makeShared({"R2000", deviceAddress})};
auto handleParameters{Device::Parameters::ReadWriteParameters::TcpHandle{}
                              .withWatchdog()
                              .withWatchdogTimeout(5000)};
auto [requestResult, dataLink]{Device::DataLinkBuilder(handleParameters).build(device)}; // Blocking build
if(requestResult != Device::RequestResult::SUCCESS)
{
     // There was an error building the channel, check requestResult value
}
// At this stage the channel is build and the sensor is sending data
dataLink->addOnNewScanAvailableCallback([](const auto& newScan) { 
            // A new scan has been delivered, do something with it ! 
}); // Add a new callback to be notified when a new scan is available
```

You can also do it asynchronously (advised):

```cpp
const auto device{Device::R2000::makeShared({"R2000", deviceAddress})}; // Make a new device
auto handleParameters{Device::Parameters::ReadWriteParameters::TcpHandle{}
                              .withWatchdog()
                              .withWatchdogTimeout(5000)}; // Request a Tcp channel with watchdog enabled
auto future{Device::DataLinkBuilder(handleParameters).build(device, 1s)}; // Build the channel asynchronously with 1s timeout
// Do some stuff if you want while the channel is being built
auto [asyncRequestResult, dataLink]{future.get()}; // Get the result and the DataLink
if (asyncRequestResult != Device::RequestResult::SUCCESS) {
    // There was an error building the channel, check requestResult value
}
// At this stage the channel is build and the sensor is sending data
dataLink->addOnNewScanAvailableCallback([](const auto& newScan) { 
            // A new scan has been delivered, do something with it ! 
}); // Add a new callback to be notified when a new scan is available
```
Note: Doing the operations asynchronously is preferable as you can set a timeout to avoid lengthy freeze in case of
network failures.

## How does it work ?

The interface is articulated around three concepts:
- **Commands and Parameters**, allow to control the sensor and configure it.
- **DataLinks**, allow to stream scans from the sensor using TCP or UDP
- **StatusWatcher**, watch for events and queries the device periodically and gather analytics data on the status of the sensor.

All these concepts revolve around the **R2000** device object. The first thing to do is to always instantiate a device:
```cpp
DeviceConfiguration configuration{"DeviceName", "192.168.1.60"}
const auto device{Device::R2000::makeShared(configuration)};
//or
const auto device{Device::R2000::makeShared({"DeviceName", "192.168.1.60"})};
```
A **R2000** cannot be directly constructed. it can only be obtained through the static _**makeShared**_ method. The
method returns a new instance of R2000 through a shared_ptr. The R2000 instance object is necessary to use any of the
above three concepts.

For every sensor on the network, an instance of **R2000** can be constructed to interact with it.

### Parameters and Commands 

#### Parameters

To configure or interact with the sensor, Parameters and Commands objects are used. These objects are a direct materialization 
of the parameters and commands mentioned in the [Protocol Version](docs/doct3469f.pdf) documentation.

The parameters are divided eight groups over two access type, ReadWrite and ReadOnly:
- BasicInformation, provides basic information on the sensor (RW,RO)
- Capabilities, provides information about the sensor capabilities (RO) 
- Ethernet, control the sensor ethernet configuration (RW,RO)
- Measure, control the sensor measurement configuration (RW,RO)
- HmiDisplay, control the sensor HMI (RW)
- SystemStatus, provides information about the status of the sensor (RO)
- UdpHandle, used to set up a UDP scan stream with custom parameters (RW)
- TcpHandle, used to set up a TCP scan stream with custom parameters (RW)

The parameters are basically [builders](https://en.wikipedia.org/wiki/Builder_pattern) using the [fluent interface pattern](https://en.wikipedia.org/wiki/Fluent_interface)
to easily compose a set of parameters that can be read or modified by a command. Setting up a builder necessitate the specification of its parameters access type, meaning ReadOnly
or ReadWrite, then specify the parameters we want to act upon or read from.
Example with RW parameters:
```cpp
#include "Control/Parameters.hpp"

//...

// Request the HMI parameters as RW then unlock the HMI button, set the display language to english, set the display mode
// to application text then provides the two lines of text to display.
const auto hmiParameters{Device::Parameters::ReadWriteParameters::HmiDisplay()
                                 .unlockHmiButton()
                                 .withHmiLanguage(Device::Parameters::Language::ENGLISH)
                                 .withHmiDisplayMode(Device::Parameters::HMI_DISPLAY_MODE::APPLICATION_TEXT)
                                 .withHmiApplicationText1("Line 1")
                                 .withHmiApplicationText2("Line 2")};
// Pass the parameters to a special command, so they can be written into the sensor.
// ...

// Set the operating mode to measure (activate the measuring head), the frequency of scan to 35 Hz, the number of points
// per scan to 7200 and the direction of rotation to counter clock wise.
const auto measurementParameters{Device::Parameters::ReadWriteParameters::Measure()
                                  .withOperatingMode(Device::Parameters::OPERATING_MODE::MEASURE)
                                  .withScanFrequency(35.0)
                                  .withSamplesPerScan(7200)
                                  .withScanDirection(Device::Parameters::SCAN_DIRECTION::CCW)};
// Pass the parameters to a special command, so they can be written into the sensor.
// ...
```
Example with RO parameters:
```cpp
// Request a bunch of parameters from the status of the sensor.
const auto systemParameters{Parameters::ReadOnlyParameters::SystemStatus{}.requestLoadIndication()
                        .requestSystemTimeRaw()
                        .requestUpTime()
                        .requestPowerCycles()
                        .requestOperationTime()
                        .requestOperationTimeScaled()
                        .requestCurrentTemperature()
                        .requestMinimalTemperature()
                        .requestMaximalTemperature()
                        .requestStatusFlags()};
```
There are builders for all the eight group of parameters and their access type. 

#### Commands

The Commands object are lightweight proxies that take Parameters objects (or nothing) as input and form a valid expression 
interpretable by the sensor to modify its internal state or to perform an action.
There are sixteen constructible commands available to interact with the sensors:
```cpp
#include <Control/Commands.hpp>
// ...
Device::Commands::GetProtocolVersionCommand getProtocolVersionCommand{*device};
Device::Commands::GetProtocolInfoCommand getProtocolInfoCommand{*device};
Device::Commands::ReleaseHandleCommand releaseHandleCommand{*device};
Device::Commands::StartScanCommand startScanCommand{*device};
Device::Commands::StopScanCommand stopScanCommand{*device};
Device::Commands::FeedWatchdogCommand feedWatchdogCommand{*device};
Device::Commands::GetParametersCommand getParametersCommand{*device};
Device::Commands::FetchParametersCommand fetchParametersCommand{*device};
Device::Commands::SetParametersCommand setParametersCommand{*device};
Device::Commands::FactoryResetParametersCommand factoryResetParametersCommand{*device};
Device::Commands::FactoryResetDeviceCommand factoryResetDeviceCommand{*device};
Device::Commands::RebootDeviceCommand rebootDeviceCommand{*device};
Device::Commands::RequestUdpHandleCommand requestUdpHandleCommand{*device};
Device::Commands::RequestTcpHandleCommand requestTcpHandleCommand{*device};
Device::Commands::GetScanOutputConfigCommand getScanOutputConfigCommand{*device};
Device::Commands::SetScanOutputConfigCommand setScanOutputConfigCommand{*device};
```
On construction, a command is given an instance of **R2000** to operate on. Hence, each instance of a Command is attached to a sensor.
All commands can be executed in three modes:
- synchronously 
- asynchronously with future/promise 
- asynchronously using callbacks

```cpp
Device::Commands::SetParametersCommand setParametersCommand{*device};
//...
auto result{setParametersCommand.execute(hmiParameters)}; // Blocking call until the command is executed or fails

// or    

auto future{setParametersCommand.asyncExecute(2s, hmiParameters)}; // Asynchronous execution with 2 seconds timeout
if(!future)
{
    // The command could not been submitted for execution
}
// Do something else ...
auto result{future->get()};

// or

const auto started{setParametersCommand.asyncExecute(hmiParameters, [](const auto result){
    // Do something with the result of the command execution
}, 2s)}; // Asynchronous execution with 2 seconds timeout

if(started)
{
    // The command has been submitted for execution
}
```

The commands take different number of arguments depending on their action.
```cpp
// Rebooting the device takes no arguments
const auto requestResult{rebootDeviceCommand.execute()};

// Asynchronously rebooting the device only takes the command timeout.
auto future{rebootDeviceCommand.asyncExecute(2s)};
if(!future)
{
    // ...
}

```
Some even takes a variadic number of arguments:
```cpp
const auto hmiParameters{Device::Parameters::ReadWriteParameters::HmiDisplay()
                                 .unlockHmiButton()
                                 .withHmiLanguage(Device::Parameters::Language::ENGLISH)
                                 .withHmiDisplayMode(Device::Parameters::HMI_DISPLAY_MODE::APPLICATION_TEXT)
                                 .withHmiApplicationText1("Line 1")
                                 .withHmiApplicationText2("Line 2")};
const auto measurementParameters{Device::Parameters::ReadWriteParameters::Measure()
                                     .withOperatingMode(Device::Parameters::OPERATING_MODE::MEASURE)
                                     .withScanFrequency(35.0)
                                     .withSamplesPerScan(7200)
                                     .withScanDirection(Device::Parameters::SCAN_DIRECTION::CCW)};
const auto ethernetParameters{Device::Parameters::ReadWriteParameters::Ethernet ()
                                     .withGateway("192.168.1.1")
                                     .withIpAddress("192.168.1.72")
                                     .withIpMode(Device::Parameters::IpMode::STATIC)
                                     .withSubnetMask("255.255.255.0")};

Device::Commands::SetParametersCommand setParametersCommand{*device};
// Set parameters command takes a variadic number of parameters builder, allowing to configure 
// multiple section of the sensor.
auto future{setParametersCommand.asyncExecute(2s, hmiParameters, 
                                              measurementParameters, 
                                              ethernetParameters, ...)};
if(!future)
{
    // The command could not been submitted for execution
}
```

### DataLinks

The DataLinks are the objects that handle the streaming of scan from the sensor. There is two type of link
implemented, TCP and UDP links. DataLinks are RAII by design. On construction, they immediately open the channel with
the sensor and start the streaming of scan. On destruction, they close the channel and release any resources allocated
for the stream of scan.

DataLinks must not be instantiated directly. They are built using a special factory class named **DataLinkBuilder**.
Setting up a **DataLink** necessitate to send commands to the sensors, get the results and interpret it, extract some
parameters that are used by the Links. All of this is automatically handled by the **DataLinkBuilder**. Depending on the
type of Parameters handle used with the builder, it will set up a TCP or UDP link while abstracting all the configuration
that goes underneath:
```cpp
// Building a TCP Link:
auto tcpHandleParameters{Device::Parameters::ReadWriteParameters::TcpHandle{}
                                .withWatchdog()
                                .withWatchdogTimeout(5000)
                                // ... other parameters
                                };
auto [requestResult, dataLink]{Device::DataLinkBuilder(handleParameters).build(device)}; // Build a TCP Link

// Building and UDP Link (asynchronously):
auto udpHandleParameters{Device::Parameters::ReadWriteParameters::UdpHandle{}
                              .withHostname("192.168.1.5") // In UDP, you must specify the destination of the packet scan
                              .withPort(62000) // And the destination port
                              .withPacketType(Device::Parameters::PACKET_TYPE::B)
                              .withWatchdog()
                              .withWatchdogTimeout(5000)};
auto future{Device::DataLinkBuilder(udpHandleParameters).build(device, 1s)};
// Do something else
auto [requestResult, dataLink]{future.get()};
```

There is four different way to get the received scans from a DataLink depending on the needs:
```cpp
using namespace std::chrono_literals;
// ...
const auto scan {dataLink->getLastScan()}; // Get the last scan received (wait-free, lock-free)
// or
const auto scan {dataLink->waitForNextScan()}; // Block until the next scan is received
// or
const auto scan {dataLink->waitForNextScan(750ms)}; // Block until the next scan is received or timeout is reached
// or
dataLink->addOnNewScanAvailableCallback([](const auto& newScan){ 
    
}); // Register a callback to listen for the arrival of every new scan.
```

### StatusWatcher

The StatusWatcher is a utility object that monitor the network and health status of the sensor periodically and can raise
events. It can be used to listen for device connection/disconnection or to generate reports about the current state of the
device.
```cpp
const auto device{Device::R2000::makeShared({"DeviceName", "192.168.1.60"})};
// ...
Device::StatusWatcher statusWatcher{device, 2s}; // Watcher with a period of 2s

statusWatcher.addOnDeviceConnectedCallback([](){ 
    // Device has connected
}); // Register a callback to listen for device connection

statusWatcher.addOnDeviceDisconnectedCallback([](){ 
    // Device has disconnected
}); // Register a callback to listen for device disconnection
```
The watcher generate periodically an object named **DeviceStatus**. This object contains all the status information about
the sensor at a given time T. There are two different method of getting the status:
```cpp
const auto deviceStatus{statusWatcher->getLastReceivedStatus()}; // Get the last received status (lock-free, wait-free)
// or 
statusWatcher->addOnStatusAvailableCallback([](auto status)
{
    // A new status has been received.
}); // Register a callback to listen for the arrival of every new status.
```
The DeviceStatus object exposes a list of getter on the internal state of the sensor:

```cpp
const auto deviceStatus{statusWatcher->getLastReceivedStatus()};
const auto rawSystemTime{deviceStatus->getRawSystemTime()};
const auto cpuLoad{deviceStatus->getCpuLoad()};
const auto currentTemperature{deviceStatus->getCurrentTemperature()};
const auto statusFlags{deviceStatus->getStatusFlags()};
// ...
const auto error{statusFlags.deviceHasError()};
const auto deviceIsInitializing{statusFlags.isInitializing()};
// ...
```

## Building the examples

There is a set of executables in the [examples](examples) to test the interface. You can build them using the following commands:
```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
It will generate 3 executables: 
- DeviceEventWatch: shows how to use the StatusWatcher to listen for connection/disconnection.
- DeviceStatusWatch: shows how to use the StatusWatcher to monitor the device.
- AcquireScan: show how to quickly set up a DataLink to acquire scans.

```sh
./DeviceEventWatch 192.168.2.30
# or
./DeviceStatusWatch 192.168.2.30
# or
./AcquireScan 192.168.2.30
```

## Troubleshooting

- Using the UDP mode, mind that the address given to the builder with the UdpHandle is not the device address, but the address where the sensor must send the datagrams.
It is usually the address of the machine running this communication interface.
- Be careful of the firewall. It can restrict both the configuration packet on TCP and the scans on TCP and UDP.
In some cases, the firewall allows the configuration packet to flow through, but the DataLinks doesn't receive the
scans in UDP. Make sure that the firewall accepts both the configuration packets on TCP and the scans on UDP.
- Using the DataLinks without watchdog enabled and killing the application without destroying the DataLinks 
can cause "zombie" links within the sensors. Current software revisions of the R2000 only allows at most 3 streams of scans.
Trying to set up more DataLink when this maximum number is reached (due to zombie links or because the device is already
streaming scans to 3 clients) will result in failure.
**In any case, it is strongly advised to set up the DataLinks with watchdog enabled, so that the sensor can kill 
automatically "zombie" links.**

## Feedback

Don't hesitate if you have any suggestions for improving this project, or if you find any error. I will be glad to
hear from you. There are traits that have been implemented yet. You can submit implementations suggestions.
Contributions are welcomed :)

## License

Distributed under the MIT Software License (X11 license).
See accompanying file LICENSE.