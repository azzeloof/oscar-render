#include "include/osc.hpp"
#include <cstring>

OSCListener::OSCListener() = default;
OSCListener::~OSCListener() = default;

void OSCListener::ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) {
    (void) remoteEndpoint; // Mark as unused if not needed
    try {
        // For debugging, you can keep this:
        // std::cout << "OSC Message Received: " << m.AddressPattern() << std::endl;
        
        osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

        // --- Lock the mutex before accessing or modifying shared data ---
        std::lock_guard<std::mutex> lock(param_mutex_);

        if (std::strncmp(m.AddressPattern(), "/scope/", 7) == 0) {
            // This is a simplified approach and assumes a single digit index.
            int scope_index = m.AddressPattern()[7] - '0'; // Convert char to int
            rcv_index = scope_index;

            // Basic validation for the scope index
            if (scope_index >= 0 && scope_index < 4) { // Assuming 4 scopes (0-3)
                // Now check the rest of the address pattern for parameters
                const char* param_pattern = m.AddressPattern() + 8; // Skip "/scope/X"
                std::cout << "OSC: Scope Index: " << scope_index << std::endl;
                std::cout << "OSC: Param Pattern: " << param_pattern << std::endl;

                // --- Parse specific OSC messages ---
                if (std::strcmp(param_pattern, "/trace/thickness") == 0) {
                    float val;
                    args >> val >> osc::EndMessage; // Ensure all arguments are consumed
                    if (val >= 1.f) { 
                        trace_thickness_update_ = val;
                        std::cout << "  OSC: Trace thickness update queued: " << *trace_thickness_update_ << std::endl;
                    } else {
                        std::cerr << "  OSC: Invalid trace thickness received: " << val << std::endl;
                    }
                } else if (std::strcmp(param_pattern, "/persistence/samples") == 0) {
                    osc::int32 val;
                    args >> val >> osc::EndMessage;
                    if (val > 0) {
                        persistence_samples_update_ = static_cast<unsigned int>(val);
                        std::cout << "  OSC: Persistence Samples update queued: " << *persistence_samples_update_ << std::endl;
                    } else {
                        std::cerr << "  OSC: Invalid persistence samples received: " << val << std::endl;
                    }
                } else if (std::strcmp(param_pattern, "/persistence/strength") == 0) {
                    osc::int32 val; // Assuming strength is sent as int 0-255
                    args >> val >> osc::EndMessage;
                    if (val >= 0 && val <= 255) {
                        persistence_strength_update_ = static_cast<unsigned int>(val);
                        std::cout << "  OSC: Persistence Strength update queued: " << *persistence_strength_update_ << std::endl;
                    } else {
                        std::cerr << "  OSC: Invalid persistence strength (0-255) received: " << val << std::endl;
                    }
                } else if (std::strcmp(param_pattern, "/trace/color") == 0) {
                    osc::int32 val;
                    args >> val >> osc::EndMessage;
                    //if (val >= 0) {
                        trace_color_update_ = static_cast<unsigned int>(val);
                        std::cout << "  OSC: Trace Color update queued: " << *trace_color_update_ << std::endl;
                    //} else {
                    //    std::cerr << "  OSC: Trace Color strength received: " << val << std::endl;
                    //}
                } else if (std::strcmp(param_pattern, "/trace/blur") == 0) {
                    float val; // OSC 'f' type tag
                    args >> val >> osc::EndMessage;
                    // Add validation for blur spread
                    if (val >= 0.0f) { 
                        blur_spread_update_ = val;
                        std::cout << "  OSC: Blur Spread update queued: " << *blur_spread_update_ << std::endl;
                    } else {
                        std::cerr << "  OSC: Invalid blur spread received: " << val << std::endl;
                    }
                } else if (std::strcmp(param_pattern, "/alpha_scale") == 0) {
                    osc::int32 val;
                    args >> val >> osc::EndMessage;
                    if (val >= 0) {
                        alpha_scale_update_ = static_cast<unsigned int>(val);
                        std::cout << "  OSC: Alpha Scale update queued: " << *alpha_scale_update_ << std::endl;
                    } else {
                        std::cerr << "  OSC: Invalid alpha scale received: " << val << std::endl;
                    }
                } else if (std::strcmp(param_pattern, "/scale") == 0) {
                    float val; // OSC 'f' type tag
                    args >> val >> osc::EndMessage;
                    // Add validation for scale
                    if (val >= 0.0f && val <= 1.0f) { 
                        scale_update_ = val;
                        std::cout << "  OSC: Scale update queued: " << *scale_update_ << std::endl;
                    } else {
                        std::cerr << "  OSC: Scale received: " << val << std::endl;
                    }
                }
            } else {
                std::cerr << "  OSC: Invalid scope index received: " << scope_index << std::endl;
            }
        }

    } catch(const osc::Exception& e) {
        std::cerr << "Error while parsing OSC message in OSCListener::ProcessMessage: "
                  << m.AddressPattern() << ": " << e.what() << std::endl;
    }
}

