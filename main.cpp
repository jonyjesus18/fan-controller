#include "mbed.h"
#include "helper_funcs.h"
#include <cstdio>
#include "TextLCD.h"

TextLCD lcd(PB_15, PB_14,PB_10,PA_8,PB_2,PB_1); // RS, E, D4-D7

DigitalOut pwm(PB_0); //PWM output pin 
DigitalOut led_green(LED1); //Small green LED in the nucleo board
DigitalOut eb_ledB(PA_15); // B Led in the extension Board
DigitalOut eb_ledA(PB_7); // A Led in the extension Board
InterruptIn on_off(BUTTON1); // Blue button int the nucleo board.
InterruptIn fan_tacho (PA_0); // Tachometer input from the fan.
InterruptIn rot_a (PA_1); // Rotary encoder pin a input
InterruptIn rot_b (PA_4); // Rotary encoder pin b input

int system_enable = 1; // Main loop enabler.


// Timers & Tickers & Timeouts
Timer t; // Main timer that takes speed samples. Sets the rate at which speed in the fan is calculated.
Timer wave_timer_high; // Times the period between rising edges.
Ticker pid_ticker; // Pid ticker, sets the rate at which the PID is updated.

// Wave period
int revs = 0; // Counter that is incremented on each rising edge.
float period = 0; // Holds the last measured wave period length in ms.
float periods[] = {0,0}; //ensure changes shift left insert.

//Encoder
float counter = 50; // encoder output (0-100) - starting value.
int previous_rot_a = 0; // Initialises rotary encoder a pin position.
int previous_rot_b = 0; // Initialises rotary encoder b pin position.

//PID
float integral_component; // Initialise intergal componennt in PID. This is incremented/decremented at each PID() call.
float error_array[2] = {0,0}; // Initialises the error array going into the PID function. Takes two elements : [old_error, current_error]


// PWM Variables
Ticker period_ticker;  // Sets the PWM period
Timeout pwm_timeout; // Times out the high edge in each PWM Wave period.
double period_s = 0.005; // Defines the PWM period (5ms).
double duty_cycle = 0.4; // Initialises Duty Cycle at 30%.
int start; // Enabled the PWM streching on the rising edge of the tacho signal

void rise_add_rev() { 
    /*
    Increments the revs variable at each rising edge and times the period between edges. 
    Each period is inserted in the periods array.
    */
    start = 1;
    wave_timer_high.stop();
    period = wave_timer_high.elapsed_time().count();
    if (period > 5000){ // Remove periods that are too short to be accurate
        shif_left_insert(periods,2,period);
    }
    period = 0;
    wave_timer_high.reset();
    wave_timer_high.start();
    revs++;
}

void update_counter() {
    /*
    Updates the encoder counter value. By checking the current and old positions of pins A and B in the encoder,
    we determine the relative movement and direction to increment and decrement a counter. Each array indicates a clockwise transition
    in the encoder.

    At each moment we calculate a transition array. If that transition arrat matches one of the predifined transition arrays in the clockwise 
    or counterclockwise direction the counter moves in that direction. 
    */

    // Clockwise transition arrays : 
    int a_c_1[] = {0,0,1,0};
    int a_c_2[] = {1,0,1,1};
    int a_c_3[] = {1,1,0,1};
    int a_c_4[] = {0,1,0,0};

    // Counter Clockwise transition arrays : 

    int a_cc_1[] = {1,0,0,0};
    int a_cc_2[] = {1,1,0,1};
    int a_cc_3[] = {0,1,1,1};
    int a_cc_4[] = {0,0,0,1};



    int transition_array[] = {previous_rot_a,previous_rot_b, rot_a.read(),rot_b.read()}; 
    /*
    Update transition array. 
    If encoder does not move, transition array will be either [0,0,0,0] or [1,1,1,1]. Because these arrays do not match
    any of the predefined arrays, the encoder counter value will not move.
    */
    
    int counter_sensivity = 2; // Sets up the encoder sensitivity. (How much each transition actually increments the counter value)

    previous_rot_a = rot_a.read();
    previous_rot_b = rot_b.read();

    if (compareArrays(transition_array,a_c_1, 3) == 1) {
        counter += counter_sensivity;
    }
    if (compareArrays(transition_array,a_c_2, 3) == 1) {
        counter += counter_sensivity;
    }
    if (compareArrays(transition_array,a_c_3, 3) == 1) {
        counter += counter_sensivity;
    }
    if (compareArrays(transition_array,a_c_4, 3) == 1) {
        counter += counter_sensivity;
    }
    if (compareArrays(transition_array,a_cc_1, 3) == 1) {
        counter -= counter_sensivity;
    }
    if (compareArrays(transition_array,a_cc_2, 3) == 1) {
        counter -= counter_sensivity;
    }
    if (compareArrays(transition_array,a_cc_2, 3) == 1) {
        counter -= counter_sensivity;
    }
    if (compareArrays(transition_array,a_cc_3, 3) == 1) {
        counter -= counter_sensivity;
    }       

    // Ensures counter does not go above 100 or bellow 0.
    if (counter < 0){
        counter = 0;
    } 
    if (counter > 100){
        counter = 100;
    }
}


