#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"


int
main(int argc, char *argv[])
{


  char ans[16];
  int n = atoi(argv[1]);
  int j = 0;
  for (int i = 1; i <= n; i++)
	{
		if (n%i == 0)
		{
			int reverse = 0;
			int z = 0;
			int ii = i;
			while (ii > 0)
			{
				while (ii % 10 == 0)
				{
					z++;
					ii /= 10;
				}
				int r = ii % 10;
				reverse += r;
				ii = (ii - r) / 10;
				reverse *= 10;
			}
			reverse /= 10;
			while (reverse > 0)
			{
				int r = reverse % 10;
				reverse = (reverse - r) / 10;
				ans[j] = r + '0';
				j++;
			}
			while (z > 0)
			{
				ans[j] = 0 + '0';
				j++;
				z--;
			}
			ans[j] = ' ';
			j++;
		}
	}
  
  ans[j] = '\n';
  ans[j + 1] = '\0';

  unlink("factor_result.txt");
  int fd = open("factor_result.txt", O_CREATE | O_RDWR);
  write(fd, ans, strlen(ans));

  close(fd);
  exit();
}
