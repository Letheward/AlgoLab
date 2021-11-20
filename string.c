#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct {
    unsigned char*         data;
    unsigned long long int count;
} String;

// convert null terminated ascii to String
#define _(s) (String) {(unsigned char*) s, sizeof(s) - 1} // compile time version, does this made the string twice?
String string(char* s) {                                  // runtime version
    unsigned long long int i = 0;
    while (s[i] != '\0') i++;
    return (String) {(unsigned char*) s, i};
}

#define print_(s, ...) print(_(s), __VA_ARGS__)
#define print__(s) print(_(s), 0)
void print(String s, ...) {
    if (!s.data) return;
    for (int i = 0; i < s.count; i++) putchar(s.data[i]);
}

String view(String s, int p) {
    if (p >= s.count) return (String) {0};
    return (String) {&s.data[p], s.count - p};
}

// find b in a, return string view (if not return NULL)
#define find_(a, b) find(a, _(b))
#define find__(a, b) find(_(a), _(b))
String find(String a, String b) {
    
    if (a.data == NULL || b.data == NULL) return (String) {0};
    
    for (int i = 0; i < a.count; i++) {
        if (a.data[i] == b.data[0]) {
            for (int j = 0; j < b.count; j++) {
                if (a.data[i + j] != b.data[j]) goto next;
            }
            return (String) {&a.data[i], a.count - i};
            next: continue;
        }
    }
    
    return (String) {0};
}

// split s by x, return an array of string view
#define split_(s, x, count) split(s, _(x), count)
#define split__(s, x, count) split(_(s), _(x), count)
String* split(String s, String x, unsigned int* out_count) {
    
    if (s.data == NULL || x.data == NULL) return NULL;
    
    unsigned int count = 0;
    
    // we do this loop twice (for allocating), is there a better way?
    String current = s;
    while (current.data != NULL) {
        count++;
        String new = find(current, x);
        if (!new.data) break;
        current = view(new, x.count);
    };
    
    String* out = malloc(sizeof(String) * count);
    
    current = s;
    for (int i = 0; i < count; i++) {
        String new = find(current, x);
        if (!new.data) {out[i] = current; break;}
        out[i] = (String) {current.data, new.data - current.data};
        current = view(new, x.count);
    };
    
    *out_count = count;
    return out;
}

String concat(String a, String b) {
    
    unsigned int   count = a.count + b.count;
    unsigned char* data  = malloc(sizeof(unsigned char) * count); // this does allocation, is there a better way?
    
    for (int i = 0; i < a.count; i++) data[i]           = a.data[i];
    for (int i = 0; i < b.count; i++) data[i + a.count] = b.data[i];

    return (String) {data, count};
}


#define join_(s, sep) join(s, sizeof(s) / sizeof(s[0]), _(sep))
String join(String* s, int count, String seperator) {
    
    int result_count = 0;
    for (int i = 0; i < count; i++) result_count += s[i].count;   
    result_count += seperator.count * (count - 1);
    
    unsigned char* data = malloc(sizeof(unsigned char*) * result_count); // todo: this does allocation, is there a better way?

    int start = 0;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < s[i].count; j++) data[start + j] = s[i].data[j];
        start += s[i].count;
        for (int i = 0; i < seperator.count; i++) data[start + i] = seperator.data[i];
        start += seperator.count;
    }
   
    return (String) {data, result_count};
}




int main() {


    print__("view() and concat():\n");    
    {
        String a = _("this is ");
        String b = _("a string\n");

        print(view(concat(a, b), 5));
    }
    print__("\n");    


    print__("join():\n");    
    {
        String fruits[] = {
            _("Apple"),
            _("Banana"),
            _("Cherry"),
            _("Orange"),
            _("Pear"),
        };

        print(join_(fruits, " | "));
    }
    print__("\n\n");    
    

    print__("split():\n");    
    {
        unsigned int count = 0;
        String* result = split__("Apple, Banana, Cherry, Orange, Pear", ", ", &count);
        for (int i = 0; i < count; i++) {
            print(result[i]);
            print__("\n");
        }
    }
    print__("\n");    
    

    return 0;
}
