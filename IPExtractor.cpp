/*
    EECS 581 Assignment 1
    Extracting IPv4 Addresses from Noisy Text
    Inputs: Noisy text stream with potential IP Address within
    Outputs: IP Address & Port Number
    Name: Ian Ruebelmann
    Creation date: 9/27/26
    LLM Used: Claude Opus 5.5
*/

#include <iostream>
#include <string>
// AI Generation Start
#include <cctype>
#include <cstddef>
using namespace std;

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
            outAddress = addr; //Added by IR
            outPort = port; //Added by IR
            return true;
        }
    }

    return false;
}

std::string ipv4ToString(unsigned long address)
{
    std::string result;
    for (int shift = 24; shift >= 0; shift -= 8) {
        unsigned int octet = static_cast<unsigned int>((address >> shift) & 0xFFUL);
        result += std::to_string(octet);
        if (shift > 0)
            result += '.';
    }
    return result;
}
//AI Generation End

int main()
{
    while(true)
    {
        char buffer[256];
        cout << "Enter a string (or 'END' to quit): ";
        cin.getline(buffer, 256);
        string input = buffer;
        if(input == "END") break;
        unsigned long outAddress = -1;
        int outPort = 0;
        bool extracted = extractIPv4(input, outAddress, outPort);
        if (extracted == false) cout << "Invalid input: no valid IPv4 address found\n";
        else
        {
            string IpAddress = ipv4ToString(outAddress);
            cout << "Extracted IPv4 address: ";
            cout << IpAddress ;
            cout << " (decimal value: " << outAddress << ", port: " << (outPort == -1 ? "none" : to_string(outPort)) << ")\n";
        }
    }
    cout << "Program terminated.\n";
    return 0;
}