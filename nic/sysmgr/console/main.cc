#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#define CONSOLE_MAGIC_FILE "/sysconfig/config0/.console"
#define SYSTEM_CONFIG_FILE "/sysconfig/config0/system-config.json"

#define BUFLEN 32

void
wait_enter (void)
{
    char c;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) < 1) {
            exit(0);
        }
        if (c == '\n') {
            return;
        }
    }
}

void
print_string (const char *str)
{
    fputs(str, stdout);
    fflush(NULL);
}

static bool console_disabled (void)
{
    boost::property_tree::ptree config_spec;
    std::string console;

    if (access(SYSTEM_CONFIG_FILE, F_OK) != -1)
    {
        try {
            boost::property_tree::read_json(SYSTEM_CONFIG_FILE,
                                            config_spec);
        } catch (std::exception const &ex) {
            syslog(LOG_INFO, "Error reading config spec:\n %s\n",
                   ex.what());
            return (false);
        }
        console = config_spec.get("console", "None");
        if (strcasecmp(console.c_str(), "disabled") == 0) {
            return (true);
        }
    }
    return (false);
}

static bool
console_no_password (void)
{
    if (access(CONSOLE_MAGIC_FILE, F_OK) != -1) {
        return (true);
    }

    return (false);
}

int
main (int argc, char *argv[])
{
    const char *auth[] = {"/bin/login", NULL};
    const char *no_auth[] = {"/bin/login", "-f", "root", NULL};

    if (console_no_password()) {
        // Set to drop straight into bash shell, with no authentication
        // need. Just do so.
        execvp(no_auth[0], (char* const*)no_auth);
    }

    if (console_disabled()) {
        while (1) {
            print_string("Console disabled\n");
            wait_enter();
        }
    } else {
        print_string("Press enter to activate console\n");
        wait_enter();
        execvp(auth[0], (char* const*)auth);
    }   
}
