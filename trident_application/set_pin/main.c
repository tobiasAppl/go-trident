/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include<stdlib.h>
#include<stdio.h>

#include <trident/layer0/layer0.h>

int main(int argc, char* argv[]) {
  LOG_LAYER(TRIDENT_APPLICATION_LOG);
  if(3 != argc) {
      printf("--usage: set_pin <pin_nr> <1|0>\n");
      return -1;
  }
  int pin_nr = atoi(argv[1]);
  int value = atoi(argv[2]);

  if(0 != value && 1 != value) {
      printf("value %d invalid, only 0 or 1 allowed\n", value);
      return -1;
  }
  value = 0 == value ? BIT0 : BIT1;

  ERRNO l0_init_success = layer0_initialize();
  if(ERRNO_NO_ERROR != l0_init_success) {
      ERROR(l0_init_success, "int main(int, char*): error while initializing layer 0");
      return l0_init_success;
  }

  int pin_init_success = layer0_initialize_pin(pin_nr, COMM_IO_OUT);
  if(0 != pin_init_success) {
      printf("Unable to initialize pin, function returned %d\n", pin_init_success);
      return pin_init_success;
  }

  int set_success = layer0_write_to_pin(pin_nr, value);
  if(0 != set_success) {
      printf("Unable to set pin value, function returned %d\n", set_success);
      return set_success;
  }

  int free_success = layer0_finalize();

  return free_success;
}
