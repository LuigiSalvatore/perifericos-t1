#include <Arduino.h>

void setup()
{
    Serial.begin(9600);
}

void loop()
{

    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "T")
        {
            float temp = 25.5;
            Serial.println("T:" + String(temp));
        }

        else if (cmd == "H")
        {
            int hum = 60;
            Serial.println("H:" + String(hum));
        }

        else if (cmd == "TH")
        {
            float temp = 25.5;
            int hum = 60;
            Serial.println("TH:" + String(temp) + "," + String(hum));
        }

        else
        {
            Serial.println("ERR");
        }
    }
}