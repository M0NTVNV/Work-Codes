//
// Configuring KMK board and start application
//

#include "app/app.h"
#include "arch.h"
#include "kmk_common.h"


/* declarations for INT2 handler */

void led_run(unsigned char bOn);
void kmk_my_int2(void);

//////////


void led_run(unsigned char bOn)
{
    GPIO_WritePin(GPIOB_BASE, 0, (bOn)? 0 : 1);
}

void line_data(unsigned char bOn)
{
    GPIO_WritePin(GPIOA_BASE, 6, bOn);
}

void line1(unsigned char bOn)
{
    GPIO_WritePin(GPIOA_BASE, 3, bOn);
}

void line2(unsigned char bOn)
{
    GPIO_WritePin(GPIOA_BASE, 0, bOn);
}

//////////

#define SPI_DMA_BUFFER_SIZE  16
unsigned char pppp[SPI_DMA_BUFFER_SIZE] __attribute__ ((aligned(4)));

static inline int init_board(void)
{
    unsigned char *pbBuf, ucFlags, ucSPIBaudrate;
    unsigned long ulTmp, ulDMAHi = 0, ulDMALo, x;

    kmk_set_int_vector(2, &kmk_my_int2);
    int_connect(IRQ_UART0, kmk_serial_irq, 0);
    int_connect(IRQ_SPI1, kmk_spi_irq, 1);

    GPIO_SetPinMode(GPIOB_BASE, 0, GPIO_MODE_OUTPUT);  // RUN LED

/*    GPIO_SetPinMode(GPIOA_BASE, 6, GPIO_MODE_OUTPUT);  // line data

    GPIO_SetPinMode(GPIOA_BASE, 3, GPIO_MODE_OUTPUT);  // line1
*/

    GPIO_SetPinMode(GPIOA_BASE, 3, GPIO_MODE_ALT1);
//    GPIO_SetPinMode(GPIOA_BASE, 5, GPIO_MODE_ALT1);
    GPIO_SetPinMode(GPIOA_BASE, 6, GPIO_MODE_ALT1);

    pbBuf = pppp; //(unsigned char *)res_manager_smalloc(SPI_DMA_BUFFER_SIZE);
    ulDMALo = (unsigned long)(CPHYSADDR(pbBuf));
    pbBuf = (unsigned char *)CKSEG1ADDR(pbBuf); // ???

// TODO: get bus clock...
    ucSPIBaudrate = 0x16; //kmk_spi_calc_baudrate(150000000, 5500000);

    kmk_spi_init(1, KMK_ADDR(SPI1_BASE), ucSPIBaudrate, KMK_SPI_FLAG_USE_IRQ, IRQ_SPI1, pbBuf, SPI_DMA_BUFFER_SIZE, ulDMAHi, ulDMALo);
    kmk_init_int_line(IRQ_SPI1, 2, 1);


    GPIO_SetPinMode(GPIOA_BASE, 0, GPIO_MODE_OUTPUT);  // line2

    kmk_serial_init(0, KMK_ADDR(UART0_BASE), KMK_SERIAL_FLAG_IRQ_TX, IRQ_UART0, 0, 0, 0, 0, 0, 0);
    __system_serial_configure(__system_serial_get(0), 115200, 8, 0, 1);
    kmk_init_int_line(IRQ_UART0, 2, 0);


    // I2C 0
    GPIO_SetPinMode(GPIOB_BASE, 6, GPIO_MODE_ALT1);
    GPIO_SetPinMode(GPIOB_BASE, 7, GPIO_MODE_ALT1);

    kmk_i2c_init(0, KMK_ADDR(I2C0_BASE), 400000);

    return 1;
}

//////////


int main(void)
{
    init_board();

    app_initialize();

    while (1) {

        app_cycle_tick();

    }

    // unreacheable
    return 1;
}

