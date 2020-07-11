package utils

import (
	"bufio"
	"errors"
	log "github.com/sirupsen/logrus"
	"os"
	"strings"
)

// Starting with RHEL compatible file. Add other distro formats and filenames (TODO)
// other options to include /etc/lsb-release
const EtcOsRelease string = "/etc/os-release"

// Returns a map of os-release key-value
func ReadOsRelease() (map[string]string, error) {
	return parseOsReleaseFile(EtcOsRelease)
}

// Internal function to parse given file and return key-value map
func parseOsReleaseFile(fileName string) (map[string]string, error) {
	osRelease := make(map[string]string)

	lines, err := readFile(fileName)
	if err != nil {
		return nil, err
	}

	for _, line := range lines {
		key, value, err := parseLine(line)
		if err == nil {
			osRelease[key] = value
		} else {
			log.Errorf("Failed to parse line :%v, err: %v", line, err.Error())
		}
	}
	return osRelease, nil
}

// Read file into slice of lines
func readFile(fileName string) ([]string, error) {
	file, err := os.Open(fileName)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	lines := make([]string, 0)
	for scanner.Scan() {
		lines = append(lines, scanner.Text())
	}
	return lines, scanner.Err()
}

// main function to parse given line, handling variations
func parseLine(line string) (string, string, error) {
	// skip empty lines
	if len(line) == 0 {
		err := errors.New("Skipping: zero-length lines")
		return "", "", err
	}

	// skip comments
	if line[0] == '#' {
		err := errors.New("Skipping: commented lines")
		return "", "", err
	}

	// try to split string at the first '='
	splitString := strings.SplitN(line, "=", 2)
	if len(splitString) != 2 {
		err := errors.New("Can not parse key=value")
		return "", "", err
	}

	// trim white space from key and value
	key := splitString[0]
	key = strings.Trim(key, " ")
	value := splitString[1]
	value = strings.Trim(value, " ")

	// Handle double quotes
	if strings.ContainsAny(value, `"`) {
		first := string(value[0:1])
		last := string(value[len(value)-1:])

		if first == last && strings.ContainsAny(first, `"'`) {
			value = strings.TrimPrefix(value, `'`)
			value = strings.TrimPrefix(value, `"`)
			value = strings.TrimSuffix(value, `'`)
			value = strings.TrimSuffix(value, `"`)
		}
	}

	// expand anything else that could be escaped
	value = strings.Replace(value, `\"`, `"`, -1)
	value = strings.Replace(value, `\$`, `$`, -1)
	value = strings.Replace(value, `\\`, `\`, -1)
	value = strings.Replace(value, "\\`", "`", -1)
	return key, value, nil
}
