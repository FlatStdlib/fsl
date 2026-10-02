/* 
    fsl-gcc chain compiler v1.5 (Production Rewrite)

    Supporting specifically x86 && x86_64 linux ubuntu

    Original repo; https://github.com/FlatStdlib/fsl
*/
#include <fsl.h>

extern i32 __ARGC__;
extern string __ARGV__[50];

string FSL_STDLIBS[] = {
	"/usr/lib/libfsl.a",
	"/usr/lib/libfsl_x86.a"
};

#if defined(__linux__)
    #define CC "gcc"
#else
    #error "Unsupport Platform"
#endif

const string COMPILER_FLAGS[] = {
    "/usr/bin/gcc",
    "-ffunction-sections",
    "-fdata-sections",
    "-nostdlib",
    "-ffreestanding",
    "-c",
    NULL
};

#define LD_FLAGS 5
string LD_LINKER_FLAGS[LD_FLAGS] = {
    "/usr/bin/ld",
	"--no-relax",
    "--gc-sections",
    "-o",
    NULL
};

const string FILES[] = {
    "src/c/_syscall.c",
    "src/c/allocator.c",
    "src/c/any.c",
    "src/c/internal.c",
    "src/c/memory.c",
    "src/c/start_up.c",
    "src/c/stdlib/int.c",
    "src/c/stdlib/char.c",
    "src/c/stdlib/string.c",
    "src/c/stdlib/array.c",
    "src/c/stdlib/map.c",
    "src/c/stdlib/file.c",
    "src/c/stdlib/socket.c",
    "src/c/stdlib/thread.c",
    // "src/c/stdlib/heap_string.c",
    NULL
};

static void __execute(char *app, char **args) 
{ 
	if(!app || !args) 
		return; 
 
	long pid = __syscall__(0, 0, 0, -1, -1, -1, _SYS_FORK); 
 
	if(pid == 0) 
	{ 
		__syscall__((long)app, (long)args, 0, -1, -1, -1, _SYS_EXECVE); 
	} else if(pid > 0) { 
		long ret = __syscall__(pid, 0, 0, -1, -1, -1, _SYS_WAIT4);
	} else { 
		__syscall__(1, (long)"fork error\n", 7, -1, -1, -1, _SYS_WRITE); 
	} 
}

bool validate_c_file(string q, int sz)
{ return (q[sz - 2] == '.' && q[sz - 1] == 'c'); }

