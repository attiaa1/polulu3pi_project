#include <stdint.h>
#include <stdio.h> 
#include <util/delay.h>
#include <avr/io.h>
#include "lcd_driver.h"
#include "port_macros.h"
#include <stdbool.h>
// ask about office hours

// defs of buttons based on schematics
#define btnA 1
#define btnB 4
#define btnC 5

// defs of motors based on schematics
#define motor1 6
#define motor2 5
#define motor3 3
#define motor4 3

// defs of motor pins for forward and reverse
#define motor1F OCR0B
#define motor1R OCR0A
#define motor2F OCR2B
#define motor2R OCR2A

// defs of speed // play around with 
#define _FAST	0x7F 
#define _MEDIUM	0x3F 
#define _SLOW	0x0F 

#define _CW	    0x01
#define _CCW	0x02
#define _F		0x03
#define _R		0x04

#define PWM_TOP 100

uint8_t movement_incr, speed_incr;
double time_incr; // figure this out later bc time is in int of 0.1 (needs to be float)
_Bool CW_Turn, Fwd;


_Bool buttonPress(int btnNum);

// GUI 
void movementChoiceUp();
void movementChoiceDown();
void speedChoiceUp();
void speedChoiceDown();
void timeChoiceUp();
void timeChoiceDown();

// Global vars for CW / Fwd

// Actual movement
void pwm_speed(uint8_t dutycycle);
void turn(uint8_t m1_speed, uint8_t m2_speed, _Bool CW_Turn);
void move(uint8_t m1_speed, uint8_t m2_speed, _Bool Fwd);
void stop(); 

// Struct to store commands so we can loop through our commands. 
typedef struct param{
	
    uint8_t direction;
    uint8_t speed;
	double time;

}param;

param get_param();

int main(){

    //initializing/turning on display
    initialize_LCD_driver();
    LCD_execute_command(TURN_ON_DISPLAY);

    // setting up buttons as inputs
    DDRB &= ~((1<<btnA)|(1<<btnB)|(1<<btnC)); 
    PORTB |= ((1<<btnA)|(1<<btnB)|(1<<btnC));
    //from lab 7 pwm
    //uint8_t pwm_counter=0;
    // setting up motors port and pins
    // 1,2,4 based on schematic need a seperate DDR compared to 3, a B instead of D
    DDRD |= ((1<<motor1)|(1<<motor2)|(1<<motor4));
    PORTD &= ~((1<<motor1)|(1<<motor2)|(1<<motor4));
    
    DDRB |= (1<<motor3);
    PORTB &= ~(1<<motor3);

     // initializing motors
    
    /*
    TCCR0A = 0xF3;
    TCCR2A = 0xF3;
	TCCR0B = 0x02;
    TCCR2B = 0x02;
    */

    param _cmnds[4];
    int num_cmnds;
   
    while(1){

        for(num_cmnds = 0; num_cmnds < 4; num_cmnds++){

            _cmnds[num_cmnds] = get_param(); // returns struct param with sequences of instructions
            
            if(_cmnds[num_cmnds].direction == _CW) CW_Turn = true;
            if(_cmnds[num_cmnds].direction == _CCW) CW_Turn = false;
            if(_cmnds[num_cmnds].direction == _F) Fwd = true;
            if(_cmnds[num_cmnds].direction == _R) Fwd = false;

            if((_cmnds[num_cmnds].direction == _CW)||(_cmnds[num_cmnds].direction == _CCW)){
                
                turn(_cmnds[num_cmnds].speed, _cmnds[num_cmnds].speed, CW_Turn);
                //_delay_ms(_cmnds[num_cmnds].time * DELAYFACTOR);
                stop();

            }

            if((_cmnds[num_cmnds].direction == _F)||(_cmnds[num_cmnds].direction == _R)){
                
                move(_cmnds[num_cmnds].speed, _cmnds[num_cmnds].speed, Fwd);
                //_delay_ms(_cmnds[num_cmnds].time * DELAYFACTOR);
                stop();

            }   

        }
        
    }

    return 0;


}

_Bool buttonPress(int btnNum){

    return ( (PINB |(~(1<<btnNum)) ) == ( ~(1<<btnNum)) ); // will return a 1 if pressed and a 0 if not 

}

void movementChoiceUp(){
    LCD_execute_command(CLEAR_DISPLAY);
    // default choice is CW
    if((movement_incr < 4)) movement_incr++;
    else if((movement_incr = 4)) LCD_print_String("Err"); // in case they scroll too far
    // 0 move incr => 1 => CW
    // 1 move incr => 2 => CWW
    // 2 move incr => 3 => F
    // 3 move incr => 4 => R where reverse will be a 180° turn followed by F but we will sort that out in movement
    if(movement_incr == 1) LCD_print_String("CW");
    if(movement_incr == 2) LCD_print_String("CCW");
    if(movement_incr == 3) LCD_print_String("F");
    if(movement_incr == 4) LCD_print_String("R");

}

void movementChoiceDown(){
    LCD_execute_command(CLEAR_DISPLAY);

    if(movement_incr >= 1) movement_incr--;
    
    if(movement_incr == 0) LCD_print_String("Err");
    if(movement_incr == 1) LCD_print_String("CW");
    if(movement_incr == 2) LCD_print_String("CCW");
    if(movement_incr == 3) LCD_print_String("F");
    if(movement_incr == 4) LCD_print_String("R");
}

