#include "gps/nmea_decoder.hpp"

#include <cmath>
#include <cstdio>
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

void feed(cap_gps::gps::NmeaDecoder& decoder, const std::string& text, uint32_t nowMs, std::size_t split)
{
    for (std::size_t offset = 0; offset < text.size(); offset += split) {
        const std::size_t count = std::min(split, text.size() - offset);
        decoder.feed(reinterpret_cast<const uint8_t*>(text.data() + offset), count, nowMs);
    }
}

bool testFragmentedFix()
{
    cap_gps::gps::NmeaDecoder decoder;
    std::string stream;
    stream += sentence("GNRMC,080023.00,A,2232.5858,N,11403.4719,E,1.25,73.4,230726,,,A");
    stream += sentence("GNGGA,080023.00,2232.5858,N,11403.4719,E,1,12,0.82,35.6,M,-2.3,M,,");
    stream += sentence("GNGSA,A,3,03,07,08,11,16,20,22,26,,,,,1.40,0.82,1.13");
    stream += sentence("GNGSV,1,1,18,03,42,112,37,07,31,231,34,08,66,047,41,11,19,301,28");
    stream += sentence("GNVTG,73.4,T,,M,1.25,N,2.32,K,A");
    feed(decoder, stream, 1200, 7);

    const auto& data = decoder.navigation();
    CHECK(data.fixValid);
    CHECK(data.positionValid);
    CHECK(std::fabs(data.latitudeDeg - 22.5430966667) < 0.000001);
    CHECK(std::fabs(data.longitudeDeg - 114.057865) < 0.000001);
    CHECK(data.satellitesUsed == 12);
    CHECK(data.satellitesVisible == 18);
    CHECK(data.fixType == 3);
    CHECK(data.altitudeValid && std::fabs(data.altitudeM - 35.6) < 0.01);
    CHECK(data.speedValid && std::fabs(data.speedKph - 2.32) < 0.01);
    CHECK(data.courseValid && std::fabs(data.courseDeg - 73.4) < 0.01);
    CHECK(data.hdopValid && std::fabs(data.hdop - 0.82) < 0.01);
    CHECK(data.utc.dateValid && data.utc.year == 2026 && data.utc.month == 7 && data.utc.day == 23);
    CHECK(data.utc.timeValid && data.utc.hour == 8 && data.utc.minute == 0 && data.utc.second == 23);
    CHECK(data.sentencesReceived == 5);
    CHECK(data.invalidSentences == 0);
    CHECK(data.talker == "GN");
    return true;
}

bool testNoFixIsStillValidNmea()
{
    cap_gps::gps::NmeaDecoder decoder;
    const std::string stream =
        sentence("GNRMC,080000.00,V,,,,,,,230726,,,N") + sentence("GNGGA,080000.00,,,,,0,00,99.99,,,,,,");
    feed(decoder, stream, 300, stream.size());

    const auto& data = decoder.navigation();
    CHECK(decoder.hasSentence());
    CHECK(!data.fixValid);
    CHECK(!data.positionValid);
    CHECK(data.sentencesReceived == 2);
    CHECK(data.utc.dateValid);
    CHECK(data.utc.timeValid);
    return true;
}

bool testBadChecksumAndRecovery()
{
    cap_gps::gps::NmeaDecoder decoder;
    const std::string invalid = "$GNRMC,080023.00,A,2232.5858,N,11403.4719,E,1.25,73.4,230726,,,A*00\r\n";
    feed(decoder, invalid, 100, invalid.size());
    CHECK(decoder.navigation().invalidSentences == 1);
    CHECK(!decoder.hasSentence());

    const std::string valid = sentence("GNGGA,080023.00,2232.5858,N,11403.4719,E,1,08,1.20,12.3,M,-2.3,M,,");
    feed(decoder, "$" + std::string(200, 'X') + "\r\n" + valid, 200, 13);
    CHECK(decoder.hasSentence());
    CHECK(decoder.navigation().fixValid);
    CHECK(decoder.navigation().sentencesReceived == 1);
    CHECK(decoder.navigation().invalidSentences >= 2);
    return true;
}

bool testFixFreshness()
{
    cap_gps::gps::NmeaDecoder decoder;
    const std::string fixed = sentence("GNGGA,080023.00,2232.5858,N,11403.4719,E,1,08,1.20,12.3,M,-2.3,M,,");
    feed(decoder, fixed, 100, fixed.size());
    CHECK(decoder.navigation().fixValid);
    CHECK(decoder.lastFixMs() == 100);

    const std::string metadata = sentence("GNGSV,1,1,04,03,42,112,24,07,31,231,18,08,66,047,27,11,19,301,15");
    feed(decoder, metadata, 5000, metadata.size());
    CHECK(decoder.navigation().fixValid);
    CHECK(decoder.lastFixMs() == 100);

    const std::string noFix = sentence("GNRMC,080028.00,V,,,,,,,230726,,,N");
    feed(decoder, noFix, 5100, noFix.size());
    CHECK(!decoder.navigation().fixValid);
    CHECK(decoder.lastFixMs() == 100);

    const std::string missingPosition = sentence("GNGGA,080029.00,,,,,1,08,1.20,12.3,M,-2.3,M,,");
    feed(decoder, missingPosition, 5200, missingPosition.size());
    CHECK(!decoder.navigation().fixValid);
    CHECK(decoder.lastFixMs() == 100);
    return true;
}

}  // namespace

int main()
{
    if (!testFragmentedFix() || !testNoFixIsStillValidNmea() || !testBadChecksumAndRecovery() || !testFixFreshness()) {
        return 1;
    }
    std::puts("GPS NMEA decoder tests passed");
    return 0;
}
