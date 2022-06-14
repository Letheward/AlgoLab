#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>




/* ==== Macros ==== */

#define bit64(Type, value) ((union {Type a; u64 b;}) {.a = value}).b




/* ==== Types ==== */

typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef float                   f32;
typedef double                  f64;





/* ==== File System ==== */

static void win32_print_find_data(WIN32_FIND_DATA* data) {

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
    
    FindClose(handle); 
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
    if (!out) goto fail;

    {
        handle = FindFirstFile(s, &find_data);

        u64 acc  = 0;
        out[acc] = malloc(strlen(find_data.cFileName) + 1); // we assume this will succeed for now
        strcpy(out[acc], find_data.cFileName); 
        
        while (1) {
            BOOL found = FindNextFile(handle, &find_data);
            if (!found) break;
            acc++;
            out[acc] = malloc(strlen(find_data.cFileName) + 1); // we assume this will succeed for now
            strcpy(out[acc], find_data.cFileName); 
        }
    }

    FindClose(handle);
    return out;

    fail: 
    FindClose(handle);
    return NULL;
}

void free_filename_c_strings(char** names, u64 count) {
    for (u64 i = 0; i < count; i++)  free(names[i]);
    free(names);
}





/* ==== Load and Save Entire File ==== */

u8* win32_load_file(char* s, u64* count_out) {

    *count_out = 0;
    
    HANDLE handle = CreateFile(s, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);    
    if (handle == INVALID_HANDLE_VALUE) return NULL; // microsoft don't close the handle in their sample code
    
    u64 count;
    {
        LARGE_INTEGER size;
        BOOL result = GetFileSizeEx(handle, &size);
        if (!result) goto fail;

        count = (u64) size.QuadPart;
        *count_out = count;
    }
   
    u8* out = malloc(count);
    if (!out) goto fail;
    
    {
        const u64 chunk = 1024 * 1024 * 1024; // 1 GiB
        
        u8* cursor  = out;
        u64 to_read = count;
        
        DWORD read;
        while (to_read > chunk) {
            BOOL result = ReadFile(handle, cursor, chunk, &read, NULL); 
            if (!result || read != chunk) goto fail_but_allocated;
            cursor  += chunk;
            to_read -= chunk;
        }

        BOOL result = ReadFile(handle, cursor, to_read, &read, NULL); 
        if (!result || read != to_read) goto fail_but_allocated;
    }

    CloseHandle(handle);
    return out;

    fail_but_allocated: free(out); // fall-through

    fail: 
    CloseHandle(handle);
    return NULL;
}

// todo: validate that CREATE_ALWAYS does the right thing
u8 win32_save_file(char* s, u8* data, u64 count) {

    HANDLE handle = CreateFile(s, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);    
    if (handle == INVALID_HANDLE_VALUE) return 0; 
    
    {
        const u64 chunk = 1024 * 1024 * 1024; // 1 GiB
        
        u8* cursor   = data;
        u64 to_write = count;

        DWORD written;
        while (to_write > chunk) {
            BOOL result = WriteFile(handle, cursor, chunk, &written, NULL); 
            if (!result || written != chunk) goto fail;
            cursor   += chunk;
            to_write -= chunk;
        }

        BOOL result = WriteFile(handle, cursor, to_write, &written, NULL); 
        if (!result || written != to_write) goto fail;
    }
    
    CloseHandle(handle);
    return 1;
    
    fail: 
    CloseHandle(handle);
    return 0;
}




int main() {

    u64 count;
    char** filenames = win32_get_all_matched_filename_c_strings("*.c", &count);
    if (!filenames) return 1;

    for (u64 i = 0; i < count; i++) {
        printf("%s\n", filenames[i]);
    }

    {
        char* s = filenames[rand() % count];

        u64 size;
        u8* data = win32_load_file(s, &size);

        win32_save_file("temp", data, size);
        
        printf("\nLoaded %s: %p size: %llu\n", s, data, size);
        free(data);
    }

    free_filename_c_strings(filenames, count);
    
}

