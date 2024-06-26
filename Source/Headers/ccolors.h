#ifdef __unix__

/*
    These sequences only work in the Linux terminal.
*/

/*
    Sequences to print colored text in the terminal.
    No need to do printf(RED + "your text\n" + RESET).

    Text can simply be printed like:
    printf(RED "your text\n" RESET)
*/
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

/*
    Sequences to print stylized text in the terminal.
    Used the same as colors.
    printf(BOLD "your text\n" RESET)
*/
#define BOLD  "\e[1m"
#define UNDR  "\033[4m" // underline

#elif _WIN32

/*
    No colors or text customization
    on windows.

    TODO: In a future update, fix this.
*/

#define RED   " "
#define GRN   " "
#define YEL   " "
#define BLU   " "
#define MAG   " "
#define CYN   " "
#define WHT   " "
#define RESET " "

#define BOLD  " "
#define UNDR  " " // underline

#endif // __unix__