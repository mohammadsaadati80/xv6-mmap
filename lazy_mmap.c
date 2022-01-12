#include "types.h"
#include "user.h"

void print_numbers(char *numbers) {
  printf(1, "%s\n", numbers);
}

void tostring(char *str, int numb) {
  int cp = numb;
  int len = 0;
  while (cp) {
    cp = cp / 10;
    len++;
  }
  
  int i = 0;
  while (numb) {
    str[len - i - 1] = (numb % 10) + '0';
    i++;
    numb = numb / 10;
  }
  str[len] = 0;
}

void append(char *str1, char *str2) {
  sleep(1);

  int i = 0;
  while (str1[i] != '\0') {
    i++;
  }
  
  int j = 0;
  while (str2[j] != '\0') {
   str1[i + j] = str2[j];
   j++;
  }
  str1[i + j] = 0;
}

int tointeger(char *str) {
  int res = 0;
  for(int i = 0; i < strlen(str); i++)
    res = res * 10 + (str[i] - '0');
  return res;
}

void convert(char *numbers, int pow) {
  int i = 0, buffer_len = 0;
  char buffer[5];

  char res[1000] = "";
  while (numbers[i]) {
    char c = numbers[i];
    if (c == ' ') {
      buffer[buffer_len++] = 0;
      int tmp = tointeger(buffer);
      int new_number = 1;
      for(int j = 0; j < pow; j++)
        new_number = new_number * tmp;
      char *new_number_str = "";

      tostring(new_number_str, new_number);
      append(res, new_number_str);
      append(res, " ");
      buffer_len = 0;
    }
    
    else {
      buffer[buffer_len++] = c;
    }
    i++;
  }

  strcpy(numbers, res);
}

void first(char *numbers) {
  printf(1, "First program started.\n");
  
  // int fd = open("number.txt", 0);
  // char *numbers = mmap(1000, 0, fd);
  convert(numbers, 1);
  print_numbers(numbers);

  printf(1, "First program finished.\n");
}

void second(char *numbers) {
  printf(1, "Second program started.\n");

  // int fd = open("number.txt", 0);
  // char *numbers = mmap(1000, 0, fd);

  convert(numbers, 2);
  print_numbers(numbers);

  printf(1, "Second program finished.\n");
}

void third(char *numbers) {
  printf(1, "Third program started.\n");

  // int fd = open("number.txt", 0);
  // char *numbers = mmap(1000, 0, fd);
  convert(numbers, 3);
  print_numbers(numbers);

  printf(1, "Third program finished.\n");
}

int main(int argc, char* argv[]) {
  int fd = open("number.txt", 0);
  char *numbers = mmap(1000, 0, fd);

  int pid = fork();
  if (pid == 0) {
    first(numbers);
    exit();
  }

  sleep(200);
  pid = fork();
  if (pid == 0) {
    second(numbers);
    exit();
  }

  sleep(200);
  third(numbers);
  wait();
  wait();
  exit();
}
