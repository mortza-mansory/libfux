#include "libfux.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <algorithm> // For std::min
#include <string_view>

// ** FIX 1: Define NOMINMAX before including Windows headers to prevent macro conflicts **
#define NOMINMAX

// Winsock Headers
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Mstcpip.h> 

#pragma comment(lib, "ws2_32.lib")

// --- Data Structures ---
struct CapturedPacket {
    std::string summary;
    std::vector<unsigned char> rawData;
};

typedef struct ip_hdr {
    unsigned char ip_header_len : 4, ip_version : 4;
    unsigned char ip_tos;
    unsigned short ip_total_length;
    unsigned short ip_id, ip_frag_offset;
    unsigned char ip_ttl, ip_protocol;
    unsigned short ip_checksum;
    unsigned int ip_srcaddr, ip_destaddr;
} IP_HDR;

typedef struct tcp_hdr {
    unsigned short source_port, dest_port;
    unsigned int sequence, acknowledge;
    unsigned char ns : 1, reserved_part1 : 3, data_offset : 4;
    unsigned char fin : 1, syn : 1, rst : 1, psh : 1, ack : 1, urg : 1;
    unsigned short window, checksum, urgent_pointer;
} TCP_HDR;

typedef struct udp_hdr {
    unsigned short src_port, dest_port, length, checksum;
} UDP_HDR;


// --- Global State & Threading ---
ui::State<std::vector<CapturedPacket>> capturedPackets(std::vector<CapturedPacket>{});
ui::State<bool> isCapturing(false);
ui::State<std::string> selectedIpAddress("");

std::vector<CapturedPacket> sharedPacketBuffer;
std::string sharedErrorMessage;
std::mutex packetMutex;
std::thread captureThread;

// --- Helper & Parsing Functions ---

std::string generateHexDump(const unsigned char* data, int size) {
    if (size == 0) return "No payload data.";
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    // ** FIX 2: Use parenthesis around std::min to avoid macro expansion if NOMINMAX is not defined **
    // Although NOMINMAX is the better solution.
    int max_bytes_to_dump = (std::min)(size, 512);
    if (size > 512) {
        ss << "--- First 512 bytes of Payload ---\n";
    }

    for (int i = 0; i < max_bytes_to_dump; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(data[i]) << " ";
        if ((i + 1) % 16 == 0 && (i + 1) < max_bytes_to_dump) {
            ss << "\n";
        }
    }
    if (size > 512) {
        ss << "\n--- (Payload truncated) ---";
    }
    return ss.str();
}

std::string parseApplicationLayer(const unsigned char* payload, int payload_size) {
    if (payload_size <= 0) return "No application data.";

    std::string_view payload_sv(reinterpret_cast<const char*>(payload), payload_size);
    std::stringstream app_details;

    bool is_http = false;
    if (payload_sv.rfind("GET ", 0) == 0 || payload_sv.rfind("POST ", 0) == 0 ||
        payload_sv.rfind("PUT ", 0) == 0 || payload_sv.rfind("DELETE ", 0) == 0 ||
        payload_sv.rfind("HTTP/", 0) == 0) {
        is_http = true;
    }

    if (is_http) {
        app_details << "Detected Protocol: HTTP (Unencrypted)\n\n";
        size_t header_end = payload_sv.find("\r\n\r\n");
        if (header_end != std::string_view::npos) {
            std::string_view headers = payload_sv.substr(0, header_end);
            app_details << "--- Headers ---\n" << std::string(headers) << "\n\n";

            if (headers.find("Content-Type: application/json") != std::string_view::npos) {
                app_details << "--- JSON Body ---\n";
                std::string_view body = payload_sv.substr(header_end + 4);
                app_details << std::string(body);
            }
            else {
                app_details << "--- Body (Not JSON) ---\n" << "Body data exists but is not identified as JSON.";
            }
        }
        else {
            app_details << std::string(payload_sv);
        }
    }
    else {
        app_details << "Protocol: Unknown or Encrypted (e.g., HTTPS/TLS)";
    }

    return app_details.str();
}


