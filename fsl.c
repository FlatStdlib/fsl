#include "../base/headers/fsl.h"

const string HELP_BANNER = "    Argument      Description\n"
						   "______________________________________\n"
    					   "--output      output binary\n"
    					   "-c            object file\n"
    					   "-co           Combine object file(s)";


#define GCC_FLAGS 8
string GCC_COMPILE_FLAGS[GCC_FLAGS] = {
	"/usr/bin/gcc",
	"-ffunction-sections",
	"-fdata-sections",
	"-Wl,--gc-sections",
	"-nostdlib",
	"-ffreestanding",
	"-c",
	NULL
};

#define LD_FLAGS 4
string LD_COMPILE_FLAGS[LD_FLAGS] = {
    "/usr/bin/ld",
	"--no-relax",
    "-o",
    NULL
};

const string CLIBP_LIB = "/usr/lib/libbase.a";
const string CLIBP_LOADER = "/usr/lib/loader.o";

int create_gcc_command(sArr command)
{
	int i = 0;
	for(i = 0; i < GCC_FLAGS - 1; i++)
		if(GCC_COMPILE_FLAGS[i] != NULL)
		command[i] = str_dup(GCC_COMPILE_FLAGS[i]);

	return i;
}

int create_ld_command(sArr command)
{
	int i = 0;
	for(i = 0; i < LD_FLAGS - 1; i++)
		if(LD_COMPILE_FLAGS[i] != NULL)
			command[i] = str_dup(LD_COMPILE_FLAGS[i]);

	return i;
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

/*
	Since this is a one operation thing. Heap gets free'd on exit
*/
int entry(int argc, string argv[]) {
	if(argc == 2 && find_string(argv[1], "--help") > -1)
	{
		println(HELP_BANNER);
		return 0;
	}

	if(argc < 3)
	{
		println("[ x ] \x1b[31mError\x1b[39m, Invalid arguments provided\nUsage: gclibp <c_file> <opt> <output>\nUse --help for help or more arguments");
		return 1;
	}

	int idx;
	char BUFFER[1024];
	if((idx = array_contains_str((array)argv, "--output")) > -1) {
		string output_file = argv[idx + 1];

		sArr gcc_cmd = allocate(sizeof(char *), argc + 3);
		int pos = create_gcc_command(gcc_cmd);

		/* Add the remaining files or command */
		for(int c = idx + 2; c < argc; c++) {
			if(str_cmp(argv[c], "--cflags")) continue;
			gcc_cmd[pos++] = str_dup(argv[c]);
		}

		gcc_cmd[pos] = NULL;

		/*
			Debug GCC Command
		*/
		// print("Command: ");
		// for(int n = 0; n < pos; n++)
		// 	print("'"), print(gcc_cmd[n]), print("' ");
		
		// print("\n");

		println("[ + ] Compiling to object file(s)....");
		__execute(gcc_cmd[0], gcc_cmd);
		sArr ld_cmd = allocate(sizeof(char *), argc + 2);
		int ld_pos = create_ld_command(ld_cmd);
		for(int i = idx + 1; i < argc; i++)
		{
			if(str_cmp(argv[i], "--cflags")) break;
			// print("BUFFER LEN: "), _printi(len), print(" -> "), println(argv[i]);
			int cnt = count_char(argv[i], '/');
			int pos = find_char_at(argv[i], '/', cnt);
			if(pos > -1 && find_string(argv[i], "/") > -1)
			{
				argv[i] += pos + 1;
			}
			
			int len = str_len(argv[i]);
			if(argv[i][len - 1] == 'c') {
				argv[i][len - 1] = 'o';
			}

			ld_cmd[ld_pos++] = str_dup(argv[i]);
		}

		ld_cmd[ld_pos++] = str_dup(CLIBP_LIB);
		ld_cmd[ld_pos++] = str_dup(CLIBP_LOADER);
		ld_cmd[ld_pos] = NULL;

		/*
			Debug Linker Command
		*/
		// print("LINKER: ");
		// for(int n = 0; n < ld_pos; n++)
		// 	print(ld_cmd[n]), print(" ");

		// print("\n");

		/* Create binary with object files */
		if(array_contains_str((array)argv, "-c") > -1)
			return 0;

		__sprintf(BUFFER, "[ + ] Linking with /usr/lib/libclibp.a and Producing '%s'....", output_file);
		println(BUFFER);
		__execute(ld_cmd[0], ld_cmd);

		/* Remove object files */
		sArr rm = allocate(sizeof(char *), argc + 2);
		rm[0] = str_dup("/usr/bin/rm");
		for(int i = idx + 1, c = 1; i < argc; i++) {
			if(str_cmp(argv[i], "--cflags")) break;
			int pos = find_char(argv[i], '/');
			if(pos > -1)
			{
				printi(pos);
				string sub = get_sub_str(argv[i], pos + 1, str_len(argv[i]) - 1);
				println(sub);
				pfree(argv[i], 1);
				argv[i] = sub;
			}

			int len = str_len(argv[i]);
			if(argv[i][len - 1] == 'o') {
				rm[c++] = str_dup(argv[i]);
				rm[c] = NULL;
			}
		}

    	__execute(rm[0], rm);
		return 0;
	}

	string C_FILE = argv[1];
	string OPT = argv[2];
	string OUTPUT = argv[3];

	string COPY = str_dup(C_FILE);
	int len = str_len(COPY);
	COPY[len - 1] = 'o';

	__sprintf(BUFFER, "[ + ] Compiling '%s' to '%s'", C_FILE, COPY);
	println(BUFFER);
	char *GCC_CMD[10] = {
		"/usr/bin/gcc",
		"-nostdlib",
		"-ffunction-sections",
		"-Wl,--gc-sections",
		"-ffreestanding",
		"-c",
		C_FILE,
		"-o",
		COPY,
		0
	};
    __execute(GCC_CMD[0], GCC_CMD);

	if(array_contains_str((array)argv, "-c") > -1)
		return 0;

	char *LINKER_CMD[9] = {
        "/usr/bin/ld",
		"--no-relax",
		"--gc-sections",
        "-o",
        OUTPUT,
		COPY,
        "/usr/lib/libbase.a",
        "/usr/lib/loader.o",
        0
    };

	__sprintf(BUFFER, "[ + ] Linking '%s' with /usr/lib/libclibp.a", COPY);
	println(BUFFER);
	__sprintf(BUFFER, "[ + ] Producing '%s'....", OUTPUT);
	println(BUFFER);
    __execute(LINKER_CMD[0], LINKER_CMD);
	
	/* Remove object file */
	char *rm[3] = {"/usr/bin/rm", COPY, 0};
    __execute(rm[0], rm);
    return 0;
}

int main() {
	_printf("Hi from GCC with clib+\n", 0);
}
