//
//
//

#include "app.h"
#include "editor.h"
#include "arch.h"       /* <root>/arch/arch.h */
#include "serial.h"     /* <root>/system/serial.h */

#define I2C_ADDRESS  0x20
// blinker period defined in microseconds
#define BLINK_PERIOD  1000000

// MACRO to check timeouts
#define UL_POSITIVE(ul) ((ul) < (((unsigned long)(-1))>>1))

// RUN LED switching function, released in hardware-specific part (main.c)
//
void led_run(unsigned char bOn);
void led_custom(unsigned char bOn);
void line1(unsigned char bOn);
void line2(unsigned char bOn);
void line_data (unsigned char bOn);

// Some variables for blinking FSM
static unsigned char ucBlinkerFSMState = 0;
static unsigned long ulBlinkerTO = 0;

static console_ctx cs;

static inline void extender_tick(unsigned long ulNow)
{
    unsigned char pbI2CBuf[2];
    void *pI2C = i2c_get(0);

    switch(ucBlinkerFSMState) {
        case 0:
        case 3:
                // Waiting for timeout to switch User LED ON (state 0) of OFF (state 3)
                if (UL_POSITIVE(ulNow - ulBlinkerTO)) {
                    ulBlinkerTO += BLINK_PERIOD;
                    ucBlinkerFSMState++;
                } else break;
        case 1:
        case 4:
                // trying to send a command to extender
                // (we work with pin 0 of port 0, all the other pins assumes zeroed)
                // Led is switching ON by LOW level at our board
                pbI2CBuf[0] = 2; pbI2CBuf[1] = (ucBlinkerFSMState == 1)? 0 : 1;
                if (i2c_master_send(pI2C, (I2C_ADDRESS << 1), pbI2CBuf, 2, 1)) break;
                ucBlinkerFSMState++;
        case 2:
        case 5:
                if (!i2c_io_ready(pI2C)) break;
                ucBlinkerFSMState++;
                if (ucBlinkerFSMState > 5) ucBlinkerFSMState = 0;
    }
}

static unsigned char ppp[4];

// Application-level Initialization.
// - switch RUN LED on
// - configure extender to OUTPUT/HIGH for channel P0_0
//
void app_initialize(void)
{
    unsigned uLen;
    void *pS;
    unsigned char pbI2CBuf[3];
    void *pI2C = i2c_get(0);
    unsigned char x;

    led_run(1);

    // set output levels to HIGH on port 0 pin 0
    // (see PCA9555A datasheet for details)
    pbI2CBuf[0] = 2; pbI2CBuf[1] = 1; 
    if (!i2c_master_send(pI2C, (I2C_ADDRESS << 1), pbI2CBuf, 2, 1)) {
        while (!(i2c_io_ready(pI2C))) { __asm("nop"); }
    }

    // set inversion to OFF on both extender ports
    // (see PCA9555A datasheet for details)
    pbI2CBuf[0] = 4; pbI2CBuf[1] = 0; pbI2CBuf[2] = 0; 
    if (!i2c_master_send(pI2C, (I2C_ADDRESS << 1), pbI2CBuf, 3, 1)) {
        while (!(i2c_io_ready(pI2C))) { __asm("nop"); }
    }

    // configure pin 0 on port 0 as OUTPUT
    pbI2CBuf[0] = 6; pbI2CBuf[1] = 0xFE;
    if (!i2c_master_send(pI2C, (I2C_ADDRESS << 1), pbI2CBuf, 2, 1)) {
        while (!(i2c_io_ready(pI2C))) { __asm("nop"); }
    }

    pS = __system_serial_get(0);
    uLen = 15;
    if (__system_serial_write(pS, "Hello, world!\r\n", &uLen) == 1) {
        while(!__system_serial_ready(pS, SERIAL_OPERATION_TX)) {};
    }

/*    if (dump_eeprom() != 1) {
        unsigned uLen = 7;
        if (__system_serial_write(pS, "ERROR\r\n", &uLen) == 1) {
            while(!__system_serial_ready(pS, SERIAL_OPERATION_TX)) {};
        }
    }
*/
    __console_init(&cs, editor_get_command, 0);
    __console_attach_serial(&cs, __system_serial_get(0));

    for(x = 0; x < sizeof(ppp); x++) ppp[x] = 0;
    ppp[3] = 1;
}

// Application internal cycle.
// Functional component gets the time quant
//

