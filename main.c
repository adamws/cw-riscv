// See LICENSE for license details.

// This is the program which ships on the HiFive1
// board, executing out of SPI Flash at 0x20400000.

#include <stdint.h>
#include "platform.h"

#define ECB 1
#define CBC 0
#define CTR 0
#include "tiny-AES-c/aes.h"

#ifndef _SIFIVE_HIFIVE1_H
#error "This demo run on HiFive1 only."
#endif

// GPIO23:
#define TRIG_PIN_OFFSET (23)

static const char sifive_msg[] = "\n\r\
\n\r\
               'AES CPA' Demo \n\r\
\n\r";

static void _putc(char c) {
  while ((int32_t) UART0_REG(UART_REG_TXFIFO) < 0);
  UART0_REG(UART_REG_TXFIFO) = c;
}

int _getc(char * c){
  int32_t val = (int32_t) UART0_REG(UART_REG_RXFIFO);
  if (val > 0) {
    *c =  val & 0xFF;
    return 1;
  }
  return 0;
}

static void _puts(const char * s) {
  while (*s != '\0'){
    _putc(*s++);
  }
}

static void _puts_bytes(uint8_t* bytearray) {
  char * pin = (char *) bytearray;
  const char * hex = "0123456789ABCDEF";

  int i;
  for(i = 0; i < 16; ++i) {
    _putc(hex[(*pin >> 4) & 0xF]);
    _putc(hex[(*pin++) & 0xF]);
  }
}

static void test_encrypt_ecb_verbose(void) {
  // 128bit key
  uint8_t key[16] = { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16,
                      (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6,
                      (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88,
                      (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };
  // 512bit text
  uint8_t plain_text[16] = { (uint8_t) 0x6b, (uint8_t) 0xc1, (uint8_t) 0xbe, (uint8_t) 0xe2,
                             (uint8_t) 0x2e, (uint8_t) 0x40, (uint8_t) 0x9f, (uint8_t) 0x96,
                             (uint8_t) 0xe9, (uint8_t) 0x3d, (uint8_t) 0x7e, (uint8_t) 0x11,
                             (uint8_t) 0x73, (uint8_t) 0x93, (uint8_t) 0x17, (uint8_t) 0x2a };

  // print text to encrypt and key
  _puts("ECB encrypt verbose:\n\r");
  _puts("plain text: ");
  _puts_bytes(plain_text);
  _puts("\n\r");

  _puts("key:        ");
  _puts_bytes(key);
  _puts("\n\r");

  _puts("ciphertext: ");

  struct AES_ctx ctx;
  AES_init_ctx(&ctx, key);

  AES_ECB_encrypt(&ctx, plain_text);
  _puts_bytes(plain_text);
  _puts("\n\r");
}

int main(void) {

  // Make sure the HFROSC is on before the next line:
  PRCI_REG(PRCI_HFROSCCFG) |= ROSC_EN(1);
  // Run off 16 MHz Crystal for accuracy. Note that the
  // first line is
  PRCI_REG(PRCI_PLLCFG) = (PLL_REFSEL(1) | PLL_BYPASS(1));
  PRCI_REG(PRCI_PLLCFG) |= (PLL_SEL(1));
  // Turn off HFROSC to save power
  PRCI_REG(PRCI_HFROSCCFG) &= ~(ROSC_EN(1));

  // Configure UART to print
  GPIO_REG(GPIO_OUTPUT_VAL) |= IOF0_UART0_MASK;
  GPIO_REG(GPIO_OUTPUT_EN)  |= IOF0_UART0_MASK;
  GPIO_REG(GPIO_IOF_SEL)    &= ~IOF0_UART0_MASK;
  GPIO_REG(GPIO_IOF_EN)     |= IOF0_UART0_MASK;

  // 115200 Baud Rate
  UART0_REG(UART_REG_DIV) = 138;
  UART0_REG(UART_REG_TXCTRL) = UART_TXEN;
  UART0_REG(UART_REG_RXCTRL) = UART_RXEN;

  // Configure TRIG pin
  GPIO_REG(GPIO_INPUT_EN) &= ~(0x1 << TRIG_PIN_OFFSET);
  GPIO_REG(GPIO_OUTPUT_EN) |= (0x1 << TRIG_PIN_OFFSET);

  // Wait a bit to avoid corruption on the UART.
  // (In some cases, switching to the IOF can lead
  // to output glitches, so need to let the UART
  // reciever time out and resynchronize to the real
  // start of the stream.
  volatile int i=0;
  while(i < 10000){i++;}

  _puts(sifive_msg);

  test_encrypt_ecb_verbose();

  char c = 0;
  while(1){
    GPIO_REG(GPIO_OUTPUT_VAL) &= ~(0x1 << TRIG_PIN_OFFSET);

    volatile uint64_t *  now = (volatile uint64_t*)(CLINT_CTRL_ADDR + CLINT_MTIME);
    volatile uint64_t then = *now + 100;
    while (*now < then) { }

    // Check for user input
    if (c == 0){
      if (_getc(&c) != 0){
        _putc(c);
      }
    }
    GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1 << TRIG_PIN_OFFSET);
  }
}
