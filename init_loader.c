/*
*
*   [ CLIBP LINKER ]
*
*   GCC-CLIBP LOADER
*   @Reason: To debug on linux raw binaries used for making OS(s)
*/
#include "../headers/clibp.h"

// Loader's Main Function Declaration
int on_start();
int entry();
int on_exit();

/* Declare Function from build/syscall.o */
void __syscall(long syscall, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

__attribute__((used, externally_visible)) void __execute(char *app, char **args)
{
	if(!app || !args)
		return;

	int pid;
	__syscall(57, -1, -1, -1, -1, -1, -1);
	register long r asm("rax");
	pid = r;

	if(pid == 0)
	{
		__syscall(59, (long)app, (long)args, 0, -1, -1, -1);
	} else if(pid > 0) {
		__syscall(61, pid, 0, 0, -1, -1, -1);
	} else {
		__syscall(1, 1, (long)"fork error\n", 7, -1, -1, -1);
	}
}

static int ___get_cmd_info(char *buffer) {
    __syscall(2, (long)"/proc/self/cmdline", 0, 0, -1, -1, -1);
    register long open asm("rax");
    if(open <= 0)
    {
        return -1;
    }
    
    int fd = open;
    char BUFFER[255];
    __syscall(0, fd, (long)BUFFER, 255, -1, -1, -1);
    register long bts asm("rax");

    int bytes = bts;
    mem_cpy(buffer, BUFFER, bytes);

    __syscall(3, fd, -1, -1, -1, -1, -1);
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

static int ___get_args(char *argv[]) {
    int args = 0;
    char BUFFER[1024];
    int count = ___get_cmd_info(BUFFER);

    char *ptr = BUFFER;
    int test = _count_char(BUFFER, '\0', count);

    for(int i = 0, match = 0, last = 0; i < test; i++) {
        int pos = _find_char(ptr, '\0', count, match++);
        if(pos == -1)
            break;

        argv[args++] = (char *)(ptr + (pos + 1));
    }

    return args;
}

void _start() {
    char *__ARGV__[80];
    int __ARGC__ = ___get_args(__ARGV__);

	set_heap_sz(4096 * 1);
	init_mem();

	int start = on_start();
    if(!start)
        __syscall(60, 1, -1, -1, -1, -1, -1);

    entry(__ARGC__, __ARGV__);
    int exit = on_exit();

    uninit_mem();
    __syscall(60, exit, -1, -1, -1, -1, -1);
}
