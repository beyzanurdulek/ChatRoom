#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class Algo { PARITY, PARITY2D, CHECKSUM, CRC };

struct EccResult { std::string code; };

EccResult ecc_make(Algo a, const std::string& data);
bool      ecc_verify(Algo a, const std::string& data, const std::string& code);

inline Algo algo_from_string(const std::string& s) {
    if (s == "PARITY")   return Algo::PARITY;
    if (s == "PARITY2D") return Algo::PARITY2D;
    if (s == "CHECKSUM") return Algo::CHECKSUM;
    return Algo::CRC;
}
inline std::string algo_to_string(Algo a) {
    switch (a) {
    case Algo::PARITY:   return "PARITY";
    case Algo::PARITY2D: return "PARITY2D";
    case Algo::CHECKSUM: return "CHECKSUM";
    case Algo::CRC:      return "CRC";
    }
    return "CRC";
}
