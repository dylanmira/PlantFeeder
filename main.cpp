#include "mbed.h"
#include "rtos.h"
/*
    Light blinks Green when the pump is driven
    Light is Blue when bluetooth input drives water pump
    Light blinks Purple when moisture level is within Target
*/
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

PwmOut R(p24);
PwmOut G(p25);
PwmOut B(p26);

volatile float Rval = 0.0f;
volatile float Gval = 0.0f;
volatile float Bval = 0.0f;

/*
    Turns on when we want to turn on the relay
    Only when moisture level is low, the driver turns on
    the pump and the dripper turns on
*/
DigitalOut Driver(p21);


/* 
    Send the characters ON to the module and the dirver manually
    goes HIGH. State Pin triggers an interrupt that wakes the mbed
*/
InterruptIn blueOn(p22);

#if   defined(TARGET_LPC1768)
Serial bluetooth(p13, p14);         // TX, RX
#elif defined(TARGET_LPC4330_M4)
Serial bluetooth(P2_3, P2_4);         // UART3_TX, UART3_RX
#endif

/*
    Reads the moisture level once an hour and averages out 
    5 samples taken. Then, based off of those values a driver may
    turn on. If driver turns on, the moisture level will be read again
    soon after.
*/
AnalogIn moistureVal(p15);
volatile float currentSoilVal = 0.0f;

/*
    Booleans that desribe current state
*/
volatile bool waterOn = false;
volatile bool drySoil = true;
volatile bool goodSoil = false;
volatile bool deepSleep = false;


void waterPlant() {
    if (waterOn == 0) {
        //Does not run if auto-system is currently watering Plant
        Driver.write(1);
        wait(7000);
        Driver.write(0);
    }
}
/*
    Tells what voltage value was just read in on the moisture sensor.

*/
void infoForSoilVal(void const* args) {
    while(!deepSleep) {
        if ( 1.0f/3.3f < currentSoilVal & currentSoilVal <= 1.8f/3.3f)
        {
            led1.write(1);
        } 
        else if ( 1.8f/3.3f < currentSoilVal & currentSoilVal <= 2.2f/3.3f)
        {
            led2.write(1);
        } 
        else if (2.2f/3.3f < currentSoilVal & currentSoilVal <= 3.0f/3.3f)
        {
            led3.write(1);
        }
        else 
        {
            led4.write(1);
        }
        
        Thread::wait(2000);
        
        led1.write(0);
        led2.write(0);
        led3.write(0);
        led4.write(0);
        
        Thread::wait(2000);
    }
}

void ledStateMachine(void const* args) {
    while (!deepSleep) {
       R.write(Rval);
       G.write(Gval);
       B.write(Bval);
       
       Thread::wait(500);
       
       R.write(0.0f);
       G.write(0.0f);
       B.write(0.0f);  
       
       Thread::wait(500);   
    }
}

int main() {
    Thread::wait(4000);
    blueOn.rise(&waterPlant);
    Driver = 0;
    float soilMoistAverage = 0.0f;
    while(1) {
        deepSleep = 0;
        Thread LEDThread(&ledStateMachine);
        Thread DebugThread(&infoForSoilVal);
        
        Rval = 1.0f;
        Gval = 1.0f;
        Bval = 1.0f;
        
        soilMoistAverage = 0.0f;
        
        for (int i = 0; i < 5; i++) {
            currentSoilVal = moistureVal.read();
            soilMoistAverage = currentSoilVal + soilMoistAverage;
            Thread::wait(50);
        }
        currentSoilVal = soilMoistAverage / 5.0f;
        
        if (currentSoilVal > 2.20f/3.3f) 
        {
            drySoil = 1;
        }
        
        while(drySoil) {
            if (currentSoilVal > 2.20f/3.3f) {
                drySoil = 1;
                waterOn = 1;
                goodSoil = 0;
                
                Rval = 0.0f;
                Gval = 1.0f;
                Bval = 0.0f;

                
                Driver.write(1);
                Thread::wait(2000);
                Driver.write(0);
                
                Thread::wait(150000); 
                
                soilMoistAverage = 0.0f;
                
                for (int i = 0; i < 5; i++) {
                    currentSoilVal = moistureVal.read();
                    soilMoistAverage = currentSoilVal + soilMoistAverage;
                    Thread::wait(50);
                }
                currentSoilVal = soilMoistAverage / 5.0f;
                
            } else {
                drySoil = 0;
                waterOn = 0;
                goodSoil = 1;
                
                Rval = 0.5f;
                Gval = 0.2f;
                Bval = 1.0f;
                
                Thread::wait(50000);
            }  
        }
        Thread::wait(50000);
        deepSleep = 1;
        Thread::wait(1000);
        
        R.write(1.0f);
        G.write(0.0f);
        B.write(0.0f);  
        
        for (int i = 0; i < 180; i++)
        {
            Thread::wait(10000);
        }
    }
}
