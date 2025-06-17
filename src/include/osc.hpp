
#ifndef OSC_HPP
#define OSC_HPP

#include <iostream>
#include <array>
#include <functional>
#include "asio.hpp"
#include <mutex>
#include <optional>

#include "../libs/oscpack/osc/OscReceivedElements.h"
#include "../libs/oscpack/osc/OscPacketListener.h"
#include "../libs/oscpack/ip/UdpSocket.h"

#define OSC_PORT 7000
const int MAX_OSC_BUFFER_SIZE_ASIO = 4096;

class OSCListener : public osc::OscPacketListener {
public:
    OSCListener();
    virtual ~OSCListener();
    
    std::optional<unsigned int> getPendingTraceThickness();
    std::optional<unsigned int> getPendingPersistenceSamples();
    std::optional<unsigned int> getPendingPersistenceStrength();
    std::optional<uint32_t> getPendingTraceColor();
    std::optional<float> getPendingBlurSpread();
    std::optional<unsigned int> getPendingAlphaScale();
    std::optional<float> getPendingScale();
    int getIndex();

protected:
    virtual void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) override;

private:
    std::mutex param_mutex_;
    // Store pending updates. std::optional indicates if a new value has been received.
    // If an optional contains a value, it means an OSC message for that parameter came in.
    std::optional<unsigned int> trace_thickness_update_;
    std::optional<unsigned int> persistence_samples_update_;
    std::optional<unsigned int> persistence_strength_update_;
    std::optional<uint32_t> trace_color_update_;
    std::optional<float> blur_spread_update_;
    std::optional<unsigned int> alpha_scale_update_;
    std::optional<float> scale_update_;

    int rcv_index = 0;
};

class AsioOscReceiver {
public:
    AsioOscReceiver(asio::io_context& io_context, OSCListener& listener);
    ~AsioOscReceiver();
    void stop();
private:
    void startReceive();    
    void handleReceive(const asio::error_code& error, std::size_t bytes_recvd);
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remote_endpoint_asio_;
    std::array<char, MAX_OSC_BUFFER_SIZE_ASIO> recv_buffer_;
    OSCListener& listener_;
    bool stopped_ = false;
};

#endif