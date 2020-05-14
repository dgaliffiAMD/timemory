// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

/**
 * \file timemory/components/rusage/backends.hpp
 * \brief Implementation of the rusage functions/utilities
 */

#pragma once

#include "timemory/backends/process.hpp"
#include "timemory/utility/macros.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

#if defined(_UNIX)
#    include <sys/resource.h>
#    include <unistd.h>
#    if defined(_MACOS)
#        include <libproc.h>
#        include <mach/mach.h>
#    endif
#elif defined(_WINDOWS)
#    if !defined(NOMINMAX)
#        define NOMINMAX
#    endif
// currently, this is causing a bunch of errors, need to disable
// #    include <psapi.h>
#    include <stdio.h>
#    include <windows.h>
#else
#    error "Cannot define get_peak_rss() or get_page_rss() for an unknown OS."
#endif

//======================================================================================//
//
namespace tim
{
#if defined(_UNIX)

#    if !defined(_GNU_SOURCE)
#        define RUSAGE_THREAD RUSAGE_SELF
#    endif

using rusage_type_t = decltype(RUSAGE_SELF);

inline rusage_type_t&
get_rusage_type()
{
    static auto instance = RUSAGE_THREAD;
    return instance;
}

inline pid_t&
get_rusage_pid()
{
    return process::get_target_id();
}

#endif

inline void
check_rusage_call(int ret, const char* _func)
{
#if defined(DEBUG)
    if(ret > 0)
        printf("[WARN]> rusage call in '%s' returned a non-zero error code: %i\n", _func,
               ret);
#else
    tim::consume_parameters(ret, _func);
#endif
}

int64_t
get_peak_rss();
int64_t
get_page_rss();
int64_t
get_stack_rss();
int64_t
get_data_rss();
int64_t
get_num_swap();
int64_t
get_num_io_in();
int64_t
get_num_io_out();
int64_t
get_num_minor_page_faults();
int64_t
get_num_major_page_faults();
int64_t
get_num_messages_sent();
int64_t
get_num_messages_received();
int64_t
get_num_signals();
int64_t
get_num_voluntary_context_switch();
int64_t
get_num_priority_context_switch();
int64_t
get_bytes_read();
int64_t
get_bytes_written();
int64_t
get_virt_mem();
int64_t
get_user_mode_time();
int64_t
get_kernel_mode_time();

}  // namespace tim
//
//======================================================================================//

// Returns the peak (maximum so far) resident set size (physical
// memory use) measured in bytes, or zero if the value cannot be
// determined on this OS.
//
inline int64_t
tim::get_peak_rss()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

// Darwin reports in bytes, Linux reports in kilobytes
#    if defined(_MACOS)
    constexpr int64_t _units = 1;
#    else
    constexpr int64_t _units = units::kilobyte;
#    endif

    return static_cast<int64_t>(_units * _usage.ru_maxrss);

#elif defined(_WINDOWS)
    /*
    DWORD                   processID = GetCurrentProcessId();
    HANDLE                  hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, TRUE, processID);
    if(NULL == hProcess)
        return static_cast<int64_t>(0);

    int64_t nsize = 0;
    if(GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
        nsize = static_cast<int64_t>(pmc.PeakWorkingSetSize);

    CloseHandle(hProcess);
    return nsize;
    */
    return static_cast<int64_t>(0);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//
// Returns the current resident set size (physical memory use) measured
// in bytes, or zero if the value cannot be determined on this OS.
//
inline int64_t
tim::get_page_rss()
{
#if defined(_UNIX)
#    if defined(_MACOS)
    // OSX
    // kern_return_t kret;
    // task_t task;
    // kret = task_for_pid(mach_task_self(), get_rusage_pid(), &task);

    struct mach_task_basic_info info;
    mach_msg_type_number_t      infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info,
                 &infoCount) != KERN_SUCCESS)
    {
        fprintf(stderr, "Warning! %s@'%s':%i :: task_info(...) != KERN_SUCCESS\n",
                __FUNCTION__, __FILE__, __LINE__);
        return static_cast<int64_t>(0);
    }
    // Darwin reports in bytes
    return static_cast<int64_t>(info.resident_size);

#    else  // Linux

    int64_t           rss = 0;
    std::stringstream fio;
    fio << "/proc/" << get_rusage_pid() << "/statm";
    FILE* fp = fopen(fio.str().c_str(), "r");
    if(fp && fscanf(fp, "%*s%ld", &rss) == 1)
    {
        fclose(fp);
        return static_cast<int64_t>(rss * units::get_page_size());
    }

    if(fp)
        fclose(fp);

    return static_cast<int64_t>(0);

