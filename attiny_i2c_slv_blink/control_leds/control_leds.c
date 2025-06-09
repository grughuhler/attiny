/* i2c master software to drive attiny i2c slave board that charlieplexes
 * 20 LEDs
 *
 * run by "./control_leds count"
 *
 * SPDX: BSD 2-Clause "Simplified" License: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <i2c/smbus.h>
#include <stdint.h>

#define I2C_BUS "/dev/i2c-8"
#define I2C_SLAVE_ADDR 0x54

/* val is a bit mask.  Bit 0 controls the first LED;
 * bit 19 controls the 20th. */

void do_write(int file, uint32_t val)
{
  int i;

  for (i = 0; i < 4; i++) {
    if (i2c_smbus_write_byte_data(file, i, val & 0xff)) {
      perror("Write to slave failed");
    }
    val = val >> 8;
  }
}

unsigned long get_val(char *str)
{
  char *tmp;
  unsigned long val;

  val = strtoul(str, &tmp, 0);
  if(*tmp != 0) {
    fprintf(stderr, "Error: %s is not a number\n", str);
    exit(EXIT_FAILURE);
  }

  return val;
}


int main(int argc, char **argv)
{
  int file;
  char *fname = I2C_BUS;
  int dev_addr = I2C_SLAVE_ADDR;
  uint32_t val1, val2, i, j;

  if (argc != 2) {
    fprintf(stderr, "needs argument with iteration count value\n");
    exit(EXIT_FAILURE);
  }

  /* First open the i2c bus */
  if ((file = open(fname, O_RDWR)) < 0) {
    perror("Cannot open i2c bus");
    exit(EXIT_FAILURE);
  }

  /* Set address of i2c target to be read/written */
  if (ioctl(file, I2C_SLAVE, dev_addr) < 0) {
    perror("Cannot access slave device");
    close(file);
    exit(EXIT_FAILURE);
  }

  val1 = 0x1;
  val2 = 0x80000;
  for (i = 0; i < get_val(argv[1]); i++) {
    for (j = 0; j < 10; j++) {
      do_write(file, val1 | val2);
      val1 = val1 << 1;
      val2 = val2 >> 1;
      usleep(30000);
    }
    val1 = 0x1;
    val2 = 0x80000;
  }

  close(file);

  return EXIT_SUCCESS;
}
