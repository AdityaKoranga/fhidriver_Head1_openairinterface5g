#include <stdio.h>
#include <string.h>

void make_args(char **argv, int *argc, char *string)
{
  char tmp[1024]={0x0};
  FILE *cmd=NULL;
  int i=0;
  char *p=NULL;
											
  sprintf(tmp, "set - %s && for i in %c$@%c;\n do\n echo $i\ndone",string, '"', '"');
  cmd=popen(tmp, "r");
  while (fgets(tmp, sizeof(tmp), cmd)!=NULL)
  {
    p=strchr(tmp, '\n');
    if (p!=NULL) *p=0x0;
    argv[i] = malloc(strlen(tmp));
    strcpy(argv[i++], tmp);
  }
  *argc=i;
}
