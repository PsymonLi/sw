package utils

import (
	"bufio"
	"errors"
	"os"
	"strings"
)

// Starting with RHEL compatible file. Add other distro formats and filenames (TODO)
// other options to include /etc/lsb-release
const EtcOsRelease string = "/etc/os-release"

// Returns a map of os-release key-value
func ReadOsRelease() (osRelease map[string]string, err error) {
	osRelease, err = parseOsReleaseFile(EtcOsRelease)
	return
}

// Internal function to parse given file and return key-value map
func parseOsReleaseFile(fileName string) (osRelease map[string]string, err error) {
	osRelease = make(map[string]string)

	lines, err := readFile(fileName)

	for _, line := range lines {
		key, value, err := parseLine(line)
		if err != nil {
			osRelease[key] = value
		}
	}
	return
}

// Read file into slice of lines
func readFile(fileName string) (lines []string, err error) {
	file, err := os.Open(fileName)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		lines = append(lines, scanner.Text())
	}
	return lines, scanner.Err()
}

// main function to parse given line, handling variations
func parseLine(line string) (key string, value string, err error) {
	err = nil

	// skip empty lines
	if len(line) == 0 {
		err = errors.New("Skipping: zero-length lines")
		return
	}

	// skip comments
	if line[0] == '#' {
		err = errors.New("Skipping: commented lines")
		return
	}

	// try to split string at the first '='
	splitString := strings.SplitN(line, "=", 2)
	if len(splitString) != 2 {
		err = errors.New("Can not parse key=value")
		return
	}

	// trim white space from key and value
	key = splitString[0]
	key = strings.Trim(key, " ")
	value = splitString[1]
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
	return
}
