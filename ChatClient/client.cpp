#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <thread>
#include <atomic>
#include <fstream>

#include "protocol.hpp"
#include "error_check.hpp"
#include "util.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using socklen_t = int;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET (-1)
#define SOCKET int
#define closesocket close


#endif

static std::atomic<bool> running{ true };

void send_line(SOCKET s, const std::string& line) {
    std::string out = line + "\n";
    send(s, out.c_str(), (int)out.size(), 0);
}
bool recv_line(SOCKET s, std::string& line) {
    line.clear(); char c;
    while (true) {
        int r = recv(s, &c, 1, 0);
        if (r <= 0) return false;
        if (c == '\n') return true;
        line.push_back(c);
    }
}

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    std::string server_ip = (argc >= 2) ? argv[1] : "127.0.0.1";
    int port = (argc >= 3) ? std::stoi(argv[2]) : 5555;

    std::string myname;
    std::cout << "Kullanici adin: "; std::getline(std::cin, myname);

    std::string algoStr;
    std::cout << "Algo [PARITY|PARITY2D|CHECKSUM|CRC]: ";
    std::getline(std::cin, algoStr);
    if (algoStr.empty()) algoStr = "CRC";
    Algo algo = algo_from_string(algoStr);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "Baglanamadi\n"; return 1;
    }

    auto log = open_log(myname);

    // CONN
    send_line(sock, join({ "CONN", algoStr, myname }));
    log << "[OUT] CONN|" << algoStr << "|" << myname << "\n";

    // Receive thread
    std::thread rx([&]() {
        std::string line;
        while (recv_line(sock, line)) {
            auto p = split(line);
            if (p.empty()) continue;
            std::string cmd = p[0];
            if (cmd == "MESG" && p.size() >= 6) {
                Algo a = algo_from_string(p[1]);
                std::string src = p[2], dst = p[3], payload = p[4], ecc = p[5];

                bool ok = ecc_verify(a, payload, ecc);
                if (!ok) {
                    std::cout << "[!] HATA tespit edildi. MERR gonderiliyor. (src=" << src << ")\n";
                    send_line(sock, join({ "MERR", src, myname }));
                    log << "[OUT] MERR|" << src << "|" << myname << "\n";
                }
                else {
                    std::cout << "[" << src << " -> " << dst << "] " << payload << "\n";
                    log << "[IN ] MESG OK from " << src << ": " << payload << "\n";
                }
            }
        }
        running = false;
        });

    std::cout << "Mesaj yollamak icin: kime yazacaksan 'isim -> mesaj' formatinda gir.\n";
    std::cout << "Cikmak icin: /quit\n";

    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (line == "/quit") {
            send_line(sock, join({ "GONE", myname }));
            log << "[OUT] GONE|" << myname << "\n";
            break;
        }
        // parse: "dst -> payload"
        auto pos = line.find("->");
        if (pos == std::string::npos) { std::cout << "Format: hedef -> mesaj\n"; continue; }
        std::string dst = line.substr(0, pos);
        std::string payload = line.substr(pos + 2);
        // trim
        auto trim = [](std::string& s) {
            size_t b = s.find_first_not_of(" \t"); size_t e = s.find_last_not_of(" \t");
            s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
            };
        trim(dst); trim(payload);
        if (dst.empty() || payload.empty()) { std::cout << "Bos olmaz.\n"; continue; }

        auto ecc = ecc_make(algo, payload);
        std::string out = join({ "MESG", algoStr, myname, dst, payload, ecc.code });
        send_line(sock, out);
        log << "[OUT] " << out << "\n";
    }

    closesocket(sock);
    running = false;
    if (rx.joinable()) rx.join();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
