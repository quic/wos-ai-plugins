#pragma once

namespace WinOpt {
    // we implement a similar API for POSIX.2
    // so that some global var are necessary

    extern const char* optarg;
    extern int optind;

    enum {
        no_argument = 0,
        required_argument = 1,
        optional_argument = 2,
    };

    /**
     * name:    the name of the long option.
     * hasArg:  no_argument (or 0) if the option does not take an argument;
     *          required_argument (or 1) if the  option  requires  an argument;
     * flag:    specifies how results are returned for a long option. If flag is NULL,
     *          then GetOptLongOnly() returns val. Otherwise, it returns 0, and flag
     *          points to a variable which is set to val if the option is found, but
     *          left unchanged if the option is not found.
     * val:     the value to return, or to load into the variable pointed to by flag.
     *          The last element of the array has to be filled with zeros.
     */
    struct option {
        const char* name;
        int hasArg;
        int* flag;
        int val;
    };

    /**
     * @brief
     *   this parses command-line options as POSIX getopt_long_only()
     *   but we don't support optstring and optonal_argument now
     * @argc
     *   argument count
     * @argv
     *   argument array
     * @optstring
     *   legitimate option characters, short options, don't support now
     * @longopts
     *   a pointer to the first element of an array of struct option,
     *   hasArg field in the struct option indicates 3 possibilities,
     *   no_argument, required_argument or optional_argument. we don't
     *   support optional_argument now
     * @longindex
     *   If longindex is not NULL, it points to a variable which is set
     *   to the index of the long option relative to longopts
     * @return
     *   -1 for parsing done, '?' for non-recognized arguments, 0 for
     *   flag in longopts is not NULL and saved the val to it
     */
    int GetOptLongOnly(int argc, const char* const argv[], const char* optstring,
        const struct option* longopts, int* longindex);

} // end of WinOpt
