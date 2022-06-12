#include <stdio.h>
#include <windows.h>



typedef unsigned long long int u64;

#define bit64(Type, name) ((union {Type a; u64 b;}) {.a = name}).b

void win32_print_find_data(WIN32_FIND_DATA* data) {

    u64 uc = bit64(FILETIME, data->ftCreationTime);
    u64 ua = bit64(FILETIME, data->ftLastAccessTime);
    u64 uw = bit64(FILETIME, data->ftLastWriteTime);
    
    SYSTEMTIME c; 
    SYSTEMTIME a; 
    SYSTEMTIME w; 

    FileTimeToSystemTime(&data->ftCreationTime,   &c);
    FileTimeToSystemTime(&data->ftLastAccessTime, &a);
    FileTimeToSystemTime(&data->ftLastWriteTime,  &w);
    
    // note: this is in UTC 
    printf(
        "Filename:    %s\n"
        "Size (high): %lu\n"
        "Size (low):  %lu\n"
        "Time (creation):       %5hd %2hd %3hd %4hd %3hd %3hd %4hd    (u64) %llu\n"
        "Time (last access):    %5hd %2hd %3hd %4hd %3hd %3hd %4hd    (u64) %llu\n"
        "Time (last modified):  %5hd %2hd %3hd %4hd %3hd %3hd %4hd    (u64) %llu\n\n",
        data->cFileName,
        data->nFileSizeHigh,
        data->nFileSizeLow,
        c.wYear, c.wMonth, c.wDay, c.wHour, c.wMinute, c.wSecond, c.wMilliseconds, uc,
        a.wYear, a.wMonth, a.wDay, a.wHour, a.wMinute, a.wSecond, a.wMilliseconds, ua,
        w.wYear, w.wMonth, w.wDay, w.wHour, w.wMinute, w.wSecond, w.wMilliseconds, uw
    );
}

// todo: find a way to return the filenames
void win32_print_all_matched_files(char* s) {

    WIN32_FIND_DATA find_data;
    HANDLE handle = FindFirstFile(s, &find_data);
    if (handle == INVALID_HANDLE_VALUE) return;

    win32_print_find_data(&find_data);
    
    while (1) {
        BOOL found = FindNextFile(handle, &find_data);
        if (!found) break;
        win32_print_find_data(&find_data);
    }
}



int main() {

    win32_print_all_matched_files("*.c");

}

