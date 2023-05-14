// LD2410
// rev 1 - shabaz - May 2023


#define RX_NULL 0
#define RX_HDR_1 1
#define RX_HDR_2 2
#define RX_HDR_3 3
#define RX_DATA 4
#define TIMEOUT_MSEC 1000
#define FOREVER 1
#define DELAY_MS delay
#define PRESENCE_NONE 0
#define PRESENCE_MOVING 1
#define PRESENCE_STATIONARY 2
#define PRESENCE_COMBINED 3

typedef struct meas_s {
    uint8_t valid;
    uint8_t state;
    uint8_t moving_energy;
    uint8_t stationary_energy;
    uint16_t moving_distance;
    uint16_t stationary_distance;
    uint16_t detection_distance;
} meas_t;

int rxstate = RX_NULL;
meas_t report;


void setup() {
    // start serial port for console at 115200 baud:
    Serial.begin(115200);
    while (!Serial) {
        // wait for serial port to connect. Needed for native USB port only
    }
    // start serial port for LD2410 at 256kbps:
     Serial1.begin(256000);
}

int read_frame(void) {
    uint8_t inByte;
    int data_valid = 0;
    int not_finished = 1;
    unsigned long tstop;
    uint8_t framedata[23];
    char pbuf[47];
    tstop = millis() + TIMEOUT_MSEC;
    while (not_finished) {
        if (millis() > tstop) {
            // timeout
            break;
        }
        if (Serial1.available() == 0)
            continue;
        inByte = (uint8_t)Serial1.read();

        switch (rxstate) {
            case RX_NULL:
                report.valid = 0;
                framedata[0] = 0;
                if (inByte == 0xf4) {
                    rxstate = RX_HDR_1;
                } else {
                    rxstate = RX_NULL;
                }
                break;
            case RX_HDR_1:
                if (inByte == 0xf3) {
                    rxstate = RX_HDR_2;
                } else {
                    rxstate = RX_NULL;
                }
                break;
            case RX_HDR_2:
                if (inByte == 0xf2) {
                    rxstate = RX_HDR_3;
                } else {
                    rxstate = RX_NULL;
                }
                break;
            case RX_HDR_3:
                if (inByte == 0xf1) {
                    rxstate = RX_DATA;
                    framedata[0] = 0xf4;
                    framedata[1] = 0xf3;
                    framedata[2] = 0xf2;
                    framedata[3] = 0xf1;
                } else {
                    rxstate = RX_NULL;
                }
                break;
            default:
                if (rxstate < 23) {
                    framedata[rxstate] = inByte;
                    rxstate++;
                } else {
                    rxstate = RX_NULL;
                    if ((framedata[7] != 0xaa) || (framedata[17] != 0x55) ||
                        (framedata[21] != 0xf6) || (framedata[22] != 0xf5)) {
                        // error, unexpected data!
                    } else {
                        // decode the data
                        report.state = framedata[8];
                        report.moving_energy = framedata[11];
                        report.stationary_energy = framedata[14];
                        report.moving_distance = framedata[9] | (framedata[10]<<8);
                        report.stationary_distance = framedata[12] | (framedata[13]<<8);
                        report.detection_distance = framedata[15] | (framedata[16]<<8);
                        // sanity check
                        if ((report.moving_distance > 800) ||
                            (report.stationary_distance > 800) ||
                            (report.detection_distance > 800) ||
                            (report.moving_energy > 100) ||
                            (report.stationary_energy > 100)) {
                            // the data has some errors!
                        } else {
                            report.valid = 1;
                            data_valid = 1;
                            not_finished = 0;
                        }
                    }
                }
                break;
        } // end switch
    } // end while
    if (data_valid==0) {
        return(-1);
    }
    return(0);
}

void loop() {
    int ret=0;
    DELAY_MS(500);
    if (read_frame()==0) {
        Serial.print("State: ");
        Serial.print(report.state);
        Serial.print(" Moving energy: ");
        Serial.print(report.moving_energy);
        Serial.print(" Stationary energy: ");
        Serial.print(report.stationary_energy);
        Serial.print(" Moving distance: ");
        Serial.print(report.moving_distance);
        Serial.print(" Stationary distance: ");
        Serial.print(report.stationary_distance);
        Serial.print(" Detection distance: ");
        Serial.println(report.detection_distance);
    } else {
        Serial.println("No data received");
    }
}