#    endif
#elif defined(_WINDOWS)
    /*
    DWORD                   processID = GetCurrentProcessId();
    HANDLE                  hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if(NULL == hProcess)
        return static_cast<int64_t>(0);

    int64_t nsize = 0;
    if(GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
        nsize = static_cast<int64_t>(pmc.WorkingSetSize);

    CloseHandle(hProcess);
    return nsize;
    */
    return static_cast<int64_t>(0);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_stack_rss()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    const int64_t _units = units::kilobyte * units::clocks_per_sec;
    return static_cast<int64_t>(_units * _usage.ru_isrss);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_data_rss()
{
#if defined(_UNIX)
#    if defined(_MACOS)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    const int64_t _units = units::kilobyte * units::clocks_per_sec;
    return static_cast<int64_t>(_units * _usage.ru_idrss);

#    else  // Linux

    std::string fstatm = [&]() {
        std::stringstream fio;
        fio << "/proc/" << get_rusage_pid() << "/statm";
        return fio.str();
    }();
    int64_t       drss_size = 0;
    std::ifstream ifs;
    ifs.open(fstatm.c_str());
    if(ifs)
    {
        static int64_t dummy = 0;
        ifs >> dummy >> dummy >> dummy >> dummy >> dummy >> drss_size;
    }
    ifs.close();
    return static_cast<int64_t>(drss_size * units::get_page_size());
#    endif
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_swap()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_nswap);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_io_in()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_inblock);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_io_out()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_oublock);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_minor_page_faults()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_minflt);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_major_page_faults()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_majflt);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_messages_sent()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_msgsnd);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_messages_received()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_msgrcv);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_signals()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_nsignals);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_voluntary_context_switch()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_nvcsw);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_num_priority_context_switch()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    return static_cast<int64_t>(_usage.ru_nivcsw);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_bytes_read()
{
#if defined(_MACOS)
    rusage_info_current rusage;
    if(proc_pid_rusage(get_rusage_pid(), RUSAGE_INFO_CURRENT, (void**) &rusage) == 0)
        return rusage.ri_diskio_bytesread;
#elif defined(_LINUX)

#    if defined(TIMEM_DEBUG)
    if(get_env("TIMEM_DEBUG", false))
        printf("[%s@%s:%i]> using pid %li\n", __func__, __FILE__, __LINE__,
               (long int) get_rusage_pid());
#    endif

    std::stringstream fio;
    fio << "/proc/" << get_rusage_pid() << "/io";
    std::string   label = "";
    int64_t       value = 0;
    std::ifstream ifs(fio.str().c_str());
    if(ifs)
    {
        static constexpr int max_lines = 1;
        for(int i = 0; i < max_lines && !ifs.eof(); ++i)
        {
            ifs >> label;
            ifs >> value;
            // if(label.find("read_bytes") != std::string::npos)
            //    return value;
            // if(label.find("rchar") != std::string::npos)
            //    return value;
        }
        if(!ifs.eof())
            return value;
    }
#endif
    return 0;
}

//======================================================================================//

inline int64_t
tim::get_bytes_written()
{
#if defined(_MACOS)
    rusage_info_current rusage;
    if(proc_pid_rusage(get_rusage_pid(), RUSAGE_INFO_CURRENT, (void**) &rusage) == 0)
        return rusage.ri_diskio_byteswritten;
#elif defined(_LINUX)

#    if defined(TIMEM_DEBUG)
    if(get_env("TIMEM_DEBUG", false))
        printf("[%s@%s:%i]> using pid %li\n", __func__, __FILE__, __LINE__,
               (long int) get_rusage_pid());
#    endif

    std::stringstream fio;
    fio << "/proc/" << get_rusage_pid() << "/io";
    std::string   label = "";
    int64_t       value = 0;
    std::ifstream ifs(fio.str().c_str());
    if(ifs)
    {
        static constexpr int max_lines = 2;
        for(int i = 0; i < max_lines && !ifs.eof(); ++i)
        {
            ifs >> label;
            ifs >> value;
            // if(label.find("write_bytes") != std::string::npos)
            //     return value;
            // if(label.find("wchar") != std::string::npos)
            //    return value;
        }
        if(!ifs.eof())
            return value;
    }
#endif
    return 0;
}

//======================================================================================//

inline int64_t
tim::get_virt_mem()
{
#if defined(_UNIX)
#    if defined(_MACOS)
    // OSX
    // kern_return_t kret;
    // task_t task;
    // kret = task_for_pid(mach_task_self(), get_rusage_pid(), &task);

    struct mach_task_basic_info info;
    mach_msg_type_number_t      infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info,
                 &infoCount) != KERN_SUCCESS)
    {
        fprintf(stderr, "Warning! %s@'%s':%i :: task_info(...) != KERN_SUCCESS\n",
                __FUNCTION__, __FILE__, __LINE__);
        return static_cast<int64_t>(0);
    }
    // Darwin reports in bytes
    return static_cast<int64_t>(info.virtual_size);

#    else  // Linux

#        if defined(TIMEM_DEBUG)
    if(get_env("TIMEM_DEBUG", false))
        printf("[%s@%s:%i]> using pid %li\n", __func__, __FILE__, __LINE__,
               (long int) get_rusage_pid());
#        endif

    std::string fstatm = [&]() {
        std::stringstream fio;
        fio << "/proc/" << get_rusage_pid() << "/statm";
        return fio.str();
    }();
    int64_t       vm_size = 0;
    std::ifstream ifs;
    ifs.open(fstatm.c_str());
    if(ifs)
        ifs >> vm_size;
    ifs.close();
    return static_cast<int64_t>(vm_size * units::get_page_size());

#    endif
#elif defined(_WINDOWS)
    return static_cast<int64_t>(0);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_user_mode_time()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    constexpr int64_t MSEC = 1000000;
    return static_cast<int64_t>(_usage.ru_utime.tv_sec * MSEC + _usage.ru_utime.tv_usec);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//

inline int64_t
tim::get_kernel_mode_time()
{
#if defined(_UNIX)
    struct rusage _usage;
    check_rusage_call(getrusage(get_rusage_type(), &_usage), __FUNCTION__);

    constexpr int64_t MSEC = 1000000;
    return static_cast<int64_t>(_usage.ru_stime.tv_sec * MSEC + _usage.ru_stime.tv_usec);
#else
    return static_cast<int64_t>(0);
#endif
}

//======================================================================================//
