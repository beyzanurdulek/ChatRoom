#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <sstream>
#include <cstring>
#include <cstdlib>   // rand, srand
#include <ctime>     // time

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

static std::mutex g_mx;
static std::map<std::string, SOCKET> g_clients; // name -> sock

struct Msg {
    std::string payload;
    std::string ecc;
    Algo        algo;
};

static std::map<std::string, std::map<std::string, Msg>> g_lastClean;

// Tek satýr göndermek için
void send_line(SOCKET s, const std::string& line) {
    std::string out = line + "\n";
    send(s, out.c_str(), (int)out.size(), 0);
}

bool recv_line(SOCKET s, std::string& line) {
    line.clear();
    char c;
    while (true) {
        int r = recv(s, &c, 1, 0);
        if (r <= 0) return false;
        if (c == '\n') return true;
        line.push_back(c);
    }
}

// %75 olasýlýkla payload'ý bozan fonksiyon
inline std::string maybe_corrupt_75(const std::string& original) {
    if (original.empty()) return original;

    std::string s = original;   // kopya üzerinde çalýþ
    int r = rand() % 100;       // 0..99

    if (r < 75) { // %75 boz
        size_t i = (size_t)(rand() % s.size());
        char old = s[i];

        if (old == (char)127)
            s[i] = old - 1;
        else
            s[i] = old + 1;

        std::cout << "[SERVER] MESAJ BOZULDU (" << r << "%): '"
            << original << "' -> '" << s
            << "' (index " << i << ", '" << old
            << "' -> '" << s[i] << "')\n";
    }
    else {
        std::cout << "[SERVER] MESAJ BOZULMADI (" << r << "%): '"
            << original << "'\n";
    }

    return s;
}

void client_thread(SOCKET csock, std::string remote_ip) {
    std::string myname = "";
    std::cout << "[*] Thread started for " << remote_ip << "\n";
    std::string line;

    while (recv_line(csock, line)) {
        auto p = split(line);
        if (p.empty()) continue;
        std::string cmd = p[0];

        if (cmd == "CONN" && p.size() >= 3) {
            Algo a = algo_from_string(p[1]);
            myname = p[2];
            {
                std::lock_guard<std::mutex> lk(g_mx);
                g_clients[myname] = csock;
            }
            std::cout << "[+] CONN " << myname << " (" << remote_ip << ")\n";
            {
                std::lock_guard<std::mutex> lk(g_mx);
                std::cout << "    Active: ";
                for (auto& kv : g_clients) std::cout << kv.first << " ";
                std::cout << "\n";
            }
        }
        else if (cmd == "MESG" && p.size() >= 6) {
            Algo a = algo_from_string(p[1]);
            std::string src = p[2];
            std::string dst = p[3];
            std::string payload = p[4];
            std::string ecc = p[5];

            {
                std::lock_guard<std::mutex> lk(g_mx);
                g_lastClean[src][dst] = Msg{ payload, ecc, a };
            }

            // %75 ihtimalle boz ve logla
            std::string maybe_bad = maybe_corrupt_75(payload);

            SOCKET dSock = INVALID_SOCKET;
            {
                std::lock_guard<std::mutex> lk(g_mx);
                if (g_clients.count(dst))
                    dSock = g_clients[dst];
            }

            if (dSock != INVALID_SOCKET) {
                std::string out = join(
                    { "MESG", algo_to_string(a), src, dst, maybe_bad, ecc }
                );
                send_line(dSock, out);
            }
        }
        else if (cmd == "MERR" && p.size() >= 3) {
            std::string src = p[1], dst = p[2];
            SOCKET dSock = INVALID_SOCKET;
            {
                std::lock_guard<std::mutex> lk(g_mx);
                if (g_clients.count(dst))
                    dSock = g_clients[dst];
            }
            if (dSock != INVALID_SOCKET) {
                Msg m;
                {
                    std::lock_guard<std::mutex> lk(g_mx);
                    m = g_lastClean[src][dst];
                }
                std::string out = join(
                    { "MESG", algo_to_string(m.algo), src, dst, m.payload, m.ecc }
                );
                send_line(dSock, out);
            }
        }
        else if (cmd == "GONE" && p.size() >= 2) {
            std::string who = p[1];
            std::lock_guard<std::mutex> lk(g_mx);
            if (g_clients.count(who)) {
                std::cout << "[-] GONE " << who << "\n";
                g_clients.erase(who);
                break;
            }
        }
    }

    closesocket(csock);
    if (!myname.empty()) {
        std::lock_guard<std::mutex> lk(g_mx);
        g_clients.erase(myname);
    }
    std::cout << "[*] Thread ended for " << remote_ip << "\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    // Rastgelelik için seed
    std::srand((unsigned)std::time(nullptr));

    int port = (argc >= 2) ? std::stoi(argv[1]) : 5555;

    SOCKET lsock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int yes = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

    bind(lsock, (sockaddr*)&addr, sizeof(addr));
    listen(lsock, 16);

    std::cout << "[*] Server listening on port " << port << "\n";

    while (true) {
        sockaddr_in cli{};
        socklen_t sl = sizeof(cli);
        SOCKET csock = accept(lsock, (sockaddr*)&cli, &sl);
        if (csock == INVALID_SOCKET) continue;

        std::string ip = inet_ntoa(cli.sin_addr);
        std::thread(client_thread, csock, ip).detach();
    }

#ifdef _WIN32
    WSACleanup();
#endif
}
