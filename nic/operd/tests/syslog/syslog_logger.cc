#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/operd/operd.hpp"
#include "lib/operd/logger.hpp"
#include "lib/operd/decoder.h"

int
main (int argc, const char *argv[])
{
    sdk::operd::producer_ptr producer = sdk::operd::producer::create("syslog");

    assert(argc == 2);
    
    producer->write(OPERD_DECODER_PLAIN_TEXT, sdk::operd::INFO, argv[1],
                    strlen(argv[1]) + 1);

    return 0;
}
