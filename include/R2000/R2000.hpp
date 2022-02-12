//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file

#pragma once

#include "Worker.hpp"
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <list>
#include <optional>
#include <utility>
#include "Control/Parameters.hpp"

using namespace std::chrono_literals;

namespace Device {
    namespace internals {
        /**
         * RAII wrapper over a socket that shut it down and close it on destruction.
         * This wrapper only support Boost sockets.
         * @tparam Socket The Type of socket to wrap.
         */
        template<typename Socket>
        class SocketGuard {
        public:
            /**
             *
             * @tparam Args Type of the argument pack of the socket constructor.
             * @param args Arguments of the socket constructor.
             */
            template<typename... Args>
            explicit SocketGuard(Args &&... args) : socket(std::forward<Args>(args)...) {}

            /**
             * @return Get the guarded socket.
             */
            inline Socket &operator*() { return socket; }

            /**
             * Shutdown and close the socket on destruction of this instance.
             */
            ~SocketGuard() {
                boost::system::error_code placeholder{};
                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, placeholder);
                socket.close(placeholder);
            }

        private:
            Socket socket;
        };

        /**
             * Get the underlying value of an enumeration.
             * @tparam E The type of the enumerator.
             * @param enumerator The instance of enumerator to get the value.
             * @return The underlying value of the enumeration.
             */
        template<typename E>
        static constexpr auto underlyingType(E enumerator) {
            return static_cast<std::underlying_type_t<E>>(enumerator);
        }
    } // namespace internals
    using PropertyTree = boost::property_tree::ptree;

    class DeviceConfiguration {
    public:
        /**
         * @param name The name of the device.
         * @param deviceAddress The ipv4 device address.
         * @param port The Http port of the device.
         */
        [[maybe_unused]] DeviceConfiguration(std::string name, const std::string &deviceAddress,
                                             unsigned short port = 80)
                : name(std::move(name)),
                  deviceAddress(boost::asio::ip::address::from_string(deviceAddress)),
                  httpServicePort(port) {
        }

    public:
        std::string name;
        const boost::asio::ip::address deviceAddress{};
        const unsigned short httpServicePort{};
    };

    /**
     * Result of an asynchronous request.
     */
    enum class RequestResult {
        SUCCESS = 200,
        BAD_REQUEST = 400,
        FORBIDDEN = 403,
        UNKNOWN_COMMAND = 404,
        TIMEOUT = 408,
        FAILED = 800,
        INVALID_DEVICE_RESPONSE = 801
    };

    constexpr inline RequestResult requestResultFromCode(const unsigned int code) noexcept {
        switch (code) {
            case internals::underlyingType(RequestResult::SUCCESS): {
                return RequestResult::SUCCESS;
            }
            case internals::underlyingType(RequestResult::BAD_REQUEST): {
                return RequestResult::BAD_REQUEST;
            }
            case internals::underlyingType(RequestResult::FORBIDDEN): {
                return RequestResult::FORBIDDEN;
            }
            case internals::underlyingType(RequestResult::UNKNOWN_COMMAND): {
                return RequestResult::UNKNOWN_COMMAND;
            }
            case internals::underlyingType(RequestResult::TIMEOUT): {
                return RequestResult::TIMEOUT;
            }
            case internals::underlyingType(RequestResult::INVALID_DEVICE_RESPONSE): {
                return RequestResult::INVALID_DEVICE_RESPONSE;
            }
            default: {
                return RequestResult::FAILED;
            }
        }
    }

    /**
     * Convert a RequestResult to a string.
     * @param result The RequestResult to convert.
     * @return The RequestResult as a string.
     */
    inline std::string asyncResultToString(RequestResult result) {
        switch (result) {
            case RequestResult::SUCCESS:
                return "Success";
            case RequestResult::FAILED:
                return "Failed";
            case RequestResult::TIMEOUT:
                return "Timeout";
            case RequestResult::BAD_REQUEST:
                return "Bad Request";
            case RequestResult::FORBIDDEN:
                return "Forbidden";
            case RequestResult::UNKNOWN_COMMAND:
                return "Not Found";
            case RequestResult::INVALID_DEVICE_RESPONSE:
                return "Invalid device response";
        }
        return "Unknown";
    }

    class DeviceAnswer {
    public:

        /**
         * @param result The result of the asynchronous request.
         */
        explicit DeviceAnswer(RequestResult result);

        /**
         * @param result The result of the asynchronous request.
         * @param tree The property tree obtained after the asynchronous HTTP request.
         */
        DeviceAnswer(RequestResult result, const PropertyTree &tree);

        /**
         * Test either or not the asynchronous has succeed.
         * @return True if the request has yielded a SUCCESS value.
         */
        inline explicit operator bool() const { return requestResult == RequestResult::SUCCESS; }

        /**
         * Compare this instance with a RequestResult value.
         * @param rhs The RequestResult to test against.
         * @return True if the RequestResult stored by this instance is equals to the parameter rhs.
         */
        inline bool operator==(const RequestResult &rhs) const { return requestResult == rhs; }

        /**
         * Compare this instance with a RequestResult value.
         * @param rhs The RequestResult to test against.
         * @return True if the RequestResult stored by this instance is different to the parameter rhs.
         */
        inline bool operator!=(const RequestResult &rhs) const { return !(rhs == (*this).requestResult); }

        /**
         * @return the property tree obtained after the HTTP request.
         */
        [[nodiscard]] inline const PropertyTree &getPropertyTree() const { return propertyTree; }

        /**
         * @return Get the RequestResult of the HTTP request.
         */
        [[nodiscard]] inline RequestResult getRequestResult() const { return requestResult; }

    private:
        RequestResult requestResult{};
        PropertyTree propertyTree{};
    };

    class R2000 {
    public:
        using CommandCallback = std::function<void(const DeviceAnswer &)>;
    private:

        /**
         * Setup a new HTTP command interface.
         * @param configuration The device address and the chosen port.
         */
        explicit R2000(DeviceConfiguration configuration);

        /**
         * Enable this class to be built through the make_shared method.
         * @tparam Arg Type of the argument pack of the class constructor.
         * @param arg Argument pack of the class constructor.
         * @return A shared_ptr of this class.
         */
        template<typename... Arg>
        std::shared_ptr<R2000> static enableMakeShared(Arg &&... arg) {
            struct EnableMakeShared : public R2000 {
                explicit EnableMakeShared(Arg &&... arg) : R2000(std::forward<Arg>(arg)...) {}
            };
            return std::make_shared<EnableMakeShared>(std::forward<Arg>(arg)...);
        }

    public:
        [[maybe_unused]] static std::shared_ptr<R2000> makeShared(const DeviceConfiguration &configuration);

        /**
         * Send a HTTP Command with only one parameter and its corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameter Parameters attached to the command to send.
         * @param value Values of the parameters.
         * @return A pair containing as first a flag indicating either or not the command has been successfully sent
         * and a reply received. The second of the pair is a property tree representing the content of the Http answer.
         */
        [[nodiscard]] DeviceAnswer sendHttpCommand(const std::string &command,
                                                   const std::string &parameter = "",
                                                   std::string value = "") const noexcept(false);

        /**
         * Send a HTTP Command with several parameters and their corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameters Parameters and values map attached to the command to send.
         * @return A pair containing as first a flag indicating either or not the command has been successfully sent
         * and a reply received. The second of the pair is a property tree representing the content of the Http answer.
         */
        [[nodiscard]] DeviceAnswer sendHttpCommand(const std::string &command,
                                                   const Parameters::ParametersMap &parameters) const
        noexcept(false);

        /**
         * Asynchronously send a HTTP Command without any parameters to the device.
         * @param command The command to send to the device.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the requests queue is full.
         */
        bool asyncSendHttpCommand(const std::string &command, CommandCallback callable,
                                  std::chrono::milliseconds timeout = 5000ms) noexcept(true);

        /**
         * Asynchronously send a HTTP Command with several parameters and their corresponding value to the device.
         * @param deviceAnswer The deviceAnswer to send to the device.
         * @param parameters Parameters attached to the deviceAnswer to send.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the requests queue is full.
         */
        bool asyncSendHttpCommand(const std::string &deviceAnswer, const Parameters::ParametersMap &parameters,
                                  CommandCallback callable,
                                  std::chrono::milliseconds timeout = 5000ms) noexcept(true);

        /**
         * Cancel the pending command being run by the device.
         */
        void cancelPendingCommands() noexcept;

        /**
         * @return The device hostname.
         */
        [[nodiscard]] inline auto getHostname() const { return configuration.deviceAddress; }

        /**
         * @return The device name.
         */
        [[nodiscard]] inline auto getName() const { return configuration.name; }

        /**
         * Release the resources (mainly network)
         */
        virtual ~R2000();

    private:
        /**
         * Construct a string representation of a command given a map of parameters.
         * @param command The input command.
         * @param parameters The map of parameters associated with the command.
         * @return A string representation of the command and the parameters.
         */
        [[nodiscard]] static std::string makeRequestFromParameters(const std::string &command,
                                                                   const Parameters::ParametersMap &parameters);

        /**
         * Get a resolver query from the device configuration that is used to connect to the socket.
         * @return a resolver query.
         */
        [[nodiscard]] static boost::asio::ip::tcp::resolver::query
        getDeviceQuery(const DeviceConfiguration &configuration);

        /**
         * Send a HTTP GET request to the device. This methods blocks until the socket is connected.
         * @param requestPath The http request path to send.
         * @return A HttpResult object containing the status code, the header and the content of the answer.
         */
        [[nodiscard]] DeviceAnswer HttpGet(const std::string &requestPath) const noexcept(false);

        /**
         * Send a HTTP GET to a remote endpoint through a given socket.
         * @param socket The socket to send the request through and read the answer from.
         * @param requestPath The http request path to send.
         * @return  A HttpResult object containing the status code, the header and the content of the answer.
         */
        [[nodiscard]] static DeviceAnswer HttpGet(boost::asio::ip::tcp::socket &socket, const std::string &requestPath);

        /**
         * Asynchronously send a HTTP GET request to the device with a timeout.
         * @param request The http request to send.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the request queue is full.
         */
        bool AsyncHttpGet(const std::string &request, CommandCallback callable,
                          std::chrono::milliseconds timeout) noexcept(true);

        /**
         * Handle the resolution of connection to endpoints and initiate a socket connection to send a http request.
         * @param error Error encountered during the resolution if any.
         * @param request The http request to send if the resolution has succeeded.
         * @param callable The http callback to execute once the http request is sent.
         * @param endpoints The endpoints to try to connect the socket to.
         * @param timeout The timeout of the socket connection before being cancelled.
         */
        void onEndpointResolutionAttemptCompleted(const boost::system::error_code &error, const std::string &request,
                                                  CommandCallback callable,
                                                  const boost::asio::ip::tcp::resolver::results_type &endpoints,
                                                  std::chrono::milliseconds timeout);

        /**
         * Execute the io service run function until this instance is destroyed.
         */
        void ioServiceTask();

        /**
         * Handle the connection of the socket and send a http request.
         * @param error Error encountered during the socket connection if any.
         * @param request The http request to send if the resolution has succeeded.
         * @param callable The http callback to execute once the http request is sent.
         */
        void onSocketConnectionAttemptCompleted(const boost::system::error_code &error, const std::string &request,
                                                const CommandCallback &callable);

        /**
         * Verify if the sensor does not generate an error.
         * @param tree Represent the property tree.
         * @return True if the error code is equal to 0 and the error text is equal to "success", false otherwise.
         */
        static bool verifyErrorCode(const PropertyTree &tree);

        /**
         * Handle the asynchronous endpoints resolver deadline.
         * @param error Error encountered while waiting for the deadline of the resolver.
         */
        void onEndpointResolutionDeadlineReached();

        /**
         * Handle the asynchronous socket connection deadline.
         * @param error Error encountered while waiting for the socket to connect.
         */
        void onSocketConnectionDeadlineReached();

        /**
         * Schedule a timeout deadline for the asynchronous resolver.
         * @param timeout Maximum time allowed for the resolver to complete before cancelling the operation.
         */
        void scheduleEndpointResolverNextDeadline(std::chrono::milliseconds timeout);

        /**
         * Schedule a timeout deadline for the asynchronous socket connection.
         * @param timeout Maximum time allowed for the socket to connect before cancelling the operation.
         */
        void scheduleSocketConnectionNextDeadline(std::chrono::milliseconds timeout);

    private:
        DeviceConfiguration configuration;
        std::future<void> ioServiceTaskFuture{};
        mutable boost::asio::io_service ioService{};
        boost::asio::steady_timer resolverDeadline{ioService};
        boost::asio::steady_timer socketDeadline{ioService};
        boost::asio::ip::tcp::resolver asyncResolver{ioService};
        boost::asio::ip::tcp::socket asyncSocket{ioService};
        std::optional<boost::asio::ip::tcp::resolver::results_type> deviceEndpoints{std::nullopt};
        std::condition_variable ioServiceTaskCv{};
        std::mutex ioServiceLock{};
        std::atomic_bool interruptIoServiceTask{false};
        Worker worker{1};
    };
} // namespace Device