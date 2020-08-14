// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

#include <arpa/inet.h>
#include <assert.h>
#include <ev++.h>
#include <limits.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "gtest/gtest.h"
#include "syslog_endpoint.hpp"

#define PORT 6969

class SyslogTest : public testing::Test {
private:
    int socket_;
public:
    void SetUp() override {
        struct sockaddr_in addr;
        int rc;
        
        this->socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        assert(this->socket_ != -1);

        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY; 
        addr.sin_port        = htons(PORT);

        rc = bind(this->socket_, (const struct sockaddr *)&addr, sizeof(addr));
        assert(rc != -1);
    }

    void TearDown() override {
        close(this->socket_);
    }

    const void receive(char *buf, int len) {
        recv(this->socket_, buf, len, 0);
    }
};

TEST_F(SyslogTest, BasicTest) {
    char buf[PATH_MAX];
    struct timeval tv;
    
    syslog_endpoint_ptr endpt = syslog_endpoint::factory(
        "127.0.0.1", PORT, true, 0, "oper_test", "app_name", "proc_id");

    gettimeofday(&tv, NULL);
    endpt->send_msg(1, &tv, "-", "this is a test");

    this->receive(buf, PATH_MAX);
    printf("Got: %s\n", buf);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
