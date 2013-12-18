//For the RX target board, the connection is Gray, White, Orange, Blue, and Black, Red, White, Yellow

#include <xc.h>
#include "constants.h"
#include "config.h"
#include "serlcd.h"
#include "nRF2401.h"

#define LED_GREEN_TRIS TRISBbits.TRISB4
#define LED_GREEN PORTBbits.RB4

#define LED_RED_TRIS TRISBbits.TRISB5
#define LED_RED PORTBbits.RB5

#define STRIP_DATA_TRIS TRISAbits.TRISA0
#define STRIP_DATA PORTAbits.RA0

#define STATUS_TRIS TRISBbits.TRISB4
#define STATUS_LED PORTBbits.RB4

#define MODE_SELECT_TRIS TRISBbits.TRISB0
#define MODE_SELECT PORTBbits.RB0
#define MODE_SEND 1

#define DIP_3_TRIS TRISBbits.TRISB1
#define DIP_3 PORTBbits.RB1

unsigned char tx_buf[MAX_PAYLOAD];
unsigned char rx_buf[MAX_PAYLOAD];

void setup(void);

////                            MasterCode                                 ////
void run(void);
void interruptService(void);

////                            Shared Code                                 ////
void delay(void);

////                            System Code                                 ////
void main(void);
void interrupt interrupt_high(void);

void setup(void) {
    //Misc config
    STRIP_DATA_TRIS = OUTPUT;
    STATUS_TRIS = OUTPUT;
    MODE_SELECT_TRIS = INPUT;
    STATUS_LED = 0;
    LED_GREEN_TRIS = OUTPUT;
    LED_RED_TRIS = OUTPUT;
    DIP_3_TRIS = INPUT;

    //Enable internal pullup resistor for port B
    INTCON2bits.RBPU = CLEAR;
    WPUB = 0b1111;

    //This is to toggle pins from digital to analog
    //unimp, RD3, RD2, RD1     RD1, AN10, AN9, AN8 (in order)
    ANCON0 = 0b00000000;
    ANCON1 = 0b11111000;
    
    //NRF port configure (todo: move me)
    TRIS_CE = OUTPUT;
    TRIS_CSN = OUTPUT;
    TRIS_IRQ = INPUT;
    TRIS_SCK = OUTPUT;
    TRIS_MISO = INPUT;
    TRIS_MOSI = OUTPUT;

    //oscillator setup
    OSCCONbits.IRCF = 0b111; //sets internal osc to 111=16mhz, 110=8mhz
    OSCCONbits.SCS = 0b00;
    OSCTUNEbits.PLLEN = 0b1; //1=pllx4 enabled

    //set up timer for LEDs
    T2CONbits.TMR2ON = 1; //enable timer 2
    T2CONbits.T2CKPS = 0b00; //prescaler 0b10=1/16
    PIE1bits.TMR2IE = 0;
    PR2 = 20;

    //set up timer for interrupt
    T0CONbits.TMR0ON = 1; //enable timer 0
    T0CONbits.T0CS = 0; //select clock (0=internal,1=t0pin)
    T0CONbits.PSA = 1; //disable's prescaler (1=disable, 0=enable)
    T0CONbits.T08BIT = 0; //set mode (1=8bit mode, 0=16bit mode)
    T0CONbits.T0SE = 1; //edge select (1=falling edge, 0=rising edge)
    T0CONbits.T0PS = 0b000; //configure prescaler 000=1:2

    //Set up timer0 interrupts
    INTCONbits.TMR0IE = 1;
    INTCONbits.TMR0IF = 0;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;


    //set up USB serial port
    TRISCbits.TRISC6 = 1;
    RCSTA1bits.SPEN = 1;
    TXSTA1bits.TXEN = 1;

    TXSTA1bits.SYNC = 0;
    BAUDCON1bits.BRG16 = 0;
    TXSTA1bits.BRGH = 1;

    // 19.2kbaud = 001, 832
    SPBRG1 = 34;
    
    //9.6kbaud = 000, 103
    RCSTA1bits.CREN = SET;
}

////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                            Sender Code                                 ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
void dumpNrf(void) {
    int i;
    char val;
    for (int i=0; i<=0x1d; i++) {
        val = nrf_readRegister(i);
        sendHex(i);
        sendLiteralBytes("\t");
        sendHex(val);
        sendLiteralBytes("\n");
    }
}

void run(void) {
    int i;
    short nextSlot;
    char status;

    nrf_init();
    delay();

    nrf_rxmode();
    delay();

    //nrf_setTxAddr(0);
    //nrf_setRxAddr(0,0);

    sendLiteralBytes("Receiver!\n");

    dumpNrf();

    i=0;
    while(1) {
        LED_RED++;
        sendIntDec(nrf_getStatus());
        sendLiteralBytes("\n");

        LED_GREEN = nrf_receive(&tx_buf,&rx_buf);
        delay();
    }
}

void runSend(void) {
    nrf_init();
    delay();

    nrf_txmode();
    delay();

    //nrf_setTxAddr(0);
    //nrf_setRxAddr(0,0);

    sendLiteralBytes("Sender!\n");

    dumpNrf();

    tx_buf[0] = 42;
    while(1) {
        LED_RED++;
        LED_GREEN = nrf_send(&tx_buf,&rx_buf);
        sendIntDec(nrf_getStatus());
        sendLiteralBytes("\n");
        delay();
    }
}

void interruptService(void) {

}

////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                            Shared Code                                 ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////

void delay(void) {
    Delay10KTCYx(254);
}

////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                            System Code                                 ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////


void main(void) {
    setup();

    if (MODE_SELECT == MODE_SEND) {
        runSend();
    } else {
        run();
    }

    while(1);
}

void interrupt interrupt_high(void) {
    interruptService();

    INTCONbits.TMR0IF = CLEAR;
}
