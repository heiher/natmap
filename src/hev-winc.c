/*
 ============================================================================
 Name        : hev-winc.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2025 xyz
 Description : Functions for Windows console
 ============================================================================
 */

#if defined(__MSYS__)

#include <windows.h>

static BOOL WINAPI
ctrlHandler (DWORD signal)
{
    if (signal == CTRL_C_EVENT)
        exit (STATUS_CONTROL_C_EXIT);
    return TRUE;
}

int
hev_winc_setup_ctrlc (void)
{
    return SetConsoleCtrlHandler (ctrlHandler, TRUE) ? 0 : -1;
}

#endif /* defined(__MSYS__) */