// --- The Sniffer Class ---
class Sniffer {
private:
    SOCKET m_socket = INVALID_SOCKET;
public:
    void Start(const std::string& ipAddress) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::lock_guard<std::mutex> guard(packetMutex); sharedErrorMessage = "WSAStartup failed!"; isCapturing.set(false); return;
        }
        m_socket = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
        if (m_socket == INVALID_SOCKET) {
            std::lock_guard<std::mutex> guard(packetMutex); sharedErrorMessage = "Failed to create socket. Run as Administrator."; WSACleanup(); isCapturing.set(false); return;
        }
        sockaddr_in dest;
        dest.sin_family = AF_INET; dest.sin_port = 0;
        inet_pton(AF_INET, ipAddress.c_str(), &dest.sin_addr.s_addr);
        if (bind(m_socket, (struct sockaddr*)&dest, sizeof(dest)) == SOCKET_ERROR) {
            std::lock_guard<std::mutex> guard(packetMutex); sharedErrorMessage = "Bind failed."; closesocket(m_socket); WSACleanup(); isCapturing.set(false); return;
        }
        DWORD j = 1;
        if (WSAIoctl(m_socket, SIO_RCVALL, &j, sizeof(j), 0, 0, &j, 0, 0) == SOCKET_ERROR) {
            std::lock_guard<std::mutex> guard(packetMutex); sharedErrorMessage = "WSAIoctl failed. Try running as Administrator."; closesocket(m_socket); WSACleanup(); isCapturing.set(false); return;
        }
        char buffer[65536];
        while (isCapturing.get()) {
            int bytesRead = recv(m_socket, buffer, sizeof(buffer), 0);
            if (bytesRead > 0) ParseAndStorePacket(buffer, bytesRead);
            else if (isCapturing.get()) isCapturing.set(false);
        }
    }
    void Stop() {
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket); m_socket = INVALID_SOCKET; WSACleanup();
        }
    }
    void ParseAndStorePacket(char* buffer, int size) {
        if (size < sizeof(IP_HDR)) return;
        CapturedPacket newPacket;
        newPacket.rawData.assign(buffer, buffer + size);
        IP_HDR* ipheader = (IP_HDR*)buffer;
        in_addr src, dest;
        src.s_addr = ipheader->ip_srcaddr; dest.s_addr = ipheader->ip_destaddr;
        char srcIpStr[INET_ADDRSTRLEN], destIpStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src, srcIpStr, sizeof(srcIpStr)); inet_ntop(AF_INET, &dest, destIpStr, sizeof(destIpStr));
        std::stringstream summary_ss;
        summary_ss << srcIpStr << " -> " << destIpStr;
        unsigned short ip_header_size = ipheader->ip_header_len * 4;
        if (ipheader->ip_protocol == 6) {
            TCP_HDR* tcpheader = (TCP_HDR*)(buffer + ip_header_size);
            unsigned short srcPort = ntohs(tcpheader->source_port), dstPort = ntohs(tcpheader->dest_port);
            summary_ss << " | TCP Port: " << srcPort << " -> " << dstPort;
            if (srcPort == 80 || dstPort == 80) summary_ss << " [HTTP]";
        }
        else if (ipheader->ip_protocol == 17) {
            UDP_HDR* udpheader = (UDP_HDR*)(buffer + ip_header_size);
            summary_ss << " | UDP Port: " << ntohs(udpheader->src_port) << " -> " << ntohs(udpheader->dest_port);
        }
        else summary_ss << " | Proto: " << std::to_string(ipheader->ip_protocol);
        summary_ss << " | Size: " << size << " bytes";
        newPacket.summary = summary_ss.str();
        std::lock_guard<std::mutex> guard(packetMutex);
        sharedPacketBuffer.push_back(std::move(newPacket));
    }
};

