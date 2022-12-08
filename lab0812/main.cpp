#include "mbed.h"
#include <cstdio>
// #include "TextLCD.h"

// TextLCD lcd(PA_9, PB_10, PB_4, PB_5, PB_3, PA_8,TextLCD::LCD16x2); // RS, E, D4-D7
// TextLCD lcd(PB_4, PB_5,PA_3,PA_2,PA_10,PB_3); // RS, E, D4-D7



PwmOut pwm(PB_0);
InterruptIn fan_tacho (PA_0);
InterruptIn rot_a (PA_1);
InterruptIn rot_b (PA_4);
Timer t;
Timer wave_high;
Timer wave_low;
int enable_flag = 1;
// int current_time = 0;
// int print_current_time = 0;
// int time_delta = 1000;
// int old_fan_tacho = fan_tacho;
int revs = 0;

// void wave_rise() {
//     wave_low.stop();
//     wave_high.reset();
//     wave_high.start();

//     if (enable_flag && wave_low.elapsed_time().count() > 10) { // 2_ if enabled by rise, check if low time was long anough to not count as error
//         revs ++ ;
//     }
//     enable_flag = 0;
// }

// void wave_fall() {
//     wave_high.stop(), 
//     wave_low.reset(); 
//     wave_low.start();

//     if (wave_high.elapsed_time().count() > 100) {
//         revs ++;
//         enable_flag = 1; // 1) normal rise time, check for low period to see if makes sense
//     }
// }


void wave_rise() {
    wave_high.reset();
    wave_high.start();
}

void wave_fall() {
    wave_high.stop();
    if (wave_high.elapsed_time().count() > 15000) {
        revs ++;
    }
}

float error_delta;


float pid(float target, float reading, float kp,float ki,float kd){
    float proportional;
    error_delta  = target - reading;
    // Proportional
    proportional = error_delta * kp;

    return proportional;
    // 
}


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

float counter = 0; // encoder output

int previous_rot_a = 0;
int previous_rot_b = 0;

int compareArrays(int a[], int b[], int n) {
  int ii;
  for(ii = 0; ii <= n; ii++) {
    if (a[ii] != b[ii]) return 0;
  }
  return 1;
}

int map(float val,
           float min_range_1,
           float max_range_1,
           float min_range_2,
           float max_range_2) { 
               float a = (val - min_range_1) / (max_range_1 - min_range_1);
               float map = min_range_2 + a * (max_range_2 - min_range_2);
            return map;
           }

void shif_left_insert (float shifted_array[], int size, float insert) {
    if(size == 1){
        //do nothing
    }
    else{
        float temp;
        for (int i=0; i < size; i++) {
            shifted_array[i] = shifted_array[i + 1];//myarray[0] == myarray[1]
        }
    }
    shifted_array[size-1] = insert;
}

float array_average(float a[], int size) {
    float summer;
    for (int i=0; i < size; i++) {
        summer += a[i];
    }
    return summer/size;
    // return (summer / size_array);
}

void update_counter() {
    int transition_array[] = {previous_rot_a,previous_rot_b, rot_a.read(),rot_b.read()};

    previous_rot_a = rot_a.read();
    previous_rot_b = rot_b.read();
    
    int counter_sensivity = 1;

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

    if (counter < 0){
        counter = 0;
    } 
    if (counter > 100){
        counter = 100;
    }
}


int main() 
{ 
    t.reset();
    t.start();
    fan_tacho.rise(wave_rise);
    fan_tacho.fall(wave_fall);

    float times_array[4] = {0,0,0,0};
    
    int current_time = t.elapsed_time().count();
    int print_current_time = t.elapsed_time().count();
    int time_delta = 200000;
    int target_rpm;

    int old_fan_tacho = 0;
    // Timer tt;
    while(1) {  
        // tt.reset();
        // tt.start();
        // if (fan_tacho.read() != old_fan_tacho) {
        //     printf("%i", fan_tacho.read());
        //     printf("\n");
        //     printf("%llu",  tt.elapsed_time().count());
        //     printf("\n");
        //     old_fan_tacho = fan_tacho.read();

        // }

        float duty_cycle = counter/100;
        pwm.write(duty_cycle);
        update_counter();
        current_time = t.elapsed_time().count();
        target_rpm = map(counter,0,100,0,2500);
           

            if (revs == 8) { // Inner cycle to print things at slower pace (for visibility)
            printf("Duty cycle : %f" ,duty_cycle);
            printf("   Revs : %d" ,revs);
            shif_left_insert(times_array,4,t.elapsed_time().count());
            printf("   The time taken was %f milliseconds", times_array[0]);
            printf("   Smoothed timings, n = 4: %f", array_average(times_array,4));
            float rpm = 15/(0.000001*array_average(times_array,4));
            printf("   Smoothed speed (ms): %f" , rpm);

            // printf("  t0 %f", times_array[0]);
            // printf("  t1 %f", times_array[1]);
            // printf("  t2 %f", times_array[2]);
            // printf("  t3 %f", times_array[3]);

            printf("\n");

            revs = 0;
            t.reset();
            
        }
    }
}
 