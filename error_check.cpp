#include "error_check.hpp"
#include <sstream>
#include <bitset>

// ---- Basit Parite (even) ----
static std::string parity_make(const std::string& s) {
    size_t ones = 0;
    for (unsigned char c : s) ones += std::bitset<8>(c).count();
    return (ones % 2 == 0) ? "0" : "1"; // 0 eklersek toplam çift kalır
}
static bool parity_verify(const std::string& s, const std::string& code) {
    size_t ones = 0;
    for (unsigned char c : s) ones += std::bitset<8>(c).count();
    int p = (code == "1") ? 1 : 0;
    return ((ones + p) % 2) == 0;
}

// ---- 2D Parite (8 sütun) ----
static std::string parity2d_make(const std::string& s, int cols = 8) {
    // ASCII bit akışını cols sütuna dizer, satır pariteleri + sütun pariteleri üretir.
    std::vector<int> colp(cols, 0);
    std::ostringstream row;
    int bitcount = 0, rsum = 0;
    for (unsigned char c : s) {
        for (int b = 7;b >= 0;--b) {
            int bit = (c >> b) & 1;
            rsum ^= bit;
            colp[bitcount % cols] ^= bit;
            bitcount++;
            if (bitcount % cols == 0) { row << rsum; rsum = 0; }
        }
    }
    if (bitcount % cols != 0) row << rsum; // son satır
    // sütun pariteleri
    for (int k = 0;k < cols;++k) row << colp[k];
    return row.str();
}
static bool parity2d_verify(const std::string& s, const std::string& code, int cols = 8) {
    // üret ve karşılaştır
    return code == parity2d_make(s, cols);
}

// ---- Checksum 16-bit one's complement ----
static uint16_t checksum16(const std::string& s) {
    uint32_t sum = 0;
    for (size_t i = 0;i < s.size(); i += 2) {
        uint16_t w = (uint8_t)s[i] << 8;
        if (i + 1 < s.size()) w |= (uint8_t)s[i + 1];
        sum += w;
        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~((uint16_t)sum);
}
static std::string checksum_make(const std::string& s) {
    uint16_t c = checksum16(s);
    std::ostringstream os; os << std::hex << std::uppercase;
    os.width(4); os.fill('0'); os << c;
    return os.str();
}
static bool checksum_verify(const std::string& s, const std::string& code) {
    uint16_t c = checksum16(s);
    std::ostringstream os; os << std::hex << std::uppercase;
    os.width(4); os.fill('0'); os << c;
    return os.str() == code;
}

// ---- CRC16-CCITT (0x1021, init 0xFFFF) ----
static uint16_t crc16(const std::string& s) {
    uint16_t crc = 0xFFFF;
    for (unsigned char c : s) {
        crc ^= (uint16_t)c << 8;
        for (int i = 0;i < 8;i++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}
static std::string crc_make(const std::string& s) {
    uint16_t c = crc16(s);
    std::ostringstream os; os << std::hex << std::uppercase;
    os.width(4); os.fill('0'); os << c;
    return os.str();
}
static bool crc_verify(const std::string& s, const std::string& code) {
    uint16_t c = crc16(s);
    std::ostringstream os; os << std::hex << std::uppercase;
    os.width(4); os.fill('0'); os << c;
    return os.str() == code;
}

EccResult ecc_make(Algo a, const std::string& data) {
    switch (a) {
    case Algo::PARITY:   return { parity_make(data) };
    case Algo::PARITY2D: return { parity2d_make(data) };
    case Algo::CHECKSUM: return { checksum_make(data) };
    case Algo::CRC:      return { crc_make(data) };
    }
    return { crc_make(data) };
}
bool ecc_verify(Algo a, const std::string& data, const std::string& code) {
    switch (a) {
    case Algo::PARITY:   return parity_verify(data, code);
    case Algo::PARITY2D: return parity2d_verify(data, code);
    case Algo::CHECKSUM: return checksum_verify(data, code);
    case Algo::CRC:      return crc_verify(data, code);
    }
    return false;
}
