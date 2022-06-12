#include <stdio.h>
#include <windows.h>



typedef unsigned long long int u64;

#define bit64(Type, value) ((union {Type a; u64 b;}) {.a = value}).b

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


char** win32_get_all_matched_filename_c_strings(char* s, u64* count_out) {

    *count_out = 0;

    WIN32_FIND_DATA find_data;
    HANDLE handle = FindFirstFile(s, &find_data);
    if (handle == INVALID_HANDLE_VALUE) return NULL;
    
    u64 count = 1;
    
    while (1) {
        BOOL found = FindNextFile(handle, &find_data);
        if (!found) break;
        count++;
    }

    *count_out = count;

    char** out = malloc(sizeof(char*) * count); // using malloc(), ehh...
    {
        handle = FindFirstFile(s, &find_data);

        u64 acc  = 0;
        out[acc] = malloc(strlen(find_data.cFileName) + 1); // ehh....
        strcpy(out[acc], find_data.cFileName); 
        
        while (1) {
            BOOL found = FindNextFile(handle, &find_data);
            if (!found) break;
            acc++;
            out[acc] = malloc(strlen(find_data.cFileName) + 1); // ehh....
            strcpy(out[acc], find_data.cFileName); 
        }
    }

    FindClose(handle);

    return out;
}

void free_filename_c_strings(char** names, u64 count) {
    for (u64 i = 0; i < count; i++) {
        free(names[i]);
    }
    free(names);
}



int main() {

    //win32_print_all_matched_files("*.c");

    u64 count;
    char** filenames = win32_get_all_matched_filename_c_strings("*.c", &count);
    
    for (u64 i = 0; i < count; i++) {
        printf("%s\n", filenames[i]);
    }
    
    free_filename_c_strings(filenames, count);

}

