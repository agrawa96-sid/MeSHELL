#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
int main() {
  int fd = open("outfile", O_RDWR);
  if ( fork() ) {
    dup2(fd, 1);
  } else {
    write(1, "hello\n", 6);
  }
}