char BUILD_COMMAND[2048];
char LINK_COMMAND[2048];
public int entry(int argc, string argv[])
{
    if(argc < 3)
    {
        _printf("[ x ] Error, Invalid arguments provided\nUsage: %s <input_file> <opt> <output_file>\n", argv[0]);
        return 1;
    }

    int DEBUG = 0;
    if(array_contains_str((array)argv, "--debug") > -1)
        DEBUG = 1;

    memzero(BUILD_COMMAND, 2048);

	int pos = 0;
	if((pos = array_contains_str((array)argv, "--cc")) == -1)
	{
	    /* Add Default Command */
    	str_join(BUILD_COMMAND, (array)COMPILER_FLAGS, ' ');
	} else {
		pos++;
        int len = __get_size__(argv[pos]) - 1;
		mem_cpy(BUILD_COMMAND, argv[pos], len);
        BUILD_COMMAND[len] = ' ';
		str_join(BUILD_COMMAND + 1, (array)COMPILER_FLAGS + 1, ' ');
	}

    string executable[50];
    memzero(executable, 50);

    char *C_FILES[1024];
    int _c_files = 0;

    /* Iterate Command Arguments */
    int output_pos = 0;
    int cflags = 0, ld_flags = 0, exec = 0;
    for(int i = 0; i < argc; i++)
    {
        int sz = str_len(argv[i]);
        if(validate_c_file(argv[i], sz))
        {
            str_append(BUILD_COMMAND, argv[i]), str_append(BUILD_COMMAND, " ");
            C_FILES[_c_files] = str_dup(argv[i]);
            C_FILES[_c_files][sz - 1] = 'o';
            _c_files++;
        }

        if(str_cmp(argv[i], "-o"))
            exec = i + 1, output_pos = i + 1;

        if(str_cmp(argv[i], "--cflags"))
            cflags = i + 1;
    }

    /* Add C Flags Upon --cflags */
    if(cflags > 0)
    {
        array p = (array)argv + cflags;
        str_join(BUILD_COMMAND, p, ' ');
    }

    BUILD_COMMAND[str_len(BUILD_COMMAND) - 1] = '\0';

    /*
        GCC Object Compilation Stage
    */

    /* Compilation Arguments */
    int cmd_argc = 0;
    sArr cmd_args = split_string(BUILD_COMMAND, ' ', &cmd_argc);
    __execute(cmd_args[0], cmd_args);

    /* Debug GCC Command */
    if(DEBUG) {
        println("Raw Cmd: "), println(BUILD_COMMAND);
        _printf("\x1b[32mGCC:\x1b[0m '%s'\n", BUILD_COMMAND);
        for(int i = 0; i < cmd_argc; i++)
        {
            if(!cmd_args[i]) break;
            _printf("[%d]: %s\r\n", (ptr)&i, cmd_args[i]);
        }
    }

    /* Exit Upon Object File Flag Request '-c' */
    if(array_contains_str((array)argv, "-obj") > -1)
    {
        println("[ + ] Object File Created");
        return 0;
    }

    /*
        LINKER STAGE 
    */

    str_join(LINK_COMMAND, (array)LD_LINKER_FLAGS, ' ');

    str_append(LINK_COMMAND, argv[output_pos]);
    str_append(LINK_COMMAND, " ");

    for(int i = 0; i < _c_files; i++) {
        if(str_cmp(C_FILES[i], "/usr/lib/libfsl.a") || str_cmp(C_FILES[i], "/usr/lib/loader.o"))
            break;

        if(str_endswith(C_FILES[i], ".o")) {
            if(find_char(C_FILES[i], '/') > -1) {
                int arg_c = 0;
                sArr r = split_string(C_FILES[i], '/', &arg_c);
                if(!r)
                    continue;

                str_append(LINK_COMMAND, r[arg_c - 1]);
                str_append(LINK_COMMAND, " ");

                pfree_array((array)r);
            } else {
                str_append(LINK_COMMAND, C_FILES[i]);
                str_append(LINK_COMMAND, " ");
            }
        }
    }

    str_append(LINK_COMMAND, "/usr/lib/loader.o ");
    str_append(LINK_COMMAND, "/usr/lib/libfsl.a");
    if((ld_flags = array_contains_str((array)argv, "--ldflags")) > -1)
    {
        str_append_array(LINK_COMMAND, (string []){" ", argv[ld_flags + 1], NULL});
    }

    sArr ld_args = split_string(LINK_COMMAND, ' ', &cmd_argc);
    __execute(ld_args[0], ld_args);

    /* Debug Linker Command & Remove Object Files */
    string rm[1024] = {0}; int len = 0;
    if(DEBUG) _printf("\x1b[32mLinker:\x1b[0m '%s'\n", LINK_COMMAND);
    rm[len++] = str_dup("/usr/bin/rm");
    rm[len++] = str_dup("-rf");
    for(int i = 0; i < cmd_argc; i++)
    {
        if(!ld_args[i]) break;

        if(DEBUG) {
            _printf("[%d]: %s\r\n", (ptr)&i, ld_args[i]);
        }
        
        if(str_cmp(ld_args[i], "/usr/lib/libfsl.a") || str_cmp(ld_args[i], "/usr/lib/loader.o"))
            continue;

        if(str_endswith(ld_args[i], ".o"))
            rm[len++] = str_dup(ld_args[i]);

        rm[len] = NULL;
    }

    __execute(rm[0], rm);

    return 0;
}
