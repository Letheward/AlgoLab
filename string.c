#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct {
    unsigned char* data;
    unsigned int   count;
    int            is_temporary; // if you alloc new String.data in a formatter, mark this to 1. todo: is this a good solution?
} String;

unsigned char base64_table[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";



// convert null terminated ascii to String
#define _(s) (String) {(unsigned char*) s, sizeof(s) - 1, 0} // compile time version, does this made the string twice?
String string(char* s) {                                     // runtime version, note: will not alloc a new string
    unsigned int i = 0;
    while (s[i] != '\0') i++;
    return (String) {(unsigned char*) s, i, 0};
}

#define string_free(s) _string_free(&(s))
void _string_free(String* s) {
    free(s->data);
    *s = (String) {0};
}

String copy(String s) {
    unsigned char* data = malloc(sizeof(unsigned char) * s.count);
    for (unsigned int i = 0; i < s.count; i++) data[i] = s.data[i]; // todo: speed
    return (String) {data, s.count, 0};
}

String temp(String s) {
    s.is_temporary = 1;
    return s;
}



String view(String s, int p) {
    if (p >= s.count) return (String) {0};
    return (String) {s.data + p, s.count - p, 0};
}

String concat(String a, String b) {
    
    unsigned int   count = a.count + b.count;
    unsigned char* data  = malloc(sizeof(unsigned char) * count); 
    
    for (int i = 0; i < a.count; i++) data[i]           = a.data[i];
    for (int i = 0; i < b.count; i++) data[i + a.count] = b.data[i];

    return (String) {data, count, 0};
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
            return view(a, i); 
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

    *out_count = count;
    if (count == 1) return NULL; // thus avoid allocation
    
    String* out = malloc(sizeof(String) * count);
    
    current = s;
    for (int i = 0; i < count; i++) {
        String new = find(current, x);
        if (!new.data) {out[i] = current; break;}
        out[i] = (String) {current.data, new.data - current.data, 0};
        current = view(new, x.count);
    };
    
    return out;
}

// why is this slower?
#define split_to_(s, x, out, count) split_to(s, _(x), out, count)
#define split_to__(s, x, out, count) split_to(_(s), _(x), out, count)
void split_to(String s, String x, String* out, unsigned int* out_count) {
    
    if (s.data == NULL || x.data == NULL) return;
    
    unsigned int count = 0;
    
    String current = s;
    while (current.data != NULL) {
        String new = find(current, x);
        if (!new.data) {
            out[count] = current;
            break;
        };
        out[count] = (String) {current.data, new.data - current.data, 0};
        current = view(new, x.count);
        count++;
    };

    *out_count = count + 1;
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
   
    return (String) {data, result_count, 0};
}

#define replace_(s, a, b) replace(s, _(a), _(b))
#define replace__(s, a, b) replace(_(s), _(a), _(b))
String replace(String s, String a, String b) {
    unsigned int count = 0;
    String* chunks = split(s, a, &count);
    if (!chunks) return s;
    String result = join(chunks, count, b);
    free(chunks);
    return result;
}




// note: when use these format functions outside print(), 
//       we will need manually free(), otherwise there'll be memory leak

String format_s32(int value, int base) {

    if (base < 2 || base > 64) return (String) {0};

    unsigned char digits[32] = {0};
    int sign = (value >= 0);
    int count;
    {
        int temp = sign ? value : -value;
        for (count = 0; temp > 0; count++) {
            digits[count] = base64_table[temp % base]; // what about custom digits?
            temp /= base;
        }
    }

    unsigned char* data = malloc(sizeof(unsigned char) * 32);
    if (sign) {
        for (int i = 0; i < count; i++) data[i] = digits[count - i - 1];
    } else {
        count++;
        data[0] = '-';
        for (int i = 1; i < count; i++) data[i] = digits[count - i - 1];
    }

    return (String) {data, count, 1};
}

String format_f32(float value, int digits) {
    return (String) {0}; // this is harder
}




// note: this will auto free string that is_allocated = 1
#define print_(s, ...) print(_(s), __VA_ARGS__)
#define print__(s) print(_(s), 0)
void print(String s, ...) {
    
    if (!s.data) return;
    
    unsigned int chunk_count;
    String* chunks = split_(s, "{}", &chunk_count); // todo: speed!!!
    
    if (!chunks) {
        for (int i = 0; i < s.count; i++) putchar(s.data[i]);
        return;
    }
    
    String arg_strings[128] = {0}; // note: we do this on the stack, so argument count is bounded
    {
        va_list args;
        va_start(args, s);
        for (int i = 0; i < chunk_count - 1; i++) arg_strings[i] = va_arg(args, String);
        va_end(args);
    }

    for (int i = 0; i < chunk_count; i++) {
        String c   = chunks[i];
        String arg = arg_strings[i]; // last one will be 0, thus not printing
        for (int j = 0; j < c.count; j++) putchar(c.data[j]);
        for (int j = 0; j < arg.count; j++) putchar(arg.data[j]);
    }

    free(chunks);
    for (int i = 0; i < chunk_count - 1; i++) {
        if (arg_strings[i].is_temporary) string_free(arg_strings[i]);
    }
}




/* ==== Test ==== */

void test() {


    print__("view(), concat() and temp():\n");
    {
        String a = _("this is ");
        String b = _("a string\n");
        String c = concat(a, b);
        
        print(temp(concat(a, b))); // auto free
        
        print(view(c, 5));
        string_free(c);
    }
    print__("\n");


    print__("join(), split() and replace():\n");    
    {
        String fruits[] = {
            _("Apple"),
            _("Banana"),
            _("Cherry"),
            _("Orange"),
            _("Pear"),
        };

        print(temp(join_(fruits, " | ")));
    }
    {
        unsigned int count = 0;
        String s = _("Apple, Banana, Cherry, Orange, Pear");
        String chunks[10] = {0};
        split_to_(s, ", ", chunks, &count);
        for (int i = 0; i < count; i++) {
            print(chunks[i]);
            print__("\n");
        }

        print(temp(replace_(s, ", ", " - ")));
    }
    print__("\n\n");


    print__("format:\n");
    {
        int    a = 42;
        float  b = 6.28;
        String c = _("I'm a string!!!");
        print_("a is {}, b is {}, c is {}\n", format_s32(a, 10), format_f32(b, 6), c);
    }
    print__("\n");
}


void test_c() {


    printf("view(), concat() and temp():\n");
    {
        char* a = "this is ";
        char* b = "a string\n";
        
        char* c = malloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
        strcpy(c, a);
        strcat(c, b);

        printf(c + 5);
        free(c);
    }
    printf("\n");


    printf("join(), split() and replace():\n"); 
    {        
        char* fruits[] = {
            "Apple",
            "Banana",
            "Cherry",
            "Orange",
            "Pear",
        };

        char* seperator = " | ";
        int char_count = 0;
        int count = sizeof(fruits) / sizeof(fruits[0]);
        
        for (int i = 0; i < count; i++) {
            char_count += strlen(fruits[i]);
        }

        char_count += (count - 1) * strlen(seperator);

        char* result = malloc(sizeof(char) * char_count + 1);
        strcpy(result, fruits[0]);
        for (int i = 1; i < count; i++) {
            strcat(result, seperator);
            strcat(result, fruits[i]);
        }
        
        printf(result);
        free(result);
    }
    {
        char s[] = "Apple, Banana, Cherry, Orange, Pear"; 
        char* _s = malloc(sizeof(char) * strlen(s) + 1);
        strcpy(_s, s);
        
        {
            char* t;
            char* seperator = ", ";
            t = strtok(s, seperator);

            while (t) {
                printf("%s\n", t);
                t = strtok(NULL, seperator);
            }
        }

        {
            char* t;
            char* seperator = ", ";
            t = strtok(_s, seperator);

            while (t) {
                if (t != _s) printf(" - ");
                printf("%s", t);
                t = strtok(NULL, seperator);
            }
        }
    }
    printf("\n\n");


    printf("format:\n"); 
    {
        int   a = 42;
        float b = 6.28;
        char* c = "I'm a string!!!";
        printf("a is %d, b is %f, c is %s\n", a, b, c);
    }
    printf("\n"); 
}




int main() {

    /*
    for (int i = 0; i < 10000; i++) {
        // test();
        // test_c();
    }
    */


    test();
    // test_c();
}
