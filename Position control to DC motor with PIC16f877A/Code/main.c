#include <16F877A.h>        // Introducing the header file of the controller to use.
#device adc=10              // It is stated that 10-bit ADC will be used.
#FUSES XT, NOWDT, NOPROTECT, NOBROWNOUT, NOLVP, NOPUT, NODEBUG, NOCPD // Controller configuration settings
#use delay(crystal=4000000) // Specifies the oscillator frequency to be used for the delay function.

#include <lcd.c> // introducing the lcd.c file

#define  in1 pin_b1 // PORTB1 is called in1
#define  in2 pin_b2 // PORTB2 is called in2


signed long rev = 0;        //Defining a variable named "rev".
signed long error= 1;       //Defining a variable named "error".
signed long duty_cycle=0;   //Defining a variable named "duty_cycle".
signed int quad = 0;        //Defining a variable named "quad".
signed long change = 0;     //Defining a variable named "change".
signed long angle = 0.0f;   //Defining a variable named "angle". 
signed long value;          //Defining a variable named "value".
signed long last_read;      //Defining a variable named "last_read".
int8 encoderread;           //Defining a variable named "encoderread".
long realposition=0;        //Defining a variable named "realposition".
unsigned long Kp=0;        //Defining a variable named "Kp". The control parameter is calculated based on the information obtained from the pot connected to adc_1
unsigned int16 refangle= 20;   //Defining a variable named "refangle". Calculated based on information from pot connected to adc_0


#INT_RB                             //RB interrupt is activated when there is any change in pins b4-b7. 
                                    // It will be used to read the encoder information, depending on the motor.
void RB_IOC_ISR(void)               //encoder interrupt
{
  clear_interrupt(INT_RB);          // Set the interrupt to 0.
  encoderread = input_b() & 0x30;   // Process the 8-bit PORTB value with the number h'30' (00110000) bit by bit and transfer it to the variable "encoderread".
                                    // Masking is done in this line. In other words, except the 4th and 5th bits of portb, the other bits are set to 0.
  if(encoderread==last_read)        // If the content of the variable "encoderread" is equal to the content of the variable "last_read";
   return;                          // Exit without interrupting and continue the program from where it left off.
   
                                    // If the content of "encoderread" variable is not equal to the content of "last_read" variable, that is, if there is a change in encoder information;
  if(bit_test(encoderread,4)==bit_test(last_read,5))  // If bits 4 and 5 of the "encoderread" variable (also bits 4 and 5 of portb) are equal 
   {
   quad += 1;                       // Increment the content of the "quad" variable by one. (forward turn)
   }
   
   else                             // If bits 4 and 5 of the variable "encoderread" (also bits 4 and 5 of portb) are not equal
   {
   quad -= 1;                       // Decrease the content of the "quad" variable by one. (return)
   }
   
   last_read=encoderread;           // Pass the contents of the "encoderread" variable to the contents of the "last_read" variable. (prepares variable "last_read" for next comparison)
}


signed int encoderget(void)         // Function to be used to detect the number of pulses
{
 
 signed int value = 0 ;             // Set the "value" variable to 0.
 while (quad>=4)                    // As long as the content of the variable "quad" is greater than or equal to 4, the following two lines are processed.
                                    // the purpose here is to increase the content of the variable "value" by 1 while decreasing the content of the variable "quad" by 4,
                                    //is to calculate how far the encoder (hence the motor) rotates in the forward direction
 {
   value += 1;                      // Increment the content of the variable "value" by 1
   quad -= 4;                       // Decrease the "quad" variable content by 4
 }
 while(quad<=-4)                    // As long as the content of the variable "quad" is less than or equal to -4, the following two lines are processed. 
                                    // here the goal is to increase the content of the "quad" variable by 4 while decreasing the content of the "value" variable by 1,
                                    // is to calculate how much the encoder (hence the motor) rotates in the reverse direction
 {
  value -= 1;                       // Decrease the content of the "value" variable by 1
  quad += 4;                        // Increment "quad" variable content by 4
 }
 return value;                      // return with the contents of the variable "value"
}


