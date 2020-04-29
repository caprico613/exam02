
#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7
#define threshold 0.7071

I2C i2c( PTD9,PTD8);
Serial pc(USBTX, USBRX);
DigitalOut led1(LED1);
InterruptIn sw2(SW2);
InterruptIn sw3(SW3);
EventQueue queue1(32 * EVENTS_EVENT_SIZE);
EventQueue queue2(32 * EVENTS_EVENT_SIZE);
Thread thread1;
Thread thread2;

int m_addr = FXOS8700CQ_SLAVE_ADDR1;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);
void led1_thread();
void logger_thread();
float logging[400];
float t[3];

int main() {

    pc.baud(115200);
    led1 = 1;
    thread1.start(callback(&queue1, &EventQueue::dispatch_forever));
    thread2.start(callback(&queue2, &EventQueue::dispatch_forever));
    uint8_t who_am_i, data[2], res[6];
    int16_t acc16;
    
    // Enable the FXOS8700Q

    FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);
    data[1] |= 0x01;
    data[0] = FXOS8700Q_CTRL_REG1;
    FXOS8700CQ_writeRegs(data, 2);

    // Get the slave address
    FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);
    // pc.printf("Here is %x\r\n", who_am_i);

    sw2.rise(queue1.event(led1_thread));

    while (true) {

        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[1] = ((float)acc16) / 4096.0f;

        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
             acc16 -= UINT14_MAX;
        t[2] = ((float)acc16) / 4096.0f;

        /*
        if (t[2] < threshold) {
            if (t[0] > threshold || t[0] < -threshold) {
                printf("FB: Tilt over 45 degree.\n");   // Forward and backward
                printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)\r\n",\
                    t[0], res[0], res[1],\
                    t[1], res[2], res[3],\
                    t[2], res[4], res[5]\
                );
            }
            if (t[1] > threshold || t[1] < -threshold) {
                printf("LR: Tilt over 45 degree.\n");   // Left and right
                printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)\r\n",\
                    t[0], res[0], res[1],\
                    t[1], res[2], res[3],\
                    t[2], res[4], res[5]\
                );
            }
        }
        */
        wait(0.1);
   }
}


void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}


void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}

void led1_thread() {
    queue2.call(logger_thread);
    // printf("LED1\r\n");
    for (int i = 0; i < 10; i++) {
        led1 = !led1;
        wait(0.5);
        led1 = !led1;
        wait(0.5);
    }
}

void logger_thread() {
    // printf("Logger\r\n");
    for (int i = 0; i < 300; i+=3) {
        logging[i] = t[0];
        logging[i + 1] = t[1];
        logging[i + 2] = t[2];
        wait(0.1);
    }
    for (int i = 0; i < 300; i++){
        // printf("FXOS8700Q ACC: X=%1.4f, Y=%1.4f, Z=%1.4f\r\n", logging[i], logging[i + 1], logging[i + 2]);
        // if (logging[i + 3] == 1)    printf("Tilted\r\n");
        // else printf("Not Tilted\r\n");
        pc.printf("%1.4f\r\n", logging[i]);
        wait(0.01);
    }
}
