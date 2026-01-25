/*
    Compile and link with loader
*/
#include "../headers/asm.h"
long __syscall__(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long sys);
void print();

static int _str_len(const char *buffer)
{
    int count = 0;
    for(int i = 0; buffer[i] != '\0'; i++)
    {
        count++;
    }

    return count;
}

static int _mem_cmp(const char *buff, const char *cmp, int sz)
{
    for(int i = 0; i < sz; i++)
    {
        if(buff[i] != cmp[i])
            return 0;
    }

    return 1;
}

int arr_contains(char **args, int argc, char *needle)
{
    if(!args || !needle)
        return -1;

    for(int i = 0; i < argc; i++) {
		if(!args[i])
			break;

        if(_mem_cmp(args[i], needle, _str_len(args[i])))
            return i;
	}

    return -1;
}

static void _mem_cpy(char *dest, const char *buffer, int len)
{
    for(int i = 0; i < len; i++)
    {
        dest[i] = buffer[i];
    }
}

int get_cmd_info(char *buffer) {
    #if defined(__x86__) || defined(__x86_64__)
        long fd = __syscall__((long)"/proc/self/cmdline", 0, 0, -1, -1, -1, _SYS_OPEN);
	#elif defined(__riscv)
    	long fd = __syscall__(-100, (long)"/proc/self/cmdline", 0, 0, -1, -1, _SYS_OPENAT);
	#endif
    if(fd <= 0)
    {
        return -1;
    }

    char BUFFER[255] = {0};
    long bts = __syscall__(fd, (long)BUFFER, 255, -1, -1, -1, _SYS_READ);

    int bytes = bts;
    _mem_cpy(buffer, BUFFER, bytes);

    __syscall__(fd, 0, 0, -1, -1, -1, _SYS_CLOSE);
    return bytes;
}

static int _count_char(const char *buffer, const char ch, int sz) {
    int count = 0;
    for(int i = 0; i < sz; i++)
        if(buffer[i] == ch)
            count++;

    return count;
}

static int _find_char(const char *buffer, const char ch, int sz, int match) {
    int count = 0;
    for(int i = 0; i < sz; i++) {
        if(buffer[i] == ch)
            count++;

        if(count == match)
            return i;
    }

    return -1;
}

void __execute(char *app, char **args)
{
	if(!app || !args)
		return;
    
	long pid = __syscall__(0, 0, 0, -1, -1, -1, _SYS_FORK);

	if(pid == 0)
	{
		__syscall__((long)app, (long)args, 0, -1, -1, -1, _SYS_EXECVE);
	} else if(pid > 0) {
		__syscall__(pid, 0, 0, -1, -1, -1, _SYS_WAIT4);
	} else {
		__syscall__(1, (long)"fork error\n", 7, -1, -1, -1, _SYS_WRITE);
	}
}

int __ARGC__ = 0;
char *__ARGV__[100] = {0};

void get_cmdline_args()
{
    char BUFFER[1024] = {0};
    int count = get_cmd_info(BUFFER);

    char *ptr = BUFFER;
    int test = _count_char(BUFFER, '\0', count);

    for(int i = 0, match = 0, last = 0; i < test; i++) {
        int pos = _find_char(ptr, '\0', count, match++);
        if(pos == -1)
            break;

        __ARGV__[__ARGC__++] = (char *)(ptr + (pos + 1));
    }
}

void _start() {
    get_cmdline_args();

    if(__ARGC__ < 3)
    {
		print("[ x ] Error, Invalid arguments provided...\nUsage: gcc_clibp <c_file> <option> <output> [rest_of_compiler_commands]\n");
		__syscall__(0, 0, 0, 0, 0, 0, _SYS_EXIT);
    }

    char *SRC_CODE_FILE = __ARGV__[1];
    int src_len = _str_len(SRC_CODE_FILE);

    char COPY[255] = {0};
    _mem_cpy(COPY, SRC_CODE_FILE, src_len);
    COPY[src_len - 1] = 'o';
    char *OPT = __ARGV__[2];

	char *rm[3] = {"/usr/bin/rm", COPY, 0};
    if(SRC_CODE_FILE[src_len - 1] == 'c') {
		print("[ + ] Compiling: "), print(SRC_CODE_FILE), print(" to ");

		print(COPY), print("....!\n");

        if(arr_contains(__ARGV__, __ARGC__, "--tcc") > -1) {
            print("Compiling with TCC....\n");
            char *n[9] = {"/usr/bin/tcc", "-ffreestanding", "-std=c99", "-c", SRC_CODE_FILE, "-o", COPY, "-nostdlib", 0};
            __execute(n[0], n);
            __execute("/usr/bin/execstack", (char *[]){"/usr/bin/execstack", "-c", COPY, 0});
        } else {
            char *n[8] = {"/usr/bin/gcc", "-ffreestanding", "-c", SRC_CODE_FILE, "-o", COPY, "-nostdlib", 0};
            __execute(n[0], n);
        }

        if(OPT[1] == 'c') {
            __syscall__(0, 0, 0, -1, -1, -1, _SYS_EXIT);
        	__execute(rm[0], rm);
        }
    }

    if(arr_contains(__ARGV__, __ARGC__, "--nolink") > -1)
	{
        print("Exiting\n");
        __syscall__(0, 0, 0, -1, -1, -1, _SYS_EXIT);
	}

	char *OUTPUT = __ARGV__[3];
	print("[ + ] Linking to "), print(OUTPUT), print("....!\n");
    char *n[8] = {
        "/usr/bin/ld",
		"--no-relax",
        "-o",
        OUTPUT,
		COPY,
        "/usr/lib/libclibp.a",
        "/usr/lib/loader.o",
        0
    };

    __execute(n[0], n);
    __execute(rm[0], rm);

	if(arr_contains(__ARGV__, __ARGC__, "--strip") > -1)
	{
		print("Stripping....!\n");
		char *strip[4] = {"/usr/bin/strip", "--strip-unneeded", OUTPUT, 0};
		__execute(strip[0], strip);
	}

    __syscall__(0, 0, 0, -1, -1, -1, _SYS_EXIT);
}