void speedChoiceUp(){
    LCD_execute_command(CLEAR_DISPLAY);

    if((speed_incr < 3)) speed_incr++;
    else if((speed_incr = 3)) LCD_print_String("Err");
    
    if(speed_incr == 1) LCD_print_String("S");
    if(speed_incr == 2) LCD_print_String("M");
    if(speed_incr == 3) LCD_print_String("F");
}

void speedChoiceDown(){
    LCD_execute_command(CLEAR_DISPLAY);

    if((speed_incr >=1)) speed_incr--;

    if(speed_incr == 0) LCD_print_String("Err");
    if(speed_incr == 1) LCD_print_String("S");
    if(speed_incr == 2) LCD_print_String("M");
    if(speed_incr == 3) LCD_print_String("F");
}

void timeChoiceUp(){
    LCD_execute_command(CLEAR_DISPLAY);
    // see if we can convert float into string with built in C library //changed to double
    char tString[50];
    time_incr += 0.1;
    
    sprintf(tString, "%.1f", time_incr);
    //getchar();

    LCD_print_String(tString); 
}

void timeChoiceDown(){
    LCD_execute_command(CLEAR_DISPLAY);

    char tString[50];
    time_incr -= 0.1; // need to make conditional statement here in case of error

    if(time_incr <= 0) LCD_print_String("Err");

    else{
        sprintf(tString, "%.1f", time_incr);
        //getchar();

        LCD_print_String(tString);
    }

}
/*
// put pwm_speed inside of movement so inside move() and turn(), then figure out how long to activate it for with timing.
void pwm_speed(uint8_t duty_cycle){ //maybe use 2 duty cycles.
    //from lab 7 pwm
     pwm_counter += 1;
        if( pwm_counter >= PWM_TOP ){
        pwm_counter = 0;
        }
    if(pwm_counter < duty_cycle){

        PORTD &= ~(1<<motor2);
        PORTD |= ~(1<<motor1);

        PORTD &= ~(1<<motor4);
        PORTB |= (1<<motor3);

    }
    else{

        PORTD |= (1<<motor2);
        PORTD |= (1<<motor1);

        PORTD &= ~(1<<motor4);
        PORTB |= (1<<motor3);

    }
}
*/

/*
figure out certain time interval with delays?
smaller than milliseconds
50ms =>10 intervals = 0.5 s
delay + counter => esimates real time 
turn and move need to input time as well
pwm 0-100 running same time as counter
adds up eventually

inm ovement as well static var

PWM inside turn and move 

1 function sets motors => PWM function 

dont use coast use break in PWM
*/

void turn(uint8_t m1_speed, uint8_t m2_speed, _Bool CW_Turn){ // 1 is CW 0 if CCW

    if(CW_Turn){
        motor1F = m1_speed;
        motor2F = 0;
        motor1R = 0;
        motor2R = m2_speed; 
    }
    else{
        motor1F = 0;
        motor2F = m2_speed;
        motor1R = m1_speed;
        motor2R = 0; 
    }
}

void move(uint8_t m1_speed, uint8_t m2_speed, _Bool Fwd){ // 1 is Fwd 0 is back

    if(Fwd){
        motor1F = m1_speed;
        motor2F = m2_speed;
        motor1R = 0;
        motor2R = 0; 
    }    
    else{
        motor1F = 0;
        motor2F = 0;
        motor1R = m1_speed;
        motor2R = m2_speed;
    } 
    
}

void stop(){
    // Setting both motors to brake 3π
    motor1F = 0;
    motor1R = 0;
    motor2F = 0;
    motor2R = 0;

}

param get_param(){
    
    param set_of_params = {_CW,_SLOW,0.1}; //initializing with standard values

    // Graphical User Interface

      //movement
        movement_incr = 0;
        while(1){
            // since buttons are active low == 0 will be used to check for button press
            if(buttonPress(btnC)){
                movementChoiceUp();
            }

            if(buttonPress(btnA)){
                movementChoiceDown();
            }

            if(buttonPress(btnB)){ //!! maybe if the xxx_incr is 0 or something it shouldn't be, maybe include error here and lock them from confirming on a bad value
                break; // confirms selection on the LCD screen
            }

            LCD_execute_command(CLEAR_DISPLAY);
            _delay_ms(400);    

        }

        //speed 
        speed_incr = 0;
        while(1){

            if(buttonPress(btnC)){
                speedChoiceUp();
            }

            if(buttonPress(btnA)){
                speedChoiceDown();
            }

            if(buttonPress(btnB)){
                break; 
            }

            LCD_execute_command(CLEAR_DISPLAY);
            _delay_ms(400);    

        }
        //timing
        time_incr = 0.0; // float
        while(1){

            if(buttonPress(btnC)){
                timeChoiceUp();
            }

            if(buttonPress(btnA)){
                timeChoiceDown();
            }

            if(buttonPress(btnB)){
                break; 
            }

            LCD_execute_command(CLEAR_DISPLAY);
            _delay_ms(400);    

        }
            
            switch(movement_incr){ 
                case 1:
                    set_of_params.direction = _CW;
                    break;
                case 2:
                    set_of_params.direction = _CCW;
                    break;
                case 3:
                    set_of_params.direction = _F;
                    break;
                case 4:
                    set_of_params.direction = _R;
                    break;
            }

            switch(speed_incr){
                    case 1:
                        set_of_params.speed = _SLOW;
                        break;
                    case 2:
                        set_of_params.speed = _MEDIUM;
                        break;
                    case 3:
                        set_of_params.speed = _FAST;
                        break;
                }


            set_of_params.time = time_incr;
            //}
            // CW FAST 0.5
            // set_of_params = {_CW, _FAST, 0.5};
        return set_of_params;

}