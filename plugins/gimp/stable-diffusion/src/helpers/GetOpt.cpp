// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include "GetOpt.hpp"
#include <string.h> // for strlen
#include <string>   //for C++ style string comparison

using namespace std;

namespace WinOpt {

    const char* optarg = nullptr;
    int optind = 1;

    static const struct option*
        find_opt(const string str, const struct option* longopts, int* longindex) {
        const struct option* opt = nullptr;
        int idx = 0;
        size_t search_end = str.find_first_of("=");

        for (opt = longopts; opt->name && strlen(opt->name) > 0;
            opt++, idx++) {
            if (str.substr(0, search_end) == opt->name) {
                if (longindex) {
                    *longindex = idx;
                }
                break;
            }
        }
        // if not found, opt would point to the last element of longopts
        // whose name MUST be empty
        return opt->name ? opt : nullptr;
    }

    int GetOptLongOnly(int argc, const char* const argv[], const char*,
        const struct option* longopts, int* longindex) {
        const struct option* opt;
        int arg_len = 0;
        bool is_short = false;
        const char* arg = "";

        optarg = nullptr;
        // no arg, means the end of command
        if (optind >= argc) {
            return -1;
        }

        arg = argv[optind];

        if (arg[0] != '-') {
            optind += 1;
            return '?';
        }

        arg_len = strlen(arg);

        if (arg_len < 2) {
            optind += 1;
            return '?';
        }

        if (!longopts) {
            optind += 1;
            return '?';
        }

        // check short options with this form, -a arg
        if (arg_len == 2) {
            is_short = true;
            // check short options with this form, -a=arg
        }
        else if (arg_len > 3 && arg[2] == '=') {
            is_short = true;
            // check for long options, can be used for both forms
        }
        else if (arg_len > 2 && arg[1] != '=') {
            if (arg[1] != '-') {
                optind += 1;
                return '?';
            }
            is_short = false;
        }

        // start after -- to find the option
        const char* const opt_str = is_short ? &arg[1] : &arg[2];
        opt = find_opt(opt_str, longopts, longindex);
        if (!opt) {
            optind += 1;
            return '?';
        }

        if (opt->hasArg == no_argument) {
            optind += 1;

            if (!opt->flag) {
                return opt->val;
            }
            else {
                *(opt->flag) = opt->val;
                return 0;
            }
        }

        if (opt->hasArg == required_argument) {
            string opt_str = argv[optind];
            size_t assign_idx = opt_str.find_first_of("=");
            bool advance = (assign_idx == string::npos);

            // if it is --opt arg form, this will be true,
            // so we need to advance one step to get arg
            // otherwise, need to stop advance step & extract arg from argv[optind]
            if (advance) {
                optind += 1;
            }

            if (optind >= argc) {
                return '?';
            }
            else {
                // if advance, means it is the form --opt arg
                // otherwise, the form, --opt=arg
                if (advance) {
                    // since optind is advanced, optarg can be assigned directly
                    optarg = argv[optind];
                }
                else {
                    if (assign_idx == opt_str.size()) {
                        return '?';
                    }
                    // for not advanced form,
                    // optarg should point to the address right after "="
                    optarg = &argv[optind][assign_idx + 1];
                }
                // OK, now we are ready to handle the next pair
                optind += 1;

                if (!opt->flag) {
                    return opt->val;
                }
                else {
                    *(opt->flag) = opt->val;
                    return 0;
                }
            }
        }

        return '?';
    } // end of GetOptLongOnly

} // end of WinOpt