// --- UI & Main App Logic ---
Sniffer sniffer;
void stopCapture();
void startCapture() {
    if (!selectedIpAddress.get().empty() && !isCapturing.get()) {
        isCapturing.set(true); capturedPackets.set({}); sharedPacketBuffer.clear(); sharedErrorMessage.clear();
        captureThread = std::thread(&Sniffer::Start, &sniffer, selectedIpAddress.get());
    }
}
void stopCapture() {
    if (isCapturing.get()) {
        isCapturing.set(false); sniffer.Stop();
        if (captureThread.joinable()) captureThread.join();
    }
}
void checkSnifferUpdates() {
    std::vector<CapturedPacket> newPackets;
    { std::lock_guard<std::mutex> guard(packetMutex); if (!sharedPacketBuffer.empty()) newPackets.swap(sharedPacketBuffer); }
    if (!newPackets.empty()) { auto current = capturedPackets.get(); current.insert(current.end(), newPackets.begin(), newPackets.end()); capturedPackets.set(current); }
    std::string error;
    { std::lock_guard<std::mutex> guard(packetMutex); if (!sharedErrorMessage.empty()) error.swap(sharedErrorMessage); }
    if (!error.empty()) ui::showSnackBar(error);
}
std::vector<std::string> getLocalIpAddresses() {
    std::vector<std::string> ips;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return ips;
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        addrinfo hints = { 0 }, * res;
        hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
            for (addrinfo* p = res; p != NULL; p = p->ai_next) {
                char ipStr[INET_ADDRSTRLEN];
                sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)p->ai_addr;
                inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipStr, sizeof(ipStr));
                ips.push_back(ipStr);
            }
            freeaddrinfo(res);
        }
    }
    WSACleanup();
    return ips;
}

void showPacketDetailsDialog(const CapturedPacket& packet) {
    std::vector<ui::Widget> detailsWidgets;
    // ** FIX 3: Add a static_cast to resolve the C4267 warning **
    const int size = static_cast<int>(packet.rawData.size());
    const unsigned char* buffer = packet.rawData.data();
    detailsWidgets.push_back(ui::Text("Packet Details", { 20, ui::Colors::white }));
    detailsWidgets.push_back(ui::Divider());
    IP_HDR* ipheader = (IP_HDR*)buffer;
    detailsWidgets.push_back(ui::Text("--- IP Header ---", { 16, ui::Colors::lightBlue }));
    in_addr src, dest;
    src.s_addr = ipheader->ip_srcaddr; dest.s_addr = ipheader->ip_destaddr;
    char srcIpStr[INET_ADDRSTRLEN], destIpStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src, srcIpStr, sizeof(srcIpStr)); inet_ntop(AF_INET, &dest, destIpStr, sizeof(destIpStr));
    detailsWidgets.push_back(ui::Text("Source IP: " + std::string(srcIpStr)));
    detailsWidgets.push_back(ui::Text("Destination IP: " + std::string(destIpStr)));
    detailsWidgets.push_back(ui::Divider());
    unsigned short ip_header_size = ipheader->ip_header_len * 4;
    int payload_offset = ip_header_size;
    if (ipheader->ip_protocol == 6 && size > ip_header_size) {
        TCP_HDR* tcpheader = (TCP_HDR*)(buffer + ip_header_size);
        detailsWidgets.push_back(ui::Text("--- TCP Header ---", { 16, ui::Colors::lightBlue }));
        detailsWidgets.push_back(ui::Text("Source Port: " + std::to_string(ntohs(tcpheader->source_port))));
        detailsWidgets.push_back(ui::Text("Destination Port: " + std::to_string(ntohs(tcpheader->dest_port))));
        std::string flags;
        if (tcpheader->syn) flags += "SYN "; if (tcpheader->ack) flags += "ACK "; if (tcpheader->fin) flags += "FIN ";
        if (tcpheader->rst) flags += "RST "; if (tcpheader->psh) flags += "PSH "; if (tcpheader->urg) flags += "URG ";
        detailsWidgets.push_back(ui::Text("Flags: " + flags));
        payload_offset += tcpheader->data_offset * 4;
        detailsWidgets.push_back(ui::Divider());
    }
    detailsWidgets.push_back(ui::Text("--- Application Layer ---", { 16, ui::Colors::lightBlue }));
    if (size > payload_offset) {
        detailsWidgets.push_back(ui::Text(parseApplicationLayer(buffer + payload_offset, size - payload_offset), { .fontSize = 14, .color = ui::Colors::white }));
    }
    else detailsWidgets.push_back(ui::Text("No payload found."));
    detailsWidgets.push_back(ui::Divider());
    detailsWidgets.push_back(ui::Text("--- Raw Hex Dump ---", { 16, ui::Colors::lightBlue }));
    detailsWidgets.push_back(ui::Text(generateHexDump(buffer, size), { .fontSize = 12, .fontFile = "cour.ttf" }));
    auto dialogContent = ui::Container(
        ui::Column({
            ui::SizedBox(ui::ScrollView(ui::Column(std::initializer_list<ui::Widget>(detailsWidgets.data(), detailsWidgets.data() + detailsWidgets.size()), 5)), {.height = 400}),
            ui::Center(ui::TextButton("Okay", [] { ui::popOverlay(); }, {
                .backgroundColor = ui::Colors::blue,
                .textStyle = {.color = ui::Colors::white},
                .padding = {10, 40}
            }))
            }, 10),
        { .backgroundColor = ui::Colors::darkGrey, .padding = {10,10,10,10} }
    );
    ui::showDialog(dialogContent);
}

