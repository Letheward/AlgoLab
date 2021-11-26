#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>




/* ==== Data ==== */

typedef struct {
    unsigned char* data;
    unsigned int   count;
    int            is_temporary; // if you alloc new String.data in a formatter, mark this to 1. todo: is this a good solution?
} String;

unsigned char base64_table[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";




/* ==== Basic Utilities 1 ==== */

// note: this is the pain point of this approach, because C string literals
// have C string semantics, not in our String semantic, so we have to write
// a lot of macros to make APIs (kinda) easy to use, or we have to convert
// back and forth all the time at runtime, which is worse than just using char*

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

// mark a heap allocated String as temporary, 
// useful for print() and concat_many(), etc
// do not use this on static or stack allocated String!
String temp(String s) {
    s.is_temporary = 1;
    return s;
}

// compare strings by content, exact match
#define equal_(a, b) equal(_(a), b)
int equal(String a, String b) {
    if (a.count != b.count) return 0;
    for (unsigned int i = 0; i < a.count; i++) {
        if (a.data[i] != b.data[i]) return 0; // todo: speed!!!
    }
    return 1;
}




/* ==== Basic Utilities 2 ==== */

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

// note: will auto free temp String
String concat_many(int count, ...) {
    
    String arg_strings[128];
    {
        va_list args;
        va_start(args, count);
        for (int i = 0; i < count; i++) arg_strings[i] = va_arg(args, String);
        va_end(args);
    }

    unsigned int result_count = 0;
    for (int i = 0; i < count; i++) {
        result_count += arg_strings[i].count; // todo: solve overflow
    }

    unsigned char* data = malloc(sizeof(unsigned char) * result_count);
    {
        unsigned int counter = 0;
        for (int i = 0; i < count; i++) {
            String s = arg_strings[i];
            for (int j = 0; j < s.count; j++) {
                data[counter] = s.data[j];
                counter++;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        if (arg_strings[i].is_temporary) free(arg_strings[i].data);
    }

    return (String) {data, result_count, 0};
}




/* ==== Token Related Functions ==== */

// find b in a, return string view (if not return NULL)
#define find_(a, b) find(a, _(b))
#define find__(a, b) find(_(a), _(b))
String find(String a, String b) {
    
    if (a.data == NULL || b.data == NULL) return (String) {0};
    
    for (unsigned int i = 0; i < a.count; i++) {
        if (a.data[i] == b.data[0]) {
            for (unsigned int j = 0; j < b.count; j++) {
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
        if (equal(new, x)) {count++; break;} // todo: speed??
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





/* ==== Format and Print ==== */

// note: when use these format functions outside print(), 
//       we will need manually free(), otherwise there'll be memory leak

String format_s32(int value, int base) {

    if (value == 0) return _("0"); // quick fix, is this fast?
    if (base < 2 || base > 64) return (String) {0};

    unsigned char digits[32] = {0}; // todo: speed? just write it to the malloc() buffer?
    int sign = value >> 31;
    int count;
    {
        int temp = sign ? -value : value;
        for (count = 0; temp > 0; count++) {
            digits[count] = base64_table[temp % base]; // what about custom digits?
            temp /= base;
        }
    }

    unsigned char* data = malloc(sizeof(unsigned char) * 32);
    if (sign) {data[0] = '-'; count++;}
    for (int i = sign ? 1 : 0; i < count; i++) data[i] = digits[count - i - 1];

    return (String) {data, count, 1};
}

// todo: accuracy issue, handle infinity and NaN
String format_f32(float value) {

    unsigned int data; 
    {
        unsigned int* p = (unsigned int*) &value; // workaround for aliasing warning
        data = *p; 
    }

    // get data from IEEE 754 bits
    int sign = (data >> 31);
    int exp  = (data << 1 >> 24) - 127 - 23;
    int frac = (data << 9 >> 9) | 0x800000; // add the implicit "1." in ".1010010"

    unsigned long long int temp = frac * 1000000000000; // todo: accuracy 

    if (exp < 0) for (int i = 0; i > exp; i--) temp /= 2;
    else         for (int i = 0; i < exp; i++) temp *= 2;

    unsigned char digits[64] = {0};
    int count; // digit count
    for (count = 0; temp > 0; count++) {
        digits[count] = base64_table[temp % 10]; 
        temp /= 10;
    }
    
    unsigned char* result = malloc(sizeof(unsigned char) * 64);
    unsigned int   result_count = 0;
    
    if (sign) result[0] = '-';
    int dot_pos = count - 12; // todo: find out why it's 12
    
    if (dot_pos <= 0) {
        result_count = count - dot_pos + 2 + sign;
        result[sign    ] = '0'; 
        result[sign + 1] = '.';
        for (int i = 0; i < -dot_pos; i++) result[sign + 2 + i] = '0'; 
        for (int i = 0; i < count; i++) {
            result[sign - dot_pos + i + 2] = digits[count - i - 1]; 
        }
    } else {
        result_count = count + sign + 1;
        result[sign + dot_pos] = '.';
        for (int i =       0; i < dot_pos; i++) result[sign + i    ] = digits[count - i - 1];
        for (int i = dot_pos; i <   count; i++) result[sign + i + 1] = digits[count - i - 1];
    }

    return (String) {result, result_count, 1};
}

// note: will auto free temp String
// todo: replace putchar()
#define print_(s, ...) print(_(s), __VA_ARGS__)
#define print__(s) print(_(s), 0)
void print(String s, ...) {
    
    if (!s.data) return;
    
    unsigned int chunk_count;
    String* chunks = split_(s, "{}", &chunk_count); // todo: speed
    
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
        for (int j = 0; j < c.count;   j++) putchar(c.data[j]);
        for (int j = 0; j < arg.count; j++) putchar(arg.data[j]);
    }

    free(chunks);
    if (s.is_temporary) free(s.data);
    for (int i = 0; i < chunk_count - 1; i++) {
        if (arg_strings[i].is_temporary) free(arg_strings[i].data);
    }
}





/* ==== Tests ==== */

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
    {
        String a = _("this is ");
        String b = _("really a ");
        String c = _("string\n");
    
        print(temp(concat_many(3, a, b, c)));
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
        print__("\n");
    }
    {
        unsigned int count = 0;
        String s = _("Apple, Banana, Cherry, Orange, Pear");
        String* chunks = split_(s, ", ", &count);
        for (int i = 0; i < count; i++) {
            print(chunks[i]);
            print__("\n");
        }
        free(chunks);

        print(temp(replace_(s, ", ", " - ")));
    }
    print__("\n\n");


    print__("format:\n");
    {
        int    a = 42;
        float  b = 6.28;
        String c = _("I'm a string!!!");
        print_("a is {}, b is {}, c is {}\n", format_s32(a, 10), format_f32(b), c);
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
        
        printf(c);
        printf(c + 5);
        free(c);
    }
    {
        char* a = "this is ";
        char* b = "really a ";
        char* c = "string\n";

        char* d = malloc(sizeof(char) * (strlen(a) + strlen(b) + strlen(c) + 1));
        strcpy(d, a); 
        strcat(d, b); 
        strcat(d, c);

        printf(d);
        free(d); 
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
        printf("\n");
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
        quick speed test
        String version slower at -O0, but faster in -O3
        ~~~ sh
        gcc string.c -std=c99 -Wall -pedantic -static -O3 && time ./a > temp
        ~~~
    */
    /*    
    for (int i = 0; i < 10000; i++) {
        test();
        test_c();
    }
    */

    test();
    // test_c();

    return 0;
}
