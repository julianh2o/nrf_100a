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

#define PROBE_TRIS TRISAbits.TRISA1
#define PROBE PORTAbits.RA1

#define STATUS_TRIS TRISBbits.TRISB4
#define STATUS_LED PORTBbits.RB4

#define BUTTON_TRIS TRISBbits.TRISB1
#define BUTTON PORTBbits.RB1

#define MODE_SELECT_TRIS TRISBbits.TRISB0
#define MODE_SELECT PORTBbits.RB0
#define MODE_SEND 1

unsigned char tx_buf[MAX_PAYLOAD];
unsigned char rx_buf[MAX_PAYLOAD];

void setup(void);

void run(void);
void interruptHandler(void);

////                            Shared Code                                 ////
void delay(void);

////                            System Code                                 ////

void main(void);
void interrupt interrupt_high(void);

void setup(void) {
    //Misc config
    STRIP_DATA_TRIS = OUTPUT;
    STATUS_TRIS = OUTPUT;
    BUTTON_TRIS = INPUT;
    MODE_SELECT_TRIS = INPUT;
    STATUS_LED = 0;
    LED_GREEN_TRIS = OUTPUT;
    LED_RED_TRIS = OUTPUT;
    PROBE_TRIS = OUTPUT;

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
    INTCONbits.GIE = 0;


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
    //SPBRG1 = 103;
    RCSTA1bits.CREN = SET;
}

////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                            Main   Code                                 ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
#define MAX_NETWORK_SIZE 10
char clients[MAX_NETWORK_SIZE];
char client_count;
char turn;

void flashGreen() {
    while(1) {
        LED_GREEN = !LED_GREEN;
        delay();
    }
}

void flashRed() {
    while(1) {
        LED_RED = !LED_RED;
        delay();
    }
}

void flashAlternate() {
    while(1) {
        LED_RED = !LED_RED;
        LED_GREEN = !LED_RED;
        delay();
    }
}

void flashBoth() {
    while(1) {
        LED_RED = !LED_RED;
        LED_GREEN = LED_RED;
        delay();
    }
}

char seekExistingNetwork(void) {
    unsigned long i = 0;
    
    nrf_rxmode();
    delay();

    while(i++ < 100) {
        LED_GREEN = !LED_GREEN;
        if (nrf_receive(&tx_buf, &rx_buf)) {
            LED_GREEN = 0;
            return 1;
        }
        Delay10KTCYx(20);
    }
    return 0;
}

int gatherNetworkData(char * arr) {
    int i;
    while(!nrf_receive(&tx_buf, &rx_buf));

    for (i=0; i<rx_buf[1]; i++) {
        arr[i] = rx_buf[2+i];
    }
    return rx_buf[1];
}

int assignId(char * arr, int len) {
    int i;
    int max = 0;
    for (i=0; i<len; i++) {
        if (arr[i] > max) max = arr[i];
    }
    return max+1;
}

int findClientId(char * arr, int len, int seek) {
    int i;
    for (i=0; i<len; i++) {
        if (arr[i] == seek) return i;
    }
    return -1;
}

void participateInNetwork(char id) {
}

void run(void) {
    nrf_init();
    delay();

    nrf_setTxAddr(0);
    nrf_setRxAddr(0, 0);

    LED_GREEN = 1;
    delay();
    LED_GREEN = 0;

    unsigned int id = 1;
    client_count = 1;
    clients[0] = id;

    sendLiteralBytes("Seeking network..\n");
    if (seekExistingNetwork()) {
        sendLiteralBytes("Found existing network!\n");
        client_count = gatherNetworkData(&clients);

        id = assignId(&clients,client_count);
        clients[client_count++] = id;

        sendLiteralBytes("Assuming ID: ");
        sendDec(id);
        sendLiteralBytes("\n");

        sendCharArray(clients,client_count);
        sendLiteralBytes("\n");

        flashRed(); //TODO this causes us to freeze when we detect an existing network
    }

    sendLiteralBytes("Entering main loop\n");

    while(1) {
        tx_buf[0] = id;
        tx_buf[1] = client_count;
        for (int i=0; i<client_count; i++) {
            tx_buf[2+i] = clients[i];
        }
        LED_RED = nrf_send(&tx_buf,&rx_buf);
    }
    //TODO write the round robin turn-based interaction code
//
//    nrf_rxmode();
//    Delay10KTCYx(20);
//
//    while(!nrf_receive(&tx_buf, &rx_buf)) { //wait for next packet
//        turn = findClientId(&clients, client_count, rx_buf[0]);
//    }
//    turn++;
//
//    INTCONbits.GIE = 1;
//    flashGreen();
//    while(1) {
//
//    }
}

void interruptHandler() {
//    if (clients[turn] == id) { //its my turn, switch to TX mode and transmit
//        nrf_txmode();
//        Delay10KTCYx(20);
//
//        tx_buf[0] = id;
//        tx_buf[1] = client_count;
//        for (int i=0; i<client_count; i++) {
//            tx_buf[2+i] = clients[i];
//        }
//
//        nrf_send(&tx_buf, &rx_buf);
//    } else {
//        Delay10KTCYx(30);
//        nrf_receive(&tx_buf, &rx_buf);
//    }
//
//    if (clients[turn+1] == id) { //my turn is next!
//        nrf_txmode();
//        Delay10KTCYx(20);
//    }
//
//    turn++;
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

    run();

    while(1);
}

void interrupt interrupt_high(void) {
    interruptHandler();

    INTCONbits.TMR0IF = CLEAR;
}