ui::Widget buildCaptureView() {
    return ui::Column({
        ui::Container(ui::Row({
            ui::TextButton("Stop", [] { stopCapture(); }, {
                .backgroundColor = ui::Colors::red,
                .border = {.radius = ui::BorderRadius::all(5)},
                .textStyle = {16, ui::Colors::white},
                .padding = {10, 20}
            }),
            ui::Obx([] { return ui::Text("Capturing on: " + selectedIpAddress.get(), {16, ui::Colors::white}); })
        }, 20), {.padding = {10,10,10,10}}),
        ui::Divider(ui::Colors::grey),
        ui::ScrollView(ui::Obx([]() {
            auto packets = capturedPackets.get();
            std::vector<ui::Widget> items;
            items.reserve(packets.size());
            for (const auto& p : packets) {
                items.push_back(
                    ui::TextButton(p.summary, [p] {
                        showPacketDetailsDialog(p);
                    }, {
                        .backgroundColor = {55, 55, 60},
                        .border = {.radius = ui::BorderRadius::all(4)},
                        .textStyle = {14, ui::Colors::lightBlue},
                        .padding = {5,10,5,10}
                    })
                );
            }
            return ui::Column(std::initializer_list<ui::Widget>(items.data(), items.data() + items.size()), 3);
        }))
        });
}
ui::Widget buildIpSelectionView() {
    std::vector<ui::Widget> ipButtons;
    auto ips = getLocalIpAddresses();
    for (const auto& ip : ips) {
        ipButtons.push_back(
            ui::TextButton(ip, [ip] {
                selectedIpAddress.set(ip); startCapture();
                }, {
                    .backgroundColor = ui::Colors::blue,
                    .border = {.radius = ui::BorderRadius::all(5)},
                    .textStyle = {16, ui::Colors::white},
                    .padding = {15,15}
                })
        );
    }
    if (ips.empty()) ipButtons.push_back(ui::Text("Could not find any IP addresses."));
    return ui::Center(ui::Column({
        ui::Text("Select a Network Interface to Sniff", {22, ui::Colors::white}),
        ui::SizedBox({}, {.height = 20}),
        ui::Column(std::initializer_list<ui::Widget>(ipButtons.data(), ipButtons.data() + ipButtons.size()), 10)
        }));
}
ui::Widget buildAppUI() {
    return ui::Scaffold(ui::Container(ui::Obx([]() {
        return isCapturing.get() ? buildCaptureView() : buildIpSelectionView();
        }), { .backgroundColor = {30,30,35} }));
}
void timerCallback();
void scheduleNextTimer() {
    if (ui::App::instance()) ui::App::instance()->addTimer(250, timerCallback);
}
void timerCallback() {
    checkSnifferUpdates();
    scheduleNextTimer();
}

int main() {
    ui::App app(buildAppUI());
    scheduleNextTimer();
    app.run("Pro Sniffer", true, { 1280, 720 });
    stopCapture();
    return 0;
}