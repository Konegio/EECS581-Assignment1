#include <string>
#include <cctype>
#include <cstddef>
#include <iostream>

namespace {

inline bool isDigitAt(const std::string& s, std::size_t i)
{
    return i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]));
}

// Parses one dotted-decimal octet starting at position i.
// Rules: 1-3 digits, value 0-255, no leading zeros ("0" ok, "01" not).
// On success, stores the value, advances i past the digits, returns true.
bool parseOctet(const std::string& s, std::size_t& i, unsigned int& value)
{
    if (!isDigitAt(s, i))
        return false;

    std::size_t start = i;
    unsigned int v = 0;
    while (isDigitAt(s, i) && (i - start) < 3) {
        v = v * 10 + static_cast<unsigned int>(s[i] - '0');
        ++i;
    }

    // A 4th consecutive digit means this isn't a valid octet.
    if (isDigitAt(s, i))
        return false;

    // Reject leading zeros such as "01" or "007" (ambiguous / octal-style).
    if ((i - start) > 1 && s[start] == '0')
        return false;

    if (v > 255)
        return false;

    value = v;
    return true;
}

// Parses a port number starting at position i.
// Rules: 1-5 digits, value 0-65535, no leading zeros ("0" ok, "080" not).
bool parsePort(const std::string& s, std::size_t& i, int& port)
{
    if (!isDigitAt(s, i))
        return false;

    std::size_t start = i;
    long v = 0;
    while (isDigitAt(s, i)) {
        if (i - start >= 5)          // 6+ digits: too long
            return false;
        v = v * 10 + (s[i] - '0');
        ++i;
    }

    if ((i - start) > 1 && s[start] == '0')
        return false;

    if (v > 65535)
        return false;

    port = static_cast<int>(v);
    return true;
}

// Tries to parse a full address (and optional port) beginning exactly at pos.
bool tryParseAt(const std::string& s, std::size_t pos,
                unsigned long& address, int& port)
{
    std::size_t i = pos;
    unsigned long addr = 0;

    for (int octetIndex = 0; octetIndex < 4; ++octetIndex) {
        if (octetIndex > 0) {
            if (i >= s.size() || s[i] != '.')
                return false;
            ++i;
        }
        unsigned int octet = 0;
        if (!parseOctet(s, i, octet))
            return false;
        addr = (addr << 8) | octet;
    }

    // "1.2.3.4.5" is not an IPv4 address; reject a 5th dotted number.
    // A lone trailing dot (end of a sentence) is allowed.
    if (i < s.size() && s[i] == '.' && isDigitAt(s, i + 1))
        return false;

    int p = -1;
    if (i < s.size() && s[i] == ':' && isDigitAt(s, i + 1)) {
        std::size_t j = i + 1;
        if (!parsePort(s, j, p))
            return false;            // malformed port invalidates the match
    }

    address = addr & 0xFFFFFFFFUL;
    port = p;
    return true;
}

} // namespace

bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort)
{
    // Returns true if a valid address was found, false otherwise.
    // On success: outAddress holds the 32-bit value,
    // and outPort holds the port number, or -1 if no port was present.
    // On failure: outAddress is set to 0 and outPort is set to -1.
    outAddress = 0;
    outPort = -1;

    for (std::size_t i = 0; i < str.size(); ++i) {
        if (!isDigitAt(str, i))
            continue;

        // Only start a candidate at the beginning of a number, so we never
        // pick "2.3.4.5" out of "1.2.3.4.5" or "92.168.0.1" out of "192.168.0.1".
        if (i > 0) {
            char prev = str[i - 1];
            if (std::isdigit(static_cast<unsigned char>(prev)) || prev == '.')
                continue;
        }

        unsigned long addr = 0;
        int port = -1;
        if (tryParseAt(str, i, addr, port)) {
            outAddress = addr;
            outPort = port;
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Test harness (remove or wrap in #ifdef if you drop this into a project)
// ---------------------------------------------------------------------------
#ifndef EXTRACT_IPV4_NO_MAIN
static void check(const std::string& input, bool expectOk,
                  unsigned long expectAddr, int expectPort)
{
    unsigned long addr = 12345;
    int port = 12345;
    bool ok = extractIPv4(input, addr, port);
    bool pass = (ok == expectOk) && (addr == expectAddr) && (port == expectPort);
    std::cout << (pass ? "PASS  " : "FAIL  ") << '"' << input << "\" -> "
              << (ok ? "true" : "false") << ", 0x" << std::hex << addr
              << std::dec << ", " << port << '\n';
}

int main()
{
    check("connect to 192.168.1.1 now",          true,  0xC0A80101UL, -1);
    check("xx10.0.0.1:8080yy",                   true,  0x0A000001UL, 8080);
    check("server is 127.0.0.1.",                true,  0x7F000001UL, -1);
    check("0.0.0.0:0",                           true,  0x00000000UL, 0);
    check("255.255.255.255:65535",               true,  0xFFFFFFFFUL, 65535);
    check("bad 256.1.1.1 then 8.8.8.8:53",       true,  0x08080808UL, 53);
    check("1.2.3.4.5",                           false, 0, -1);
    check("01.2.3.4",                            false, 0, -1);
    check("1.2.3",                               false, 0, -1);
    check("1.2.3.4:65536",                       false, 0, -1);
    check("1.2.3.4:",                            true,  0x01020304UL, -1);
    check("1234.1.1.1",                          false, 0, -1);
    check("no address here",                     false, 0, -1);
    check("",                                    false, 0, -1);
    return 0;
}
#endif
