#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <errno.h>
#include <unistd.h>

using namespace std;

int main(int argc,char* argv[]){

    sleep(4);
    if(tcgetpgrp(STDOUT_FILENO)==getpgrp()){
	cout<< "THIS PROCESS RUNNING IN FOREGROUND\n";
    }
    cout<<"\nchild Process executed sucessfully\n";
}
