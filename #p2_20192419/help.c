
#include "phantomutils.h"

int main(int argc, char * argv[]) {
    if (argc >= 3) {
        printf("ERROR TOO MANY ARGS\n");
        printf("Usage: ");
        helpfuncs[6]();
        /**
         * TODO: usage
        */
        exit(1);
    }
    if (argc == 1) {
        printf("Usage: \n");
        for (int i = 0; i < commandscnt; i++) {
            printf("  >");
            helpfuncs[i]();
        }
    }
    else {
        for (int i = 0; i < commandscnt; i++) {
            if (!strcmp(commandsList[i], argv[1])){
                printf("Usage: ");
                helpfuncs[i]();
                exit(0);
            }
        }
        printf("No such help about '%s'\n", argv[1]);
    }
}