float pid_output (float current_duty_cycle, float error_array[],float pid_time_delta, float kp, float ki, float kd) {
    /*
    Takes an array of errors, a time delta and the PID constants to compute duty cycle value to be used in the PWM Calculation
    */

    float out; 
    float proportional_component;
    float derivative_component;
    float current_error  = error_array[1];
    float previous_error = error_array[0];
    if (system_enable){
        proportional_component = kp * current_error; // compute proportional component
        if (current_duty_cycle > 0 && current_duty_cycle < 1) { // Ensure PID output does not keep deviating from lower and Upper Boundaries of 0 and 1
            integral_component += current_error * pid_time_delta;                // compute integral component
        }
        derivative_component = (current_error - previous_error)/pid_time_delta;   // compute derivative component
        out = kp*proportional_component + ki*0.001*integral_component + kd*derivative_component;   
    }
    return out; //PID output                 
}


void set_low() {
    // Sets the pwm signal to 0 (low)
    pwm = pwm*0;
}

void pwm_timeout_void() {
    // Sets the pwm signal to 1 and calls the timout function to set the signal to 0 after a given time (period_s*duty_cycle)
    pwm = 1;
    pwm_timeout.attach(&set_low, period_s*duty_cycle);
}

void PID(){
    /*
    Updates the duty cycle using the PID function
    */
    duty_cycle = pid_output(duty_cycle, error_array, 2, 0.015, 0.01, 0.0001);

}

void on_off_flip() { 
    /*funcion gets called everytime the on/off button gets pressed
    Enables/disables main loo[ and flips the LED's in the extension board.
    */
    system_enable = !system_enable;
    eb_ledA =! eb_ledA;
    eb_ledB =! eb_ledB;
    duty_cycle = 0.4;
}


int main() {   
    /*
    Main function calculates speed and changes the output 
    */
    
    // Initialise tickers, timers, timeouts and digitial IO ports.
    pid_ticker.attach(&PID,0.2); //PID update function running every 
    period_ticker.attach(&pwm_timeout_void, period_s);
    on_off.mode(PullUp); // set on/off button to PullUp mode
    on_off.fall(&on_off_flip); // On falling edges (button pressed) , flip system enable variable by calling on_off_flip()
    
    eb_ledA = 1; //Initialise led A
    eb_ledB = 0; //Initialise led B
    
    t.reset(); //Reset and start t timer. Used to time the speed calculations. 
    t.start();

    fan_tacho.rise(rise_add_rev); // On each tacho wave rise, add count a tick.
    double target_rpm; // will hold the calculated target rpm, after the encoder values are mapped.
    int text_clearing_en = 0; // Enables LCD text clearing. This exists to avoid clearing the LCD multiple times on a loop.
    
    while(1) {
        if (system_enable) {// Execute control code of system enabled.

            text_clearing_en = 1; // Enables LCD text clearing for when fan goes on standby.
            update_counter(); // Updates the counter value from the Encoder
            target_rpm = map(counter,0,100,500,1800); // Maps the encoder value ranges to actual rpm targets. A 50 from 0-100 is 1500 from 1000-2000.
            double rotations = 2; // Rotations needed to sample time. The higher the value the more accurate the period readings will be, however the PWM will be High for longer.

            if (target_rpm >= 800) { 
                // Printing target RPM in the LCD display.
                lcd.locate(0,1);
                lcd.printf("TG:%.2f\n",target_rpm);

                /*
                Closed Loop control with PID integration.
                If target rpm is over 800 rpm we sample speed using PWM stretching. 
                See read.me for PWM stretching explanation
                */
                if (t.elapsed_time().count()*0.000001 > 3) { 
                    if(start){
                        revs = 0; // reset counting 
                        while (revs < rotations * 2) { 
                        pwm = 1;
                    }
                }
                start = 0;
                
                // Calculate rpm based on rotation time sampled.
                float rpm = (60)*2/(rotations*(0.000001*(periods[0]+periods[1])));
                float error = target_rpm - rpm;
                shif_left_insert(error_array, 2, error); //insert error measurement into array.


                // Printing values to console
                printf("Current reading (rpm) %f ", rpm);
                printf("Target %f ", target_rpm);
                printf("Current period %f  ",periods[1]);
                printf("PWM / PID : %f  " ,duty_cycle);
                printf("Error : %f" ,error);
                printf("\n");

                // Printing values to external LCD
                lcd.cls();
                lcd.locate(0,0);
                lcd.printf("RM:%.0f\n",rpm);
                lcd.locate(8,0);
                lcd.printf("PID:%.2f\n",duty_cycle);

                // Reset timer
                t.reset(); 
                }
            } 
            else {
                /*
                Open Loop control. Changing speed by controlling value of PWM.
                */   
                duty_cycle = counter/100;
                lcd.locate(0,0);
                lcd.printf("Open Loop Control\n");
                lcd.locate(0,1);
                lcd.printf("Duty Cycle:%.2f\n",duty_cycle);
            }   
        }
        else {
            if (text_clearing_en){ // Check if system has enabled lcd text clearing to avoid clearinf lcd on loop. Better User Interface
                lcd.cls();
                text_clearing_en = 0;
            }
            lcd.locate(0,0); // Print sentence starting on first cell in first row.
            lcd.printf("Press Blue Button to turn on :)");
        }
    }
}

