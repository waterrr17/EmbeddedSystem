#include "msp.h"
#include "Clock.h"
#include <stdio.h>

#define LED_RED 1
#define LED_GREEN (LED_RED << 1)
#define LED_BLUE  (LED_RED << 2)

/**
 * main.c
 */

void led_init() {
    P2->SEL0 &= ~0x07;
    P2->SEL1 &= ~0X07;
    P2->DIR |= 0X07;
    P2->OUT &= ~0X07;
}

void turn_off_led() {
    P2->OUT &= ~0b11111110;
}

void switch_init() {
    P1->SEL0 &= ~0x12;
    P1->SEL1 &= ~0x12;

    P1->DIR &= ~0x12;

    P1->REN |= 0x12;

    P1->OUT |= 0x12;
}

void systick_init(void) {
    SysTick->LOAD = 0x00FFFFFF;
    SysTick->CTRL = 0x00000005;
}

void systick_wait1ms() {
    SysTick->LOAD = 47999;
    SysTick->VAL = 0;
    while((SysTick->CTRL & 0x00010000)==0) {};
}

void systick_wait1s() {
    int i;
    int count = 1000;

    for(i=0; i<count; i++) {
        systick_wait1ms();
    }
}

void sensor_init() {
    //0, 2, 4, 6 IR Emitter
    P5->SEL0 &= ~0x08;
    P5->SEL1 &= ~0x08;
    P5->DIR |= 0x08;
    P5->OUT &= ~0x08;

    //1, 3, 5, 7 IR Emitter
    P9->SEL0 &= ~0x04;
    P9->SEL1 &= ~0x04;
    P9->DIR |= 0x04;
    P9->OUT &= ~0x04;

    //0~7 IR Sensor
    P7->SEL0 &= ~0xFF;
    P7->SEL1 &= ~0xFF;
    P7->DIR &= ~0xFF;
}

void pwm_init34(uint16_t period, uint16_t duty3, uint16_t duty4) {
    TIMER_A0->CCR[0] = period;

    TIMER_A0->EX0 = 0x0000;

    TIMER_A0->CCTL[3] = 0x0040;
    TIMER_A0->CCR[3] = duty3;
    TIMER_A0->CCTL[4] = 0x0040;
    TIMER_A0->CCR[4] = duty4;

    TIMER_A0->CTL = 0x02F0;

    P2->DIR |= 0xC0;
    P2->SEL0 |= 0xC0;
    P2->SEL1 &= ~0xC0;
}

void motor_init(void) {
    P3->SEL0 &= ~0xC0;
    P3->SEL1 &= ~0xC0;
    P3->DIR |= 0xC0;
    P3->OUT &= ~0xC0;

    P5->SEL0 &=~0x30;
    P5->SEL1 &= ~0X30;
    P5->DIR |= 0x30;
    P5->OUT &= ~0x30;

    P2->SEL0 &= ~0xC0;
    P2->SEL1 &= ~0xC0;
    P2->DIR |= 0xC0;
    P2->OUT &= ~0xC0;

    pwm_init34(7500, 0, 0);
}

void move(uint16_t leftDuty, uint16_t rightDuty) {
    P3->OUT |= 0xC0;
    TIMER_A0->CCR[3] = leftDuty;
    TIMER_A0->CCR[4] = rightDuty;
}

void left_forward() {
    P5->OUT &= ~0x10;
}

void left_backward() {
    P5->OUT |= 0x10;
}

void right_forward() {
    P5->OUT &= ~0x20;
}

void right_backward() {
    P5->OUT |= 0x20;
}

void (*TimerA2Task)(void);

void TimerA2_Init(void(*task)(void), uint16_t period) {
    TimerA2Task = task;
    TIMER_A2->CTL = 0x0280;
    TIMER_A2->CCTL[0] = 0x0010;
    TIMER_A2->CCR[0] = (period - 1);
    TIMER_A2->EX0 = 0X0005;
    NVIC->IP[3] = (NVIC->IP[3]&0xFFFFFF00)|0x00000040;
    NVIC->ISER[0] = 0x00001000;
    TIMER_A2->CTL |= 0x0014;
}

