#ifndef DHT_H
#define DHT_H

#define MAXTIMINGS 85

#define cli noInterrupts
#define sei interrupts

#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21

#define NAN 999999

class DHT {
    private:
        uint8_t data[6];
        uint8_t _pin, _type, _count;
        bool read(void);
        unsigned long _lastreadtime;
        bool firstreading;

    public:
        DHT(uint8_t pin, uint8_t type, uint8_t count=6);
        void begin(void);
        float readTemperature(bool S=false);
        float convertCtoF(float);
        float readHumidity(void);

};

#endif // DHT_H