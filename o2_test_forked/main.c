//
//  main.c
//  o2_test
//
//  Created by aoostdijk on 28/09/16.
//  Copyright © 2016 aoostdijk. All rights reserved.
//

//for sleep functionality
#include <unistd.h>

#include <stdio.h>
#include "o2.h"

int text_received(o2_message_ptr msg, const char *types,
                  o2_arg_ptr *argv, int argc, void *user_data) {
    const char* message = (const char*)argv[0];
    printf(message);
    printf("\n");
    return 0;
}

void render() {
    //system("clear");
}

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    o2_initialize("o2-test");
    
    o2_add_service("chat");
    o2_add_method("text", "s", &text_received, NULL, FALSE, FALSE);
    
    char *command = malloc( 144 );
    while(1) {
        gets(command);
        if ( strncmp(command, "quit", 4 ) == 0 ) {
            break;
        }
        else {
            o2_send("/chat/text", 0.0, "s", command );
        }
        o2_poll();
        usleep(16000); //approx 60fps
        render();
    }
    
    free(command);
    
    //causes malloc crash (freed but not allocated)
    //this probably explains why it isn't working at the moment...
    //o2_finish();
    
    return 0;
}