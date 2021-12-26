//
// Created by chazz on 22/12/2021.
//

#include <R2000/StatusWatcher.hpp>
#include <utility>

Device::DeviceStatus::DeviceStatus(Device::ParametersMap parameters) : systemStatusMap(std::move(parameters)){

}

Device::StatusFlagInterpretation::StatusFlagInterpretation(const uint32_t flags)
        : flags(flags) {}