// --- Implementations for the getter methods ---
std::optional<unsigned int> OSCListener::getPendingTraceThickness() {
    std::lock_guard<std::mutex> lock(param_mutex_); // Lock before accessing
    std::optional<unsigned int> val = trace_thickness_update_;
    trace_thickness_update_.reset(); // Clear the value after it's been retrieved (consumed)
    return val;
}

std::optional<unsigned int> OSCListener::getPendingPersistenceSamples() {
    std::lock_guard<std::mutex> lock(param_mutex_);
    std::optional<unsigned int> val = persistence_samples_update_;
    persistence_samples_update_.reset();
    return val;
}

std::optional<unsigned int> OSCListener::getPendingPersistenceStrength() {
    std::lock_guard<std::mutex> lock(param_mutex_);
    std::optional<unsigned int> val = persistence_strength_update_;
    persistence_strength_update_.reset();
    return val;
}

std::optional<uint32_t> OSCListener::getPendingTraceColor() {
    std::lock_guard<std::mutex> lock(param_mutex_);
    std::optional<uint32_t> val = trace_color_update_;
    trace_color_update_.reset();
    return val;
}

std::optional<float> OSCListener::getPendingBlurSpread() {
    std::lock_guard<std::mutex> lock(param_mutex_);
    std::optional<float> val = blur_spread_update_;
    blur_spread_update_.reset();
    return val;
}

std::optional<unsigned int> OSCListener::getPendingAlphaScale() {
    std::lock_guard<std::mutex> lock(param_mutex_);
    std::optional<unsigned int> val = alpha_scale_update_;
    alpha_scale_update_.reset();
    return val;
}


std::optional<float> OSCListener::getPendingScale() {
    std::lock_guard<std::mutex> lock(param_mutex_);
    std::optional<float> val = scale_update_;
    scale_update_.reset();
    return val;
}

int::OSCListener::getIndex() const {
    return rcv_index;
}

AsioOscReceiver::AsioOscReceiver(asio::io_context& io_context, OSCListener& listener)
    : socket_(io_context), listener_(listener) {
    asio::ip::udp::endpoint listen_endpoint(asio::ip::udp::v4(), OSC_PORT);
    asio::error_code ec;
    socket_.open(listen_endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Failed to open OSC socket: " << ec.message() << std::endl;
        throw std::runtime_error("Failed to open OSC socket: " + ec.message());
    }

    socket_.set_option(asio::ip::udp::socket::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Failed to set reuse_address on OSC socket: " << ec.message() << std::endl;
        // Continue if non-fatal, or throw
    }

    socket_.bind(listen_endpoint, ec);
    if (ec) {
        std::cerr << "Failed to bind OSC socket to port " << OSC_PORT << ": " << ec.message() << std::endl;
        throw std::runtime_error("Failed to bind OSC socket: " + ec.message());
    }
    
    std::cout << "OSC Receiver listening on port " << OSC_PORT << std::endl;
    startReceive();
}

AsioOscReceiver::~AsioOscReceiver() {
    stop();
}

void AsioOscReceiver::stop() {
    if (!stopped_) {
        stopped_ = true;
        asio::post(socket_.get_executor(), [this]() {
            if (socket_.is_open()) {
                asio::error_code ec;
                socket_.close(ec);
                if (ec) {
                    std::cerr << "Failed to close OSC socket: " << ec.message() << std::endl;
                }
            }
        });
    }
}

void AsioOscReceiver::startReceive() {
    if (stopped_ || !socket_.is_open()) {
        return;
    }

    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_asio_,
        // Lambda function as the completion handler
        [this](const asio::error_code& error, std::size_t bytes_recvd) {
            handleReceive(error, bytes_recvd);
        });
}

void AsioOscReceiver::handleReceive(const asio::error_code& error, std::size_t bytes_recvd) {
    // If stop() has been called, or an operation was aborted (socket closed), do nothing further.
    if (stopped_ || error == asio::error::operation_aborted) {
        return;
    }

    if (!error && bytes_recvd > 0) {
        // Convert Asio endpoint to oscpack IpEndpointName
        IpEndpointName oscpack_remote_endpoint(
            remote_endpoint_asio_.address().to_v4().to_uint(), // Get IPv4 address as unsigned long
            remote_endpoint_asio_.port()                         // Get port
        );

        try {
            // Pass the raw data to OSCListener (which derives from osc::OscPacketListener)
            listener_.ProcessPacket(recv_buffer_.data(), static_cast<int>(bytes_recvd), oscpack_remote_endpoint);
        } catch (const osc::Exception& e) {
            std::cerr << "oscpack parsing error in ProcessPacket: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Standard exception during ProcessPacket: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception during ProcessPacket." << std::endl;
        }

    } else if (error) {
        // Log other errors (excluding operation_aborted which is expected on stop)
        std::cerr << "OSC Receive error: " << error.message() << std::endl;
    }

    // Continue listening for the next packet if the socket is still open and not stopped
    if (socket_.is_open() && !stopped_) {
        startReceive();
    }
}

