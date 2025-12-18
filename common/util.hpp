#pragma once
#include <string>
#include <fstream>
#include <random>
#include <chrono>
#include <filesystem>
#include <thread>

inline void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

inline std::string now_str() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[64]; std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", std::localtime(&t));
    return buf;
}

inline std::ofstream open_log(const std::string& user) {
    std::filesystem::create_directories("logs");
    std::string file = "logs/" + now_str() + "_" + user + ".log";
    return std::ofstream(file, std::ios::app);
}

inline std::string maybe_corrupt(const std::string& s, double p = 0.35) {
    if (s.empty()) return s;

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<> dist(0.0, 1.0);

    double r = dist(rng);  // 0.0 - 1.0

    // p = 0.40 -> yaklaþýk %40 ihtimalle boz
    if (r > p) {
        std::cout << "[SERVER] maybe_corrupt: BOZULMADI (r=" << r
            << ", data='" << s << "')\n";
        return s;
    }

    std::string out = s;
    size_t i = static_cast<size_t>(dist(rng) * out.size());
    if (i >= out.size()) i = out.size() - 1;

    char old = out[i];
    out[i] = (old == (char)127) ? (old - 1) : (old + 1);

    std::cout << "[SERVER] maybe_corrupt: BOZULDU   (r=" << r
        << ", index=" << i
        << ", '" << old << "' -> '" << out[i]
        << "', orijinal='" << s
        << "', yeni='" << out << "')\n";

    return out;
}
