// This #include statement was automatically added by the Particle IDE.
#include "DHT.h"


#include "SFE_TSL2561.h"

#define PUBLISH_TOPIC   "HKSValues"

#define DELAY_LOOP      5000
#define DELAY_PUBLISH   500
#define MAX_DATA        63

#define DHTPIN          D4
#define DHTTYPE         DHT22

#define LIGHT_PIN       A6
#define LED_PIN         D7

#define R   A4
#define G   A5
#define B   A7


// TSL2561 Object
SFE_TSL2561 tsl = SFE_TSL2561();
// DHT Object
DHT dht(DHTPIN, DHTTYPE);

// TSL Variables
unsigned char id;
bool gain;
unsigned int ms, data1, data2;

// LUX Value
double currentLux;
// Temperature
float currentTemperature;
// Pressure
float currentHumidity;
// Brightness
int brightness = 100;
// Current Hue
int hue;
// Light is on or off
bool isOn;

// Forward declarations
int internalSetHue(int hue);

// Publish sensor value
void publish(char* name, double value){
    char szData[MAX_DATA];
    
    sprintf(szData, "%s=%2.02f", name, value);
    
    Particle.publish(PUBLISH_TOPIC, szData);
}

// Particle Function to control light
int ctrlLight(String args){
    int onoff = args.toInt();
    
    isOn = (1 == onoff);
    
    internalSetHue(hue);
    
    return onoff;
}

// Set brightness of LED
int setBrightness(String args){
    brightness = args.toInt();
    
    internalSetHue(hue);
    
    return brightness;
}

// Set Hue
int internalSetHue(int hue){
    if(!isOn){
        analogWrite(R, 0);
        analogWrite(G, 0);
        analogWrite(B, 0);
    
        return 0;
    }
    
    float h1 = (float)hue / 360.0;
    float h2 = h1 * 255;
    
    Serial.println();
    Serial.print(hue);
    Serial.print(",");
    Serial.print(h1);
    Serial.print(",");
    Serial.print(h2);
    Serial.println();
    
    int h = (int)h2;
    int s = 255;
    int v = (brightness * 255) / 100;
    
    unsigned char r,g,b;
    
    hsvtorgb(&r, &g, &b, h, s, v);
    
    Serial.println();
    Serial.print(h);
    Serial.print(",");
    Serial.print(s);
    Serial.print(",");
    Serial.print(v);
    Serial.println();
    
    Serial.println();
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println();
    
    analogWrite(R, r);
    analogWrite(G, g);
    analogWrite(B, b);
    
    return 0;
}

// Set Hue
int setHue(String args){
    hue = args.toInt();
    
    return internalSetHue(hue);
}

void hsvtorgb(unsigned char *r, unsigned char *g, unsigned char *b, unsigned char h, unsigned char s, unsigned char v)
{
    unsigned char region, fpart, p, q, t;
    
    if(s == 0) {
        /* color is grayscale */
        *r = *g = *b = v;
        return;
    }
    
    /* make hue 0-5 */
    region = h / 43;
    /* find remainder part, make it from 0-255 */
    fpart = (h - (region * 43)) * 6;
    
    /* calculate temp vars, doing integer multiplication */
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * fpart) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8;
        
    /* assign temp vars based on color cone region */
    switch(region) {
        case 0:
            *r = v; *g = t; *b = p; break;
        case 1:
            *r = q; *g = v; *b = p; break;
        case 2:
            *r = p; *g = v; *b = t; break;
        case 3:
            *r = p; *g = q; *b = v; break;
        case 4:
            *r = t; *g = p; *b = v; break;
        default:
            *r = v; *g = p; *b = q; break;
    }
    
    return;
}

void setup() {
    // Begin Serial
    Serial.begin(115200);
    
    // Set Light and LED pin mode
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    
    // Set RGB LED Pin mode
    pinMode(R, OUTPUT);
    pinMode(G, OUTPUT);
    pinMode(B, OUTPUT);
    
    // Initialize TSL2561
    tsl.begin();
    tsl.getID(id);
    tsl.setTiming(0, 2, ms);
    tsl.setPowerUp();
    
    // Initialize DHT
    dht.begin();
    
    // Particle Function
    Particle.function("ctrllight", ctrlLight);
    Particle.function("brightness", setBrightness);
    Particle.function("sethue", setHue);
}

void loop() {
    // Read data1 and data2
    tsl.getData(data1, data2);
	
	// Read Lux
	bool isGood = tsl.getLux(gain,ms,data1,data2,currentLux);
	
	// If we have good readings
	if(!isGood){
	    Serial.println("Lux reading not reliable.");
    }
    
    currentTemperature = dht.readTemperature();
    currentHumidity = dht.readHumidity();
    
    publish("Lux", currentLux);
    delay(DELAY_PUBLISH);
    publish("Temperature", currentTemperature);
    delay(DELAY_PUBLISH);
    publish("Humidity", currentHumidity);
    
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);

    delay(DELAY_LOOP);
}