void TA2_0_IRQHandler (void) {
    TIMER_A2->CCTL[0] &= ~0x0001;
    (*TimerA2Task)();
}

void task() {
    printf("interrupt occurs!\n");
}

void timer_A3_capture_init() {
    P10->SEL0 |= 0x30;
    P10->SEL1 &= ~0x30;
    P10->DIR &= ~0x30;

    TIMER_A3->CTL &= ~0x0030;
    TIMER_A3->CTL = 0x0200;

    TIMER_A3->CCTL[0] = 0x4910;
    TIMER_A3->CCTL[1] = 0x4910;
    TIMER_A3->EX0 &= ~0x0007;

    NVIC->IP[3] = (NVIC->IP[3]&0x0000FFFF) | 0x40400000;
    NVIC->ISER[0] = 0x0000C000;
    TIMER_A3->CTL |= 0x0024;
}

uint16_t first_left;
uint16_t first_right;
uint16_t period_left;
uint16_t period_right;

void TA3_0_IRQHandler(void) {
    TIMER_A3->CCTL[0] &= ~0x0001;
    period_right = TIMER_A3->CCR[0] - first_right;
    first_right = TIMER_A3->CCR[0];
}

//void TA3_N_IRQHandler(void) {
//    TIMER_A3->CCTL[1] &= ~0x0001;
//    period_left = TIMER_A3->CCR[1] - first_left;
//    first_left = TIMER_A3->CCR[1];
//}

uint32_t get_left_rpm() {
    return 2000000 / period_left;
}

uint32_t left_count;
void TA3_N_IRQHandler(void) {
    TIMER_A3->CCTL[1] &= ~0x0001;
    left_count++;
}

void little_bit_forward(){
    int i;
    left_forward();
    right_forward();
    move(700, 700);

    for(i=0; i<320; i++) {
        systick_wait1ms();
        //printf("%d\n", i);
    }
}

void little_bit_backward(){
    int i;
    left_backward();
    right_backward();
    move(700, 700);

    for(i=0; i<320; i++) {
        systick_wait1ms();
        //printf("%d\n", i);
    }
}

int sensor1;
int sensor2;
int sensor3;
int sensor4;
int sensor5;
int sensor6;
int sensor7;
int sensor8;

void IR_LED_charge(){
    /* Turn on IR LEDs */
        P5->OUT |= 0x08;
        P9->OUT |= 0x04;

        P7->DIR = 0xFF; // P7 OUTPUT
        P7->OUT = 0xFF; // capacitor charge

        Clock_Delay1us(10); // fully charge
        P7->DIR = 0x00; // P7 input


        Clock_Delay1us(1200); // wait for a while

        sensor1=P7->IN & 0x01;
        sensor2=P7->IN & 0x02; //sensor IR 2th
        sensor3=P7->IN & 0x04; //sensor IR 3th
        sensor4=P7->IN & 0x08; //sensor IR 4th
        sensor5=P7->IN & 0x10; //sensor IR 5th
        sensor6=P7->IN & 0x20; //sensor IR 6th
        sensor7=P7->IN & 0x40; //sensor IR 7th
        sensor8=P7->IN & 0x80;
}