void main()       // main program
{
 lcd_init();                          // LCD is getting ready
 delay_ms(10);                        // 10 mSec waiting time
 set_tris_c(0xff);                    // PORTC all output. only to be used as RC2 pwm output
 setup_ccp1(CCP_PWM);                 // RC2 pin set as PWM output (4KHz)
 setup_timer_2(T2_DIV_BY_16, 255, 1); // Setting timer2 parameters for frequency setting of PWM signal.
 setup_adc(adc_clock_div_32);         // ADC clock frequency fosc/32
 setup_adc_ports(AN0_AN1_AN3);        // A0, A1, A3 ARE SET AS ANALOG INPUT. (A3 not used)

 enable_interrupts(INT_RB);           // Enable RB interrupt
 enable_interrupts(GLOBAL);           // Allow all interrupts

 output_low(in1);                     // 0 is given to motor driver input "in1"
 output_low(in2);                     // 0 is given to motor driver input "in2" 
                                      // In this case, the motor does not rotate.


while(1)
{
  set_adc_channel(0);                        // Prepare for RA0/AN0 ADC operation
  delay_us(10);                              // 10mSec delay time
  refangle = read_adc()*250.0f/1023+20 ;     // Convert the data read from the ADC to the reference angle with the formula.
  printf(LCD_PUTC,"\fRef:%lu",refangle);     // print the reference angle value to the LCD.
  delay_ms(10);                              // 10mSec delay time
  
  set_adc_channel(1);                        // Prepare for RA0/AN0 ADC operation
  delay_us(10);                              // 10mSec delay time
  Kp = read_adc()*60f/1023+20 ;            // ADC'den okunan veriyi form?l ile  Kp de?erine ?evir.
  lcd_gotoxy(8,1);                           // LCD'de, belirtilen konuma git
  printf(LCD_PUTC,",Kp:%lu",Kp);             // Kp de?erini LCD'ye yazd?r.
  delay_ms(10);                              // 10mSec delay time
  
  change=encoderget();                       // The encoderget() function is executed. 
                                             // The 'value' value obtained as a result of the operation of the function is loaded into the "change" variable. 
                                             // What is loaded into the "change" variable is actually the position change information obtained as a result of the movement of the motor.
      if(change)
            {
            realposition =realposition+change;  // position change information obtained as a result of the movement of the motor is added to the previous position information of the motor (+ or -)            
            } 
       
      error=refangle - realposition ;        // The difference between the reference position information and the current position of the motor is found and loaded into the "error" variable
      printf(LCD_PUTC," \nEr:%li",error);    // print error value to LCD
      lcd_gotoxy(8,2);                       // Go to the specified location on the LCD
      printf(LCD_PUTC,",Pwm:%li",duty_cycle); //Print pwm duty cycle value to LCD 
      delay_ms(100);                         // 100mSec delay time
            
       while(error!=0)                       // If "error" value is different from 0
       {
          duty_cycle= abs(Kp*error);         // Determine the absolute value of the result of multiplying the "error" value with the "Kp" value as the pwm duty cycle value
          if(duty_cycle>=1023){duty_cycle=1023;}  //if the "duty_cycle" value is greater than 1203, set it to 1023. Maximum "duty_cycle" value is 1023 (100%)
          set_pwm1_duty(duty_cycle);              // Update duty cycle rate of PWM signal received from PWM1 output
          if(error>0)                             // If "error" value is greater than 0
          {
          output_low(in1);                        // 0 is given to motor driver input "in1"
          output_high(in2);                       // 1 is given to the motor driver "in2" input
                                                  // According to this situation, the motor moves forward.
          }         
          else if(error<0)                         // If "error" value is greater than 0
          {
          output_high(in1);                        // 1 is given to the motor driver "in1" input
          output_low(in2);                         // 0 is given to motor driver input "in2"
                                                   // In this case, the motor moves backwards.
          }       
          change=encoderget();               // The encoderget() function is executed.
                                             // The value value obtained as a result of the operation of the function is loaded into the "change" variable. 
                                             // What is loaded into the "change" variable is actually the position change information obtained as a result of the movement of the motor.
          realposition =realposition+change; //position change information obtained as a result of the movement of the motor is added to the previous position information of the motor (+ or -)   
          error=refangle - realposition;     // The difference between the reference position information and the current position of the motor is found and loaded into the "error" variable.
          printf(LCD_PUTC,"\fRef:%lu",refangle);      // print the reference angle value to the LCD.
          lcd_gotoxy(8,1);                            // Go to the specified location on the LCD
          printf(LCD_PUTC,",Kp:%lu",Kp);              // Print the Kp value to the LCD.
          printf(LCD_PUTC," \nEr:%li",error);         // print error value to LCD
          lcd_gotoxy(8,2);                            // Go to the specified location on the LCD
          printf(LCD_PUTC,",Pwm:%li",duty_cycle);     // Print pwm duty cycle value to LCD
           delay_ms(20);                              // 20mSec delay time
       } 
          duty_cycle= abs(Kp*error);                  // Determine the absolute value of the result of multiplying the "error" value with the "Kp" value as the pwm duty cycle value
                                                      // Since the "error" value will be 0 as a result of the position correction, the content of the "duty_cycle" variable will also be 0.
          set_pwm1_duty(duty_cycle);                  // The duty cycle rate of the PWM signal received from the PWM1 output is updated to 0 and the motor stops.
          delay_ms(100);                              // 100mSec delay time
          rev=realposition/360.0f;                    // 360 represents encoder pulses per revolution
          angle = ((int16)(rev*360))%360;             // The real-time angle value is calculated here.
                                                      
                                                      
}
}
    
