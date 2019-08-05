#include "nic/utils/events/recorder/recorder.hpp"
#include "nic/utils/events/recorder/constants.h"
#include "nic/utils/ipc/constants.h"
#include "gen/proto/eventtypes.pb.h"
#include "gen/proto/attributes.pb.h"
#include <stdint.h>
#include <string>
#include <spdlog/spdlog.h>

Logger logger = spdlog::stdout_color_mt("gen_events");

// initializes event recorder with the buffer to hold 10 events.
events_recorder* init_events_recorder()
{
    const char* component = "genevents";
    const char* shm_name = "/genevents.events";
    int shm_size = (SHM_BUF_SIZE * 11) + IPC_OVH_SIZE; // shm can hold up to 10 events
    return events_recorder::init(shm_name, shm_size, component, logger);
}

// generates a random string of given length.
void gen_random_str(char* s, const int len)
{
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
}

// returns a random valid event type from the list of events from system category.
eventtypes::EventTypes get_random_event_type()
{
    bool valid = false;
    int rand_evt_type;

    while (!valid) {
        rand_evt_type = rand() % 5 + static_cast<int>(eventtypes::SERVICE_STARTED);
        valid = eventtypes::EventTypes_IsValid(rand_evt_type);
    }

    return static_cast<eventtypes::EventTypes>(rand_evt_type);
}

// records a single event with the given params, adds a retry mechanism.
int record_event(events_recorder* recorder, eventtypes::EventTypes type, const char* message)
{
    eventattrs::Key key;
    key.set_name("dummy");

    int ret_val = recorder->event(type, "Dummy", key, message);
    int retries = 60;
    while (ret_val == -1 && retries > 0) {
        logger->error("failed to record event {}:{}, retrying", type, message);
        usleep(100000); // 100ms
        ret_val = recorder->event(type, "Dummy", key, message);
        retries--;
    }

    return (ret_val != 0 && retries == 0) ? -1 : 0;
}

// records list of events as per requested rate and total events. message_substr will be added to
// all the events generated. It will be easy to verify the events using this substr.
int record_events(events_recorder* recorder, int rps, int total_events, char* message_substr)
{
    int num_events_recorded = 0;
    int num_events_per_iteration = (rps < 10) ? rps : (rps / 10);
    while (num_events_recorded < total_events) {
        for (int i = 0; i < num_events_per_iteration; i++) {
            char rand_str[10];
            gen_random_str(rand_str, 10);
            std::string evt_message = message_substr + std::string(" ") + rand_str;
            int ret_val = record_event(recorder, get_random_event_type(), evt_message.c_str());
            if (ret_val != 0) {
                return ret_val;
            }
        }
        num_events_recorded += num_events_per_iteration;
        usleep(100000); // 100ms
    }

    return 0;
}

// prints the usage of this program.
void print_usage(char* name)
{
    fprintf(stderr, "Usage: %s \n"
                    "\t -h help \n"
                    "\t -r rate per second \n"
                    "\t -t total events \n"
                    "\t -s substring to be included in all the event messages\n",
        name);
}

// args - rate, total events, message substring
int main(int argc, char** argv)
{
    // set logger pattern
    logger->set_pattern("%L [%Y-%m-%d %H:%M:%S.%f] %P/%n: %v");

    int rate_per_second = 1;
    int total_events = 1;
    char* evt_message_substr = strdup("");

    int opt;
    while ((opt = getopt(argc, argv, "r:t:s:h")) != -1) {
        switch (opt) {
        case 'r':
            rate_per_second = atoi(optarg);
            total_events = rate_per_second;
            break;
        case 't':
            total_events = atoi(optarg);
            break;
        case 's':
            evt_message_substr = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (total_events < rate_per_second) {
        fprintf(stderr, "[-t total events] should be >= rate per second\n");
        exit(EXIT_FAILURE);
    }

    logger->info("rate per second: {}", rate_per_second);
    logger->info("total events: {}", total_events);

    // initialize events recorder
    events_recorder* recorder = init_events_recorder();
    if (recorder == nullptr) {
        printf("failed to create events recorder\n");
        exit(EXIT_FAILURE);
    }

    int ret_val = record_events(recorder, rate_per_second, total_events, evt_message_substr);
    if (ret_val == -1) {
        printf("failed to record requested number of events\n");
        exit(EXIT_FAILURE);
    }

    logger->info("{} events recorded successfully", total_events);
    exit(EXIT_SUCCESS);
}