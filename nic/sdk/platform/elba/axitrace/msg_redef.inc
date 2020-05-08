#ifndef MY_MSG
#define MY_MSG ""
#endif
#define MY_INFO cout << TOKEN_APP << TOKEN_INFO << MY_MSG
#define MY_ERROR cout << TOKEN_APP << TOKEN_ERROR << MY_MSG
#define MY_WARNING cout << TOKEN_APP << TOKEN_WARNING << MY_MSG
#define MY_DEBUG if (DEBUG_MODE) cout << TOKEN_APP << TOKEN_DEBUG << MY_MSG
