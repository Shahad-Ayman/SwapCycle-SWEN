#include "Uuid.h"

#include <sodium.h>

#include <array>
#include <cctype>
#include <cstdint>

namespace swapcycle::common {

std::string generateUuid() {
    std::array<std::uint8_t, 16> bytes{};
    randombytes_buf(bytes.data(), bytes.size());
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40);  // version 4
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80);  // RFC 4122 variant

    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(36);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) out.push_back('-');
        out.push_back(kHex[bytes[i] >> 4]);
        out.push_back(kHex[bytes[i] & 0x0F]);
    }
    return out;
}

bool isValidUuid(std::string_view text) {
    if (text.size() != 36) return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const bool dashPosition = i == 8 || i == 13 || i == 18 || i == 23;
        if (dashPosition) {
            if (text[i] != '-') return false;
        } else if (!std::isxdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

}  // namespace swapcycle::common
