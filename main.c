#include <stdint.h>
#include <stdio.h>

#include "platform.h"
#include "encoding.h"

#define ECB 1
#define CBC 0
#define CTR 0

#include "tiny-AES-c/aes.h"

// GPIO23:
#define TRIG_PIN_OFFSET (23)
#define TRIG_PIN_HIGH() (GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1 << TRIG_PIN_OFFSET))
#define TRIG_PIN_LOW() (GPIO_REG(GPIO_OUTPUT_VAL) &= ~(0x1 << TRIG_PIN_OFFSET))

static void _get_bytes(uint8_t* bytearray, size_t len) {
  while(len) {
    int32_t val = (int32_t) UART0_REG(UART_REG_RXFIFO);
    if (val >= 0) {
      *bytearray++ = 0xFF & val;
      len--;
    }
  }
}

static void _put_bytes(uint8_t* bytearray, size_t len) {
  while(len--) {
    while ((int32_t) UART0_REG(UART_REG_TXFIFO) < 0);
    UART0_REG(UART_REG_TXFIFO) = *bytearray++;
  }
}

static void encrypt_ecb(void) {
  // 128bit key
  uint8_t key[16] = { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16,
                      (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6,
                      (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88,
                      (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };
  // 512bit text
  uint8_t plain_text[16];

  _get_bytes(plain_text, sizeof(plain_text));

  struct AES_ctx ctx;
  AES_init_ctx(&ctx, key);

  TRIG_PIN_HIGH();
  AES_ECB_encrypt(&ctx, plain_text);
  TRIG_PIN_LOW();

  _put_bytes(plain_text, sizeof(plain_text));
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
  GPIO_REG(GPIO_OUTPUT_VAL) &= ~(0x1 << TRIG_PIN_OFFSET);

  // Wait a bit to avoid corruption on the UART.
  // (In some cases, switching to the IOF can lead
  // to output glitches, so need to let the UART
  // reciever time out and resynchronize to the real
  // start of the stream.
  volatile int i=0;
  while(i < 10000){i++;}

  // main loop
  while(1){
    encrypt_ecb();
  }
}