void main(void)
{
   Clock_Init48MHz();
   systick_init();
   led_init();
   switch_init();
   sensor_init();
   motor_init();
   timer_A3_capture_init();

   while(1) {
      //Turn on IR LEDs
      P5->OUT |= 0x08;
      P9->OUT |= 0x04;

      P7->DIR = 0xFF;
      P7->OUT = 0xFF;

      Clock_Delay1us(10);
      P7->DIR = 0x00;

      Clock_Delay1us(1200);

      sensor1=P7->IN & 0x01;
      sensor2=P7->IN & 0x02; //sensor IR 2th
      sensor3=P7->IN & 0x04; //sensor IR 3th
      sensor4=P7->IN & 0x08; //sensor IR 4th
      sensor5=P7->IN & 0x10; //sensor IR 5th
      sensor6=P7->IN & 0x20; //sensor IR 6th
      sensor7=P7->IN & 0x40; //sensor IR 7th
      sensor8=P7->IN & 0x80;

      // start line, end line
      if((!sensor1) && sensor3 && sensor4 && sensor5 && sensor6 && (!sensor8)) {
//          printf("start line\n");
          move(0, 0);
          systick_wait1s();
      }

      // exist left route -> turn left
      else if(sensor5 && sensor6 && sensor7) {
//          printf("left 90 degree turn\n");
//          left_forward();
//          right_forward();
//          move(700, 700);
//
          int i;
          int count = 100;
          for(i=0; i<count; i++) {
              systick_wait1ms();
          }


          //90 degree turn
          left_count = 0;
          move(700, 700);
          little_bit_forward();

          Clock_Delay1us(10);

          while(1){
              IR_LED_charge();

              right_forward();
              left_backward();

              if(left_count > 200) {
                  left_count = 0;
                  break;
              }
          }
      }

      // exist right route -> turn right
      else if(sensor2 && sensor3 && sensor4) {
          little_bit_forward();
          Clock_Delay1us(10);


          while(1) {
              IR_LED_charge();

              if(sensor4 || sensor5) {
                  P5->OUT &= ~0x30;
                  P2->OUT |= 0xC0;
                  P3->OUT |= 0xC0;
                  move(700, 700);
                  break;
              }

              else {
//                  little_bit_backward();
//                  left_forward();
//                  right_forward();
//                  move(700, 700);
//
                  int i;
                  int count = 100;
                  for(i=0; i<count; i++) {
                      systick_wait1ms();
                  }

                  //90 degree turn
                  left_count = 0;
                  move(700, 700);
                  while(1){
                      IR_LED_charge();

                      left_forward();
                      right_backward();


                      if(left_count > 180) {
                          left_count=0;

                          break;
                      }
                  }
                  little_bit_forward();
                  break;
              }

          }


      }

      // go straight
      else if (sensor4 || sensor5) {
//          printf("go straight\n");
          P2->OUT |= 0x01;
          P5->OUT &= ~0x30;
          P2->OUT |= 0xC0;
          P3->OUT |= 0xC0;
          move(700, 700);
      }



      // recognize all 8 sensor -> turn left
//      else if(sensor1 && sensor2 && sensor3 && sensor4 && sensor5 && sensor6 && sensor7 && sensor8) {
//          left_forward();
//          right_forward();
//          move(700, 700);
//
//          int i;
//          int count = 100;
//          for(i=0; i<count; i++) {
//              systick_wait1ms();
//          }
//
//          //90 degree turn
//          left_count = 0;
//          move(700, 700);
//          while(1){
//              IR_LED_charge();
//
//              right_forward();
//              left_backward();
//
//              if(left_count > 180) {
//                  left_count = 0;
//                  break;
//              }
//          }
//      }

      // there exist no path -> go back
      else if(!sensor2 && !sensor3 && !sensor4 && !sensor5 && !sensor6 && !sensor7) {
//          printf("dead end\n");
//          left_forward();
//          right_forward();
//          move(700, 700);

          int i;
          int count = 100;
          for(i=0; i<count; i++) {
              systick_wait1ms();
          }

          //180 degree turn
          left_count = 0;
          move(700, 700);
          while(1){
              IR_LED_charge();

              left_forward();
              right_backward();

              if(left_count > 360) {
                  left_count = 0;

                  break;
              }
          }

          little_bit_forward();
      }

      else{
          P2->OUT &= ~0x07;
          move(0,0);
      }
   }
}

