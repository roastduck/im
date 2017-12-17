#ifndef CONFIG_H_
#define CONFIG_H_

/// TCP port
const unsigned short PORT = 19623;

/// Max # of connections wait to be accepted
const int MAX_PENDING_CONN = 64;

/// Default pool size of epoll
const int DEFAULT_EPOLL_POOL = 512;

/// Max # of connections handled after one wait for epoll
const int EPOLL_NUM_ONE_WAIT = 16; // Assert that EPOLL_NUM_ONE_WAIT <= DEFAULT_EPOLL_POOL

/// Size of message buffer used to send or receive
const int MSG_BUF_SIZE = 1024;

#endif // CONFIG_H_

