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
        template<typename Socket>
        class SocketGuard {
        public:
            template<typename... Args>
            explicit SocketGuard(Args &&... args) : socket(std::forward<Args>(args)...) {}

            ~SocketGuard() { socket.close(); }

            inline Socket &operator*() { return socket; }

            inline Socket &getUnderlyingSocket() { return (*this).operator*(); }

        private:
            Socket socket;
        };
    } // namespace internals
    using PropertyTree = boost::property_tree::ptree;

    struct DeviceConfiguration {
        [[maybe_unused]] DeviceConfiguration(std::string name, const std::string &deviceAddress,
                                             unsigned short port = 80)
                : name(std::move(name)),
                  deviceAddress(boost::asio::ip::address::from_string(deviceAddress)),
                  httpServicePort(port) {
        }

        std::string name;
        const boost::asio::ip::address deviceAddress{};
        const unsigned short httpServicePort{};
    };

    enum class AsyncRequestResult {
        SUCCESS,
        FAILED,
        TIMEOUT
    };

    inline std::string asyncResultToString(AsyncRequestResult result) {
        switch (result) {
            case AsyncRequestResult::SUCCESS:
                return "success";
            case AsyncRequestResult::FAILED:
                return "failed";
            case AsyncRequestResult::TIMEOUT:
                return "timeout";
        }
        return "unknown";
    }

    struct AsyncResult {
    public:
        AsyncResult(AsyncRequestResult result, const PropertyTree &tree) : requestResult(result), propertyTree(tree) {}

    public:
        inline explicit operator bool() const { return requestResult == AsyncRequestResult::SUCCESS; }

        inline bool operator==(const AsyncRequestResult &rhs) const { return requestResult == rhs; }

        inline bool operator!=(const AsyncRequestResult &rhs) const { return !(rhs == (*this).requestResult); }

        [[nodiscard]] inline const PropertyTree &getPropertyTree() const { return propertyTree; }

        [[nodiscard]] inline AsyncRequestResult getRequestResult() const { return requestResult; }

    private:
        AsyncRequestResult requestResult{};
        PropertyTree propertyTree{};
    };

    class R2000 {
    public:
        using AsyncCommandCallback = std::function<void(const AsyncResult &)>;

    private:
        using HttpResult = std::tuple<int, std::string, std::string>;
        using HttpGetCallback = std::function<void(const HttpResult &)>;

        /**
         * Setup a new HTTP command interface.
         * @param configuration The device address and the chosen port.
         */
        explicit R2000(DeviceConfiguration configuration);

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
        [[nodiscard]] std::pair<bool, Device::PropertyTree> sendHttpCommand(const std::string &command,
                                                                            const std::string &parameter = "",
                                                                            std::string value = "") const noexcept(false);

        /**
         * Send a HTTP Command with several parameters and their corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameters Parameters and values map attached to the command to send.
         * @return A pair containing as first a flag indicating either or not the command has been successfully sent
         * and a reply received. The second of the pair is a property tree representing the content of the Http answer.
         */
        [[nodiscard]] std::pair<bool, Device::PropertyTree> sendHttpCommand(const std::string &command,
                                                                            const Parameters::ParametersMap &parameters) const
        noexcept(false);

        /**
         * Asynchronously send a HTTP Command without any parameters to the device.
         * @param command The command to send to the device.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the requests queue is full.
         */
        bool asyncSendHttpCommand(const std::string &command, AsyncCommandCallback callable,
                                  std::chrono::milliseconds timeout) noexcept(true);

        /**
         * Asynchronously send a HTTP Command with several parameters and their corresponding value to the device.
         * @param command The command to send to the device.
         * @param parameters Parameters attached to the command to send.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the requests queue is full.
         */
        bool asyncSendHttpCommand(const std::string &command, const Parameters::ParametersMap &parameters,
                                  AsyncCommandCallback callable,
                                  std::chrono::milliseconds timeout = 5000ms) noexcept(true);

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
        [[nodiscard]] boost::asio::ip::tcp::resolver::query getDeviceQuery() const;

        /**
         * Send a HTTP GET request to the device. This methods blocks until the socket is connected.
         * @param requestPath The http request path to send.
         * @return A HttpResult object containing the status code, the header and the content of the answer.
         */
        [[nodiscard]] HttpResult HttpGet(const std::string &requestPath) const noexcept(false);

        /**
         * Send a HTTP GET to a remote endpoint through a given socket.
         * @param socket The socket to send the request through and read the answer from.
         * @param requestPath The http request path to send.
         * @return  A HttpResult object containing the status code, the header and the content of the answer.
         */
        [[nodiscard]] static HttpResult HttpGet(boost::asio::ip::tcp::socket &socket, const std::string &requestPath);

        /**
         * Asynchronously send a HTTP GET request to the device with a timeout.
         * @param request The http request to send.
         * @param callable A callable executed when the request has completed, failed or the timeout has been reached.
         * @param timeout The timeout of the request before being cancelled.
         * @return True if the request has been submitted, false if the request queue is full.
         */
        bool AsyncHttpGet(const std::string &request, HttpGetCallback callable,
                          std::chrono::milliseconds timeout) noexcept(true);

        /**
         * Handle the resolution of connection to endpoints and initiate a socket connection to send a http request.
         * @param error Error encountered during the resolution if any.
         * @param request The http request to send if the resolution has succeeded.
         * @param callable The http callback to execute once the http request is sent.
         * @param endpoints The endpoints to try to connect the socket to.
         * @param timeout The timeout of the socket connection before being cancelled.
         */
        void handleEndpointResolution(const boost::system::error_code &error, const std::string &request,
                                      HttpGetCallback callable, boost::asio::ip::tcp::resolver::results_type &endpoints,
                                      std::chrono::milliseconds timeout);

        /**
         *
         */
        void ioServiceTask();

        /**
         * Handle the connection of the socket and send a http request.
         * @param error Error encountered during the socket connection if any.
         * @param request The http request to send if the resolution has succeeded.
         * @param callable The http callback to execute once the http request is sent.
         */
        void handleSocketConnection(const boost::system::error_code &error, const std::string &request,
                                    const HttpGetCallback &callable);

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
        void handleAsyncResolverDeadline(const boost::system::error_code &error);

        /**
         * Handle the asynchronous socket connection deadline.
         * @param error Error encountered while waiting for the socket to connect.
         */
        void handleAsyncSocketDeadline(const boost::system::error_code &error);

        /**
         * Schedule a timeout deadline for the asynchronous resolver.
         * @param timeout Maximum time allowed for the resolver to complete before cancelling the operation.
         */
        void scheduleResolverDeadline(std::chrono::milliseconds timeout);

        /**
         * Schedule a timeout deadline for the asynchronous socket connection.
         * @param timeout Maximum time allowed for the socket to connect before cancelling the operation.
         */
        void scheduleSocketConnectionDeadline(std::chrono::milliseconds timeout);

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
        Worker worker{10};
    };
} // namespace Device