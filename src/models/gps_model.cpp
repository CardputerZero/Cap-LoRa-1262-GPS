#include "models/gps_model.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace cap_gps {
namespace {

constexpr std::size_t kReadBufferSize      = 512;
constexpr std::size_t kMaximumReadsPerTick = 8;
constexpr uint32_t kPublishIntervalMs      = 200;
constexpr uint32_t kStaleAfterMs           = 3000;

uint32_t ageSince(uint32_t nowMs, uint32_t timestampMs)
{
    return nowMs - timestampMs;
}

}  // namespace

GpsModel::GpsModel() : GpsModel(gps::makeGpsBackend())
{
}

GpsModel::GpsModel(std::unique_ptr<gps::GpsBackend> backend) : _backend(std::move(backend))
{
}

GpsModel::~GpsModel()
{
    stop();
}

void GpsModel::start()
{
    if (_started) {
        return;
    }
    _started = true;
    (void)openBackend();
}

void GpsModel::stop()
{
    if (!_started) {
        return;
    }
    if (_backend) {
        _backend->close();
    }
    auto stopped        = _status.get();
    stopped.state       = gps::GpsState::Stopped;
    stopped.ready       = false;
    stopped.diagnostics = "GPS receiver stopped";
    _status.set(std::move(stopped));
    _started = false;
    spdlog::info("GPS model: stopped");
}

void GpsModel::tick(uint32_t nowMs)
{
    if (!_started) {
        return;
    }
    if (!_clock_initialized) {
        _clock_initialized = true;
        _opened_at_ms      = nowMs;
        _last_publish_ms   = nowMs;
    }

    bool changed = false;
    if (_backend && _backend->isOpen()) {
        std::array<uint8_t, kReadBufferSize> buffer{};
        for (std::size_t readIndex = 0; readIndex < kMaximumReadsPerTick; ++readIndex) {
            auto result = _backend->readAvailable(buffer.data(), buffer.size());
            if (!result.error.empty()) {
                setError("UART read failed: " + result.error, false);
                _backend->close();
                return;
            }
            if (result.bytes == 0) {
                break;
            }
            changed = _decoder.feed(buffer.data(), result.bytes, nowMs) || changed;
            if (result.bytes < buffer.size()) {
                break;
            }
        }
    }

    const bool firstNavigationUpdate = changed && _status.get().state == gps::GpsState::NoData;
    if (firstNavigationUpdate || nowMs - _last_publish_ms >= kPublishIntervalMs) {
        updateStatus(nowMs);
    }
}

bool GpsModel::retry()
{
    if (!_started) {
        return false;
    }
    if (_backend) {
        _backend->close();
    }
    return openBackend();
}

bool GpsModel::openBackend()
{
    ++_attempt;
    _decoder.reset();
    _clock_initialized = false;

    gps::GpsStatus starting;
    starting.state       = gps::GpsState::Initializing;
    starting.attempt     = _attempt;
    starting.diagnostics = "Opening GPS receiver";
    _status.set(starting);
    spdlog::info("GPS model: initialization attempt {}", _attempt);

    if (!_backend) {
        setError("This build has no GPS backend", true);
        return false;
    }

    gps::ReceiverInfo info;
    std::string error;
    if (!_backend->open(info, error)) {
        setError(error.empty() ? "GPS backend initialization failed" : std::move(error), true);
        return false;
    }

    gps::GpsStatus ready;
    ready.state       = gps::GpsState::NoData;
    ready.ready       = true;
    ready.attempt     = _attempt;
    ready.info        = std::move(info);
    ready.diagnostics = "Waiting for NMEA data";
    _status.set(std::move(ready));
    spdlog::info("GPS model: backend ready (device={}, baud={}, backend={})", _status.get().info.device,
                 _status.get().info.baud, _status.get().info.backend);
    return true;
}

void GpsModel::updateStatus(uint32_t nowMs)
{
    auto next = _status.get();
    if (!next.ready || next.state == gps::GpsState::Error || next.state == gps::GpsState::Stopped) {
        return;
    }

    next.data = _decoder.navigation();
    if (_decoder.hasSentence()) {
        next.dataAgeMs = ageSince(nowMs, _decoder.lastSentenceMs());
    } else if (_clock_initialized) {
        next.dataAgeMs = ageSince(nowMs, _opened_at_ms);
    } else {
        next.dataAgeMs = std::numeric_limits<uint32_t>::max();
    }
    next.fixAgeMs =
        _decoder.hasFixTimestamp() ? ageSince(nowMs, _decoder.lastFixMs()) : std::numeric_limits<uint32_t>::max();

    const auto previousState = next.state;
    if (!_decoder.hasSentence()) {
        next.state       = gps::GpsState::NoData;
        next.diagnostics = next.data.bytesReceived == 0 ? "No GPS data received" : "Waiting for valid NMEA data";
    } else if (next.dataAgeMs > kStaleAfterMs) {
        next.state       = gps::GpsState::Stale;
        next.diagnostics = "GPS data is stale";
    } else if (next.data.fixValid && next.fixAgeMs <= kStaleAfterMs) {
        next.state       = gps::GpsState::Fixed;
        next.diagnostics = next.data.fixType >= 3 ? "3D position fix" : "Position fix";
    } else {
        next.state       = gps::GpsState::Searching;
        next.diagnostics = "NMEA active; waiting for position fix";
    }

    next.error.clear();
    next.initializationFailed = false;
    _status.set(std::move(next));
    _last_publish_ms = nowMs;
    if (previousState != _status.get().state) {
        spdlog::info("GPS model: state -> {} (sentences={}, invalid={}, satellites={})",
                     gps::gpsStateName(_status.get().state), _status.get().data.sentencesReceived,
                     _status.get().data.invalidSentences, _status.get().data.satellitesUsed);
    }
}

void GpsModel::setError(std::string message, bool initializationFailed)
{
    auto failed                 = _status.get();
    failed.state                = gps::GpsState::Error;
    failed.ready                = false;
    failed.initializationFailed = initializationFailed;
    failed.attempt              = _attempt;
    failed.error                = std::move(message);
    failed.diagnostics          = failed.error;
    _status.set(std::move(failed));
    spdlog::error("GPS model: {}", _status.get().error);
}

}  // namespace cap_gps