#define delay(time, ulTmp) \
do { ulTmp = __micros() + time; \
while (UL_POSITIVE(ulTmp - __micros())) { __asm("nop"); } \
} while(0);


static unsigned long ulCustomLedTO = 0;
static unsigned char ucStateLed = 0;

static unsigned long ulLed = 1; //0xffffffff;


static unsigned char ucNumB = 0;

static unsigned char ucNumb = 0;

static unsigned long ulDataTO = 0;

static unsigned char i = 0;
static unsigned char j = 0;

void app_cycle_tick(void)
{

unsigned long ulTmp;
    unsigned long ulNow = __micros();
    void *pSPI1 = spi_get(1);
    unsigned char ucRes;

    switch(ucStateLed) {
        case 0:
            if(UL_POSITIVE (ulNow - ulCustomLedTO)) ucStateLed = 1;
            else break;
        case 1:
	        spi_begin_transaction(pSPI1, 0);
            line2(0);
            ucStateLed = 2;
        case 2:
            if (spi_transfer(pSPI1, ppp, 4, 0, 0) == 0) break;
            ucStateLed = 3;
        case 3:
            if (spi_io_ready(pSPI1) == 0) break;
            ucStateLed = 4;
        case 4:
            line2(1);
            spi_end_transaction(pSPI1);
            

            ppp[3 - i] <<=1;
            
                //ulLed <<=1;
                        
            j++;
            
            if (j >= 8) 
            {
                j = 0;
                i ++;
                if (i == 4) i = 0; 
                
                ppp[3 - i] = 1;
	        }
                        
            ulCustomLedTO += 1000000;
            ucStateLed = 0;
    }

	    //}

/*switch(ucStateLed) {
    case 0:
        if (ucNumLamp == ucCounter)
        {
        line_data(ulLed);
        }
        else
        {
        line_data(0);
        }
	    line1(1);
	    ulCustomLedTO += 500;
        ucStateLed ++;
     
        break;
    case 1:
        line1(0);
	    ulCustomLedTO += 500;
        if (ucCounter < 31) {
            ucCounter++;
            ucStateLed = 0;                
        } else {
            ucStateLed++;
            ucCounter = 0;
        }
        break;
	case 2:
        line2 (1);
        ulCustomLedTO += 100;
        ucStateLed ++;
        break;
    case 3: 
        line2(0);
        ulCustomLedTO +=1000000;
        //if (ulLed == 0)
        //{
        //    ulLed = 0xffffffff;
        //}
        //else
        //{
        //    ulLed = 0;
        //}
        ucStateLed = 0;

        if (ucNumLamp < 31)
        {
            ucNumLamp ++;
        }
        else
        {
            ucNumLamp =0;
        }

        break;
}
*/
//        }


/*
    switch (ucStateLed)
    {
    case 1: if(UL_POSITIVE (ulNow - ulCustomLedTO))
	    {
	    //ucStateLed = 1 - ucStateLed;
	    ucStateLed++;
	    led_custom(1);
	    ulCustomLedTO += 5000;
	    }
    break;
    case 2: if(UL_POSITIVE (ulNow - ulCustomLedTO))
	    {
	    //ucStateLed = 1 - ucStateLed;
	    ucStateLed++;
	    line1(1);
	    ulCustomLedTO += 5000;
	    }
    break;
    case 3: if(UL_POSITIVE (ulNow - ulCustomLedTO))
	    {
	    //ucStateLed = 1 - ucStateLed;
	    ucStateLed++;
	    led_custom(0);
	    ulCustomLedTO += 10000;
	    }
    break;
    case 4: if(UL_POSITIVE (ulNow - ulCustomLedTO))
	    {
	    //ucStateLed = 1 - ucStateLed;
	    ucStateLed++;
	    line2(1);
	    ulCustomLedTO += 5000;
	    }
    break;
    case 5: if(UL_POSITIVE (ulNow - ulCustomLedTO))
	    {
	    //ucStateLed = 1 - ucStateLed;
	    ucStateLed ++;
	    line1(0);
	    ulCustomLedTO += 15000;
	    }
    break;
    case 6: if(UL_POSITIVE (ulNow - ulCustomLedTO))
	    {
	    //ucStateLed = 1 - ucStateLed;
	    ucStateLed = 1;
	    line2(0);
	    ulCustomLedTO += 20000;
	    }
    break;

    }
*/




    extender_tick(ulNow);

    __console_tick(&cs);
    editor_tick();
}

