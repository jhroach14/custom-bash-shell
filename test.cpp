#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <errno.h>
using namespace std;

int main(int argc,char* argv[]){
	char buffer[256];
	int n;
	string in;
	while((n=read(STDIN_FILENO,buffer,256))>0){
		if(n != -1){
			buffer[n]='\0';
			for(int i=0;i<n;i++){
				in+=buffer[i];
			}
		}
	}
	cerr<<"std out = "<<STDOUT_FILENOM<<'\n';
	cerr<<"Process recived:\n"<<in;
	if(write(STDOUT_FILENO,in.c_str(),in.size())==-1){
		perror(strerror(errno));
	}
}
