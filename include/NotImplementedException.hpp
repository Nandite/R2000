//
// Created by chazz on 22/01/2022.
//

#pragma once

namespace Device {
    /**
     * Raised when a call to an unimplemented method is performed.
     */
    class NotImplementedException : public std::logic_error {
    public:
        inline explicit NotImplementedException(const std::string &message) : std::logic_error(message) {};
    };
}