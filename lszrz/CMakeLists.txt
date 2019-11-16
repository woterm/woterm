cmake_minimum_required(VERSION 2.8)
project(winrzsz C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(rz
        canit.c
        crctab.c
        lrz.c
        lsyslog.c
        rbsb.c
        timing.c
        timing.h
        zglobal.h
        zm.c
        zmodem.h
        zperr.c
        zreadline.c
        mywin.h
        mywin.c)

add_executable(sz
        canit.c
        crctab.c
        lsyslog.c
        lsz.c
        rbsb.c
        timing.c
        timing.h
        zglobal.h
        zm.c
        zmodem.h
        zperr.c
        zreadline.c
        mywin.h
        mywin.c)