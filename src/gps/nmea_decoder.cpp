#include "gps/nmea_decoder.hpp"

#include <minmea.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace cap_gps::gps {
namespace {

constexpr std::size_t kMaximumBufferedSentence = 160;
constexpr double kKnotsToKph                   = 1.852;

double valueOf(const minmea_float& value)
{
    if (value.scale == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return static_cast<double>(value.value) / static_cast<double>(value.scale);
}

double coordinateOf(const minmea_float& value)
{
    const double raw = valueOf(value);
    if (!std::isfinite(raw)) {
        return raw;
    }

    const double degrees = std::trunc(raw / 100.0);
    const double minutes = raw - degrees * 100.0;
    return degrees + minutes / 60.0;
}

void setTime(UtcDateTime& target, const minmea_time& source)
{
    if (source.hours < 0 || source.hours > 23 || source.minutes < 0 || source.minutes > 59 || source.seconds < 0 ||
        source.seconds > 60) {
        return;
    }
    target.timeValid = true;
    target.hour      = source.hours;
    target.minute    = source.minutes;
    target.second    = source.seconds;
}

void setDate(UtcDateTime& target, const minmea_date& source)
{
    if (source.year < 0 || source.month < 1 || source.month > 12 || source.day < 1 || source.day > 31) {
        return;
    }
    target.dateValid = true;
    target.year      = source.year < 100 ? source.year + 2000 : source.year;
    target.month     = source.month;
    target.day       = source.day;
}

bool finiteCoordinate(double latitude, double longitude)
{
    return std::isfinite(latitude) && std::isfinite(longitude) && latitude >= -90.0 && latitude <= 90.0 &&
           longitude >= -180.0 && longitude <= 180.0;
}

}  // namespace

void NmeaDecoder::reset()
{
    _navigation = {};
    _line.clear();
    _discard_line      = false;
    _has_sentence      = false;
    _has_fix_timestamp = false;
    _last_sentence_ms  = 0;
    _last_fix_ms       = 0;
}

bool NmeaDecoder::feed(const uint8_t* data, std::size_t size, uint32_t nowMs)
{
    if (!data || size == 0) {
        return false;
    }

    _navigation.bytesReceived += size;
    bool changed = false;
    for (std::size_t index = 0; index < size; ++index) {
        const char character = static_cast<char>(data[index]);
        if (character == '\r' || character == '\n') {
            if (!_line.empty() || _discard_line) {
                changed = finishLine(nowMs) || changed;
            }
            _line.clear();
            _discard_line = false;
            continue;
        }

        if (character == '$') {
            _line.assign(1, character);
            _discard_line = false;
            continue;
        }
        if (_discard_line || _line.empty()) {
            continue;
        }
        if (_line.size() >= kMaximumBufferedSentence) {
            _line.clear();
            _discard_line = true;
            continue;
        }
        _line.push_back(character);
    }
    return changed;
}

bool NmeaDecoder::finishLine(uint32_t nowMs)
{
    if (_discard_line || _line.empty()) {
        ++_navigation.invalidSentences;
        return true;
    }

    const auto sentenceId = minmea_sentence_id(_line.c_str(), true);
    if (sentenceId == MINMEA_INVALID) {
        ++_navigation.invalidSentences;
        return true;
    }
    if (sentenceId == MINMEA_UNKNOWN) {
        return false;
    }

    ++_navigation.sentencesReceived;
    _has_sentence            = true;
    _last_sentence_ms        = nowMs;
    _navigation.lastSentence = minmea_sentence(sentenceId);
    char talker[3]           = {};
    if (minmea_talker_id(talker, _line.c_str())) {
        _navigation.talker.assign(talker, 2);
    }

    switch (sentenceId) {
        case MINMEA_SENTENCE_RMC: {
            minmea_sentence_rmc frame{};
            if (!minmea_parse_rmc(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            setTime(_navigation.utc, frame.time);
            setDate(_navigation.utc, frame.date);
            bool fixed = false;
            if (frame.valid) {
                const double latitude  = coordinateOf(frame.latitude);
                const double longitude = coordinateOf(frame.longitude);
                if (finiteCoordinate(latitude, longitude)) {
                    _navigation.latitudeDeg   = latitude;
                    _navigation.longitudeDeg  = longitude;
                    _navigation.positionValid = true;
                    fixed                     = true;
                }
            }
            updateFixState(fixed, nowMs);
            const double speed = valueOf(frame.speed) * kKnotsToKph;
            if (std::isfinite(speed) && speed >= 0.0) {
                _navigation.speedKph   = speed;
                _navigation.speedValid = true;
            }
            const double course = valueOf(frame.course);
            if (std::isfinite(course) && course >= 0.0) {
                _navigation.courseDeg   = course;
                _navigation.courseValid = true;
            }
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            minmea_sentence_gga frame{};
            if (!minmea_parse_gga(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            _navigation.fixQuality     = frame.fix_quality;
            _navigation.satellitesUsed = std::max(0, frame.satellites_tracked);
            setTime(_navigation.utc, frame.time);
            bool fixed = false;
            if (frame.fix_quality > 0) {
                const double latitude  = coordinateOf(frame.latitude);
                const double longitude = coordinateOf(frame.longitude);
                if (finiteCoordinate(latitude, longitude)) {
                    _navigation.latitudeDeg   = latitude;
                    _navigation.longitudeDeg  = longitude;
                    _navigation.positionValid = true;
                    fixed                     = true;
                }
                const double altitude = valueOf(frame.altitude);
                if (std::isfinite(altitude)) {
                    _navigation.altitudeM     = altitude;
                    _navigation.altitudeValid = true;
                }
            }
            updateFixState(fixed, nowMs);
            const double hdop = valueOf(frame.hdop);
            if (std::isfinite(hdop) && hdop >= 0.0) {
                _navigation.hdop      = hdop;
                _navigation.hdopValid = true;
            }
            break;
        }
        case MINMEA_SENTENCE_GLL: {
            minmea_sentence_gll frame{};
            if (!minmea_parse_gll(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            setTime(_navigation.utc, frame.time);
            bool fixed = false;
            if (frame.status == MINMEA_GLL_STATUS_DATA_VALID) {
                const double latitude  = coordinateOf(frame.latitude);
                const double longitude = coordinateOf(frame.longitude);
                if (finiteCoordinate(latitude, longitude)) {
                    _navigation.latitudeDeg   = latitude;
                    _navigation.longitudeDeg  = longitude;
                    _navigation.positionValid = true;
                    fixed                     = true;
                }
            }
            updateFixState(fixed, nowMs);
            break;
        }
        case MINMEA_SENTENCE_GSA: {
            minmea_sentence_gsa frame{};
            if (!minmea_parse_gsa(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            _navigation.fixType = frame.fix_type;
            if (frame.fix_type <= 1) {
                _navigation.fixValid = false;
            }
            const double pdop = valueOf(frame.pdop);
            const double hdop = valueOf(frame.hdop);
            const double vdop = valueOf(frame.vdop);
            if (std::isfinite(pdop) && pdop >= 0.0) {
                _navigation.pdop      = pdop;
                _navigation.pdopValid = true;
            }
            if (std::isfinite(hdop) && hdop >= 0.0) {
                _navigation.hdop      = hdop;
                _navigation.hdopValid = true;
            }
            if (std::isfinite(vdop) && vdop >= 0.0) {
                _navigation.vdop      = vdop;
                _navigation.vdopValid = true;
            }
            break;
        }
        case MINMEA_SENTENCE_GSV: {
            minmea_sentence_gsv frame{};
            if (!minmea_parse_gsv(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            _navigation.satellitesVisible = std::max(0, frame.total_sats);
            break;
        }
        case MINMEA_SENTENCE_VTG: {
            minmea_sentence_vtg frame{};
            if (!minmea_parse_vtg(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            const double speed  = valueOf(frame.speed_kph);
            const double course = valueOf(frame.true_track_degrees);
            if (std::isfinite(speed) && speed >= 0.0) {
                _navigation.speedKph   = speed;
                _navigation.speedValid = true;
            }
            if (std::isfinite(course) && course >= 0.0) {
                _navigation.courseDeg   = course;
                _navigation.courseValid = true;
            }
            break;
        }
        case MINMEA_SENTENCE_ZDA: {
            minmea_sentence_zda frame{};
            if (!minmea_parse_zda(&frame, _line.c_str())) {
                ++_navigation.invalidSentences;
                return true;
            }
            setTime(_navigation.utc, frame.time);
            setDate(_navigation.utc, frame.date);
            break;
        }
        case MINMEA_SENTENCE_GBS:
        case MINMEA_SENTENCE_GST:
        case MINMEA_UNKNOWN:
        case MINMEA_INVALID:
            break;
    }

    return true;
}

void NmeaDecoder::updateFixState(bool fixed, uint32_t nowMs)
{
    _navigation.fixValid = fixed;
    if (fixed) {
        _has_fix_timestamp = true;
        _last_fix_ms       = nowMs;
    }
}

}  // namespace cap_gps::gps
