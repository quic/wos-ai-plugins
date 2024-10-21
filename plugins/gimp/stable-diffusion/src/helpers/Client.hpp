/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#pragma once

#include <Winsock2.h >
#include <Ws2tcpip.h>
//#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
//#include <sys/socket.h>
//#include <unistd.h>
#include <vector>

//#include "CppPythonSocketConfig.h"  // To check version, USE_OPENCV...

#define USE_OPENCV
#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace socket_communication {
    class Client {
    public:
        Client();
        Client(const std::string ip, int port);
        ~Client();

        void Init(const std::string ip = "127.0.0.1", int port = 5001);

        void Send(std::string message);

#ifdef USE_OPENCV
        void SendImage(cv::Mat img);
#endif

        std::string Receive();

    private:
        int client_;
        const int size_message_length_ = 16;  // Buffer size for the length
    };


    /**
     * @brief Simple timer class to evaluate exacution times
     * @code
     * Timer timer;
     * timer.start();
     * // do stuff
     * timer.stop("Time consumed doing stuff");
     * @endcode
     */
    class Timer {
    public:
        /**
        * @brief Trigger timer (initial time stored in a private variable)
        */
        void start() {
            t_start_ = std::chrono::system_clock::now();
        }

        /**
         * @brief Stop stop timer and print elapsed time
         * @param msg String included before displaying the time
         */
        void stop(std::string msg) {
            std::chrono::system_clock::time_point end =
                std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - t_start_;
            std::cout << msg << ": " << elapsed_seconds.count() << "s" << std::endl;
        }

        /**
         * @Brief Returns a string with current timestamp
         */
        std::string now() {
            time_t rawtime;
            struct tm* timeinfo;
            char buffer[80];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(buffer, sizeof(buffer), "%d-%m-%Y_%H:%M:%S", timeinfo);
            std::string stri(buffer);
            return stri;
        }

    private:
        std::chrono::system_clock::time_point t_start_;
    };

} // namespace socket_communication
