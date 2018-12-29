#if !defined(COMMONS_HPP)
#define COMMONS_HPP

#include <chrono>

typedef std::chrono::steady_clock::time_point TimePoint;
typedef void (DirectCallable)(void);

const int RIP_PORT = 520;

const uint8_t RIP_REQUEST = 1;
const uint8_t RIP_RESPONSE = 2;

const int BUFLEN = 1 << 10;

#endif // COMMONS_HPP
