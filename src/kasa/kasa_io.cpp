
#include "kasa.hpp"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////////
// Encode a command to a KASA device. Requires an extra 4 bytes in the
// input buffer to store the longer result. Returns the length of the
// encoded command.
// - This version was written by John Greth.
// - This was created by reverse-engineering the decode() function in
//   https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug/hs100.sh
//   by George Georgovassilis. George credits Thomas Baust for providing
//   the KASA device encoding scheme.
////////////////////////////////////////////////////////////////////////////////
int kasa::encode(char* data) {
    char temp[5];
    for (int j = 0; j < 5; j++) temp[j] = data[j];
    data[3] = 171;
    int len = 0;
    while (temp[0]) {
        data[len+4] = data[len+3] ^ temp[0];
        for (int j = 0; j < 4; j++) temp[j] = temp[j+1];
        temp[4] = data[4 + ++len];
    }
    for (int i = 0; i < 4; i++) data[3-i] = (len >> (8*i)) & 255;
    return len+4;
}

////////////////////////////////////////////////////////////////////////////////
// Decode a response from a KASA device.
// - This version was written by John Greth.
// - This was created by reverse-engineering the decode() function in
//   https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug/hs100.sh
//   by George Georgovassilis. George credits Thomas Baust for providing
//   the KASA device encoding scheme.
////////////////////////////////////////////////////////////////////////////////
void kasa::decode(char* data, int len) {
    int msg_len = 0;
    for (int i = 0; i < 4; i++) msg_len = (msg_len << 8) + data[i];
    if (msg_len < len) len = msg_len;
    data[3] = 171;
    for (int i = 0; i < len-4; i++) data[i] = data[i+3] ^ data[i+4];
    data[len-4] = 0;
}

////////////////////////////////////////////////////////////////////////////////
// The c_str in 'data' is sent to the kasa device at the given IPv4
// address. The response is written into 'data'.
////////////////////////////////////////////////////////////////////////////////
void kasa::send_recv(char* data, int data_len) {
    char report_str[1024];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // Set non-blocking so that timeouts can be enforced.
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // Attempt a connection to the device.
    struct sockaddr_in serv_addr =
        {.sin_family = AF_INET, .sin_port = htons(9999)};
    inet_pton(AF_INET, addr, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // Wait for the connection to be established.
    // Timeout after 100ms.
    struct pollfd pfd = {.fd = sock, .events = POLLOUT};
    poll(&pfd, 1, 100);

    if (pfd.revents) {
        // The socket is now open and accepting write data.
        // Send the encoded command.
        sprintf(report_str, "Message sent: %s", data);
        report(report_str, 5);
        send(sock, data, encode(data), 0);
    }
    else {
        // The connection failed or timed out.
        // Close the socket and return a null result.
        report("connection failed", 5);
        close(sock);
        data[0] = '\0';
        return;
    }

    // Wait for a response.
    // Timeout after 1000ms. State checks take <100ms, but compound
    // commands (set and check) may take a bit longer. Once the
    // connection is established, it it likely to complete
    // successfully, so this timeout should rarely be hit.
    pfd = {.fd = sock, .events = POLLIN};
    poll(&pfd, 1, 1000);

    if (pfd.revents) {
        // The socket has data to receive.
        // Fetch and decode it.
        report("decoding response from the device", 5);
        decode(data, recv(sock, data, data_len, 0));
        sprintf(report_str, "Message received: %s", data);
        report(report_str, 5);
    }
    else {
        // The connection failed or timed out.
        report("error, no response received from the device", 5);
        data[0] = '\0';
    }
    close(sock);
}

////////////////////////////////////////////////////////////////////////////////
// Attempt to switch a device on or off.
// Returns the post-switch state.
// If the target state is something other than ON or OFF, the state is
// not changed but the current state is still queried and returned.
////////////////////////////////////////////////////////////////////////////////
int kasa::sync_device(int tgt) {
    char data[1024];
    if (tgt == ON)
        strcpy(data, "{\"system\":{\"set_relay_state\":{\"state\":1},\"get_sysinfo\":null}}");
    else if (tgt == OFF)
        strcpy(data, "{\"system\":{\"set_relay_state\":{\"state\":0},\"get_sysinfo\":null}}");
    else
        strcpy(data, "{\"system\":{\"get_sysinfo\":null}}");
    send_recv(data, 1023);
    if (NULL != strstr(data, "\"relay_state\":1")) return ON;
    if (NULL != strstr(data, "\"relay_state\":0")) return OFF;
    return ERROR;
}
