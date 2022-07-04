#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>




/* ==== Pre ==== */

#define bit64(Type, value) ((union {Type a; u64 b;}) {.a = value}).b

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

const float TAU  = 6.283185307179586476925286766559;






/* ==== Utils ==== */

void win32_check_hresult(HRESULT result) {
    if (result < 0) {
        printf("[Error] The HRESULT is 0x%lx\n", result);
        exit(1);
    }
}




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




/* ==== Time ==== */

u64 win32_get_measuring_tick() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

// the result of this will stay the same, so we can save it
u64 win32_get_measuring_tick_per_second() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f); 
    return f.QuadPart;
}








/* ==== Audio ==== */

void win32_print_waveformat_ex(WAVEFORMATEX* f) {
    printf(
        "Format tag:    0x%x\n"
        "Channels:      %u\n"
        "Samples/s:     %lu\n"
        "Bytes/s (avg): %lu\n"
        "Align:         %u\n"
        "Bits/sample:   %u\n"
        "Size:          %u\n",
        f->wFormatTag,
        f->nChannels,
        f->nSamplesPerSec,
        f->nAvgBytesPerSec,
        f->nBlockAlign,
        f->wBitsPerSample,
        f->cbSize
    );
}








/* ==== Test ==== */

void file_test() {
    
    u64 count;
    char** filenames = win32_get_all_matched_filename_c_strings("*.c", &count);
    if (!filenames) exit(1);

    for (u64 i = 0; i < count; i++) printf("%s\n", filenames[i]);
    printf("\n");
    
    {
        char* loaded = filenames[rand() % count];

        u64 size;
        u8* data = win32_load_file(loaded, &size);
        if (!data) exit(1);
    
        printf("Loaded %s: %p size: %llu\n", loaded, data, size);
        
        char* saved = "temp";
        u8 ok = win32_save_file(saved, data, size);
        if (!ok) exit(1);

        printf("Saved  %s: size: %llu\n", saved, size);
        
        free(data);
    }

    free_filename_c_strings(filenames, count);
}

void time_test() {
    
    printf("\n");

    u64 freq  = win32_get_measuring_tick_per_second();
    u64 start = win32_get_measuring_tick();
    
    printf("Printing this string takes...\n");
    
    u64 end   = win32_get_measuring_tick();
    
    printf("%.3f microseconds\n", (end - start) * 1000000 / (f64) freq);
}

void wasapi_test() {

    const CLSID CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}};
    const IID   IID_IMMDeviceEnumerator  = {0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}};
    const IID   IID_IAudioClient         = {0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2}};
    const IID   IID_IAudioRenderClient   = {0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2}};
    
    const int buffer_length = 10 * 10000; // 10ms
    
    IMMDeviceEnumerator* enumerator;
    IMMDevice*           device;
    IAudioClient*        audio_client;
    IAudioRenderClient*  render_client;

    WAVEFORMATEX*        wave_format;

    HANDLE refill_event = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    HANDLE events[1] = { refill_event };
    
    u32 buffer_size;
    
    CoInitialize(NULL);
    
    CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**) &enumerator);
    enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eMultimedia, &device);
    device->lpVtbl->Activate(device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void**) &audio_client);
    
    audio_client->lpVtbl->GetMixFormat(audio_client, &wave_format);
    audio_client->lpVtbl->Initialize(audio_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, buffer_length, 0, wave_format, NULL);
    audio_client->lpVtbl->GetService(audio_client, &IID_IAudioRenderClient, (void**) &render_client);
    audio_client->lpVtbl->GetBufferSize(audio_client, &buffer_size);
    audio_client->lpVtbl->SetEventHandle(audio_client, refill_event);
    
    printf("Buffer Size: %u\n", buffer_size);
    win32_print_waveformat_ex(wave_format);
    
    const float delta_phase = 440.0 * TAU / wave_format->nSamplesPerSec;
    float phase = 0.0;
    
    u32 channel_count = wave_format->nChannels;
    
    // Initial zero fill
    {
        u8* data;
        render_client->lpVtbl->GetBuffer(render_client, buffer_size, &data);
        render_client->lpVtbl->ReleaseBuffer(render_client, buffer_size, AUDCLNT_BUFFERFLAGS_SILENT);
    }
    
    audio_client->lpVtbl->Start(audio_client);
    
    while (1) {
        
        u32 e = WaitForMultipleObjects(1, events, FALSE, INFINITE);

        switch (e) {

            case WAIT_OBJECT_0: // refill_event
            {
                u32 padding;
                audio_client->lpVtbl->GetCurrentPadding(audio_client, &padding);

                u32 available = buffer_size - padding;
                
                f32* data;
                render_client->lpVtbl->GetBuffer(render_client, available, (u8**) &data);

                for (u32 i = 0; i < available; i++) {
                    
                    f32 v = sinf(phase) * 0.25;
                    phase += delta_phase;
                    
                    for (u32 j = 0; j < channel_count; j++) {
                        *data = v;
                        data++;
                    }
                }
                
                render_client->lpVtbl->ReleaseBuffer(render_client, available, 0);
                
                break;
            }

            default: break;
        }
    }
    
    audio_client->lpVtbl->Stop(audio_client);

    CoTaskMemFree(wave_format);
    enumerator   ->lpVtbl->Release(enumerator);
    device       ->lpVtbl->Release(device);
    audio_client ->lpVtbl->Release(audio_client);
    render_client->lpVtbl->Release(render_client);

    CoUninitialize();
}




/*

Build:

~~~ sh
gcc win32_stuff.c -Wall -std=c99 -pedantic -lole32 && ./a
~~~

*/


int main() {


    /*/
    file_test();    
    time_test();
    /*/

    wasapi_test();

}


