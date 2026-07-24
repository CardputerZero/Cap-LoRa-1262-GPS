#include "models/gps_model.hpp"
#include "view_models/gps_view_model.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <string>

namespace {

#define CHECK(condition)                                                                         \
    do {                                                                                         \
        if (!(condition)) {                                                                      \
            std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false;                                                                        \
        }                                                                                        \
    } while (false)

std::string sentence(const std::string& body)
{
    uint8_t checksum = 0;
    for (char character : body) {
        checksum ^= static_cast<uint8_t>(character);
    }
    char suffix[8] = {};
    std::snprintf(suffix, sizeof(suffix), "*%02X\r\n", static_cast<unsigned int>(checksum));
    return "$" + body + suffix;
}

struct FakeState {
    std::deque<std::string> chunks;
    int openCount = 0;
    bool failOpen = false;
    bool failRead = false;
};

class FakeBackend final : public cap_gps::gps::GpsBackend {
public:
    explicit FakeBackend(std::shared_ptr<FakeState> state) : _state(std::move(state))
    {
    }

    bool open(cap_gps::gps::ReceiverInfo& info, std::string& error) override
    {
        ++_state->openCount;
        if (_state->failOpen) {
            error = "scripted open failure";
            return false;
        }
        _open        = true;
        info.backend = "test";
        info.device  = "fake://gps";
        info.baud    = 115200;
        return true;
    }

    cap_gps::gps::ReadResult readAvailable(uint8_t* data, std::size_t capacity) override
    {
        if (_state->failRead) {
            _state->failRead = false;
            return {0, "scripted read failure"};
        }
        if (_state->chunks.empty()) {
            return {};
        }
        std::string& chunk      = _state->chunks.front();
        const std::size_t count = std::min(capacity, chunk.size());
        std::memcpy(data, chunk.data(), count);
        chunk.erase(0, count);
        if (chunk.empty()) {
            _state->chunks.pop_front();
        }
        return {count, {}};
    }

    void close() noexcept override
    {
        _open = false;
    }

    bool isOpen() const noexcept override
    {
        return _open;
    }

private:
    std::shared_ptr<FakeState> _state;
    bool _open = false;
};

bool testStateTransitionsAndRetry()
{
    auto fake = std::make_shared<FakeState>();
    cap_gps::GpsModel model(std::make_unique<FakeBackend>(fake));
    model.start();
    CHECK(model.status().get().state == cap_gps::gps::GpsState::NoData);
    CHECK(model.status().get().ready);

    model.tick(100);
    CHECK(model.status().get().state == cap_gps::gps::GpsState::NoData);

    fake->chunks.push_back(sentence("GNRMC,080023.00,A,2232.5858,N,11403.4719,E,1.25,73.4,230726,,,A") +
                           sentence("GNGGA,080023.00,2232.5858,N,11403.4719,E,1,12,0.82,35.6,M,-2.3,M,,"));
    model.tick(200);
    CHECK(model.status().get().state == cap_gps::gps::GpsState::Fixed);
    CHECK(model.status().get().data.fixValid);

    model.tick(3401);
    CHECK(model.status().get().state == cap_gps::gps::GpsState::Stale);

    fake->failRead = true;
    model.tick(3500);
    CHECK(model.status().get().state == cap_gps::gps::GpsState::Error);
    CHECK(!model.status().get().initializationFailed);

    CHECK(model.retry());
    CHECK(fake->openCount == 2);
    CHECK(model.status().get().state == cap_gps::gps::GpsState::NoData);
    model.stop();
    CHECK(model.status().get().state == cap_gps::gps::GpsState::Stopped);
    return true;
}

bool testInitializationFailure()
{
    auto fake      = std::make_shared<FakeState>();
    fake->failOpen = true;
    cap_gps::GpsModel model(std::make_unique<FakeBackend>(fake));
    model.start();
    CHECK(model.status().get().state == cap_gps::gps::GpsState::Error);
    CHECK(model.status().get().initializationFailed);
    CHECK(model.status().get().error == "scripted open failure");
    return true;
}

bool testViewModelForwardsStatusAndShowsRetry()
{
    auto fake      = std::make_shared<FakeState>();
    fake->failOpen = true;
    cap_gps::GpsModel model(std::make_unique<FakeBackend>(fake));
    cap_gps::GpsViewModel viewModel(model);
    viewModel.onEnter();

    model.start();
    CHECK(viewModel.status().get().state == cap_gps::gps::GpsState::Error);
    CHECK(viewModel.modalActive());
    CHECK(viewModel.errorDialogActive().get());

    fake->failOpen = false;
    viewModel.retryReceiver();
    CHECK(viewModel.status().get().state == cap_gps::gps::GpsState::NoData);
    CHECK(!viewModel.modalActive());
    CHECK(!viewModel.errorDialogActive().get());
    viewModel.onExit();
    return true;
}

}  // namespace

int main()
{
    if (!testStateTransitionsAndRetry() || !testInitializationFailure() ||
        !testViewModelForwardsStatusAndShowsRetry()) {
        return 1;
    }
    std::puts("GPS model tests passed");
    return 0;
}
