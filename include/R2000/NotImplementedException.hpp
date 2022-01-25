//
// Created by chazz on 22/01/2022.
//

#pragma once

namespace Device {
    class NotImplemented : public std::logic_error {
    public:
        inline explicit NotImplemented(const std::string &message) : std::logic_error(message) {};
    };
}