/*
 * TIMY.c
 *
 * Created: 2/7/2016 12:42:22 PM
 * Author : NEUTRAL ATOM
 */ 

 

/***********************************PIN DEFINATION******************
CNT_UNIT DIGIT       PINA0
CNT_TENS DIGIT       PINA2
RST_UNIT DIGIT       PINA1
RST_TENS DIGIT       PINA3
DISPLAY ENABLE       PINB0  
LED ON				 PINA6
RELAY				 PINA7
BUTTON_H			 PINA4
BUTTON_M			 PINA5

* CLK INTERNAL 8MHZ 
* CLK PRESCALAR  Clk/1024
* TIMER CALCULATIONS 1000000/1024 = 977Hz
* COUNTING 65536 = 65535/977 = 67.07 Sec
* 1 sec = 65535/67.07 = 977 count

******************************************************************/
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/sleep.h>


//#include <avr/iotn841.h>
//#define F_CPU 8000000UL

//**********************************MACROS******************************************************************


#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= (~(1<<BIT)))
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))
#define eeprom_hr 02
#define eeprom_min 03
#define eepromset_hr 00
#define eepromset_min 01
#define eeflag 04


//***********************************FUNCTION CALL DECLARATION***********************************************

void in_cnt(uint8_t pulse_U);		//SET UNITS COUNT
void RST_U();						//RESET UNIT
void RST_T();						//RESET TENS
void DEI();							//DISPLAY ENABLE
void DDI();							//DISPLAY DISABLE
void RLY_ON();						//RELAY ON
void RLY_OFF();						//RELAY OFF
void BLINK();						//BLINK DISPLAY
void SCAN_BUTTON();					//BUTTON SCANNING

//************************************GLOBAL VARIABLES*******************************************************

uint32_t count,hour,min,sec,set_hr,set_min,set_sec,set_time,scan;
uint32_t hr_mode,min_mode,temp,comp,old_hr,old_min,old_sethr,old_setmin,eflag;
uint32_t dis_sec,disflag,dis_show,dis_time,blink,dis_blink,sleep_dis,dis_on,dis_min;
uint32_t tens,units,dif_hr,dif_min;
uint32_t act_time,dif_time;

//FUNCTION DECLARATION******************************************************************************************


void in_cnt (uint8_t pulse)                         //SET UNIT COUNT         
{  

   uint8_t x,y,z;
   RST_U();
   
   y=pulse/10;
   units=pulse%10;
   
   if(y!=tens)
   {  
	  RST_T();
	  tens=y;
	  
	  for(x=0;x<y;x++)
	  {
		  SETBIT(PORTA,PINA2);
		  CLEARBIT(PORTA,PINA2);
	  } 
   }
   
	   for(z=1;z<=units;z++)
	   {
		   SETBIT(PORTA,PINA0);
		   _delay_us(1);
		   CLEARBIT(PORTA,PINA0);
	   }
   
}


void DEI()                                    //DISPLAY ENABLE
{
   SETBIT(PORTB,PINB0);
}

void DDI()                                    //DISPLAY DISABLE
{
   CLEARBIT(PORTB,PINB0);
}

void RST_U()                                 //RST UNIT DIGIT
{
   SETBIT(PORTA,PINA1);
   _delay_us(10);
   CLEARBIT(PORTA,PINA1);
}

void RST_T()                                 //RESET TENS DIGIT
{
   SETBIT(PORTA,PINA3);
   _delay_us(10);
   CLEARBIT(PORTA,PINA3);
}

void RLY_ON()                                 //MAKES RELAY ON & LED ON
{
   SETBIT(PORTA,PINA7);
   SETBIT(PORTA,PINA6);
}

void RLY_OFF()                                 //MAKES RELAY OFF & LED OFF
{
   CLEARBIT(PORTA,PINA7);
   CLEARBIT(PORTA,PINA6);
}

void BLINK()
{   
	if(blink==1)
	DDI();
	if(blink==0)
	DEI();
}

void SCAN_BUTTON()
{  
   if (set_time!=1)
   {   
	   scan=0;
   
      if (!(CHECKBIT(PINA,PINA4)))
      {   min=0;
		  DEI();
		  if (min_mode==1)
		 {
			 RST_U();
			 RST_T();
			 min_mode=0;
			 in_cnt(set_hr);
			 //_delay_ms(300);
		 }
		 
         hr_mode=1;
         count=0;
         RLY_OFF();
         set_hr++;
		 _delay_ms(300);
		 
		 while((!(CHECKBIT(PINA,PINA4))))
		 {
			 set_hr++;
			 _delay_ms(200);
			 in_cnt(set_hr);
			 
			 if (set_hr>=24)
			 {
				 set_hr=0;
				 RST_U();
				 RST_T();
			 }
			 
		 }
		 
		 if (set_hr>=24)
		 {
			 set_hr=0;
			 RST_U();
			 RST_T();
		 }
		 
		 in_cnt(set_hr);
         set_sec=sec;
         scan=1;
         
      }
      
      if (!(CHECKBIT(PINA,PINA5)))
      {   min=0;
		  DEI();
		  
		  if (hr_mode==1)
	      {
		      RST_U();
		      RST_T();
		      hr_mode=0;
			  in_cnt(set_min);
	      }
		  
         min_mode=1;
         count=0;
         RLY_OFF();
         set_min++;
		 _delay_ms(300);
		 
		  while((!(CHECKBIT(PINA,PINA5))))
		  {
			  set_min++;
			  _delay_ms(200);
			  in_cnt(set_min);
			  
			  if (set_min>=60)
			  {
				  set_min=0;
				  RST_U();
				  RST_T();
			  }
			  
		  }
		 
		 if (set_min>=60)
		 {
			set_min=0;
			RST_U();
			RST_T();
		 }
		 
		 in_cnt(set_min);
         set_sec=sec;
         scan=1;
      }
   }   //END OF SET TIME = 0
   
   if (set_time==1)												//TIME SET
   {  
      if ((!(CHECKBIT(PORTA,PINA4)))|(!(CHECKBIT(PORTA,PINA5))))
      {
		  dis_show=1;
		  dis_min=0;
      }
       
   }  //END OF SET TIME = 1
   
}

unsigned char EEPROM_read(unsigned int ucAddress)
{  
	char rsreg;
	rsreg=SREG;
	cli();
   /* Wait for completion of previous write */
   while(EECR & (1<<EEPE));
   /* Set up address register */
   EEAR = ucAddress;
   /* Start eeprom read by writing EERE */
   EECR |= (1<<EERE);
   /* Return data from data register */
   return EEDR;
   SREG=rsreg;
   sei();
}


void EEPROM_write(unsigned int ucAddress, unsigned char ucData)
{  
	char wsreg;
	wsreg=SREG;
   cli();
   /* Wait for completion of previous write */
   while(EECR & (1<<EEPE));
   /* Set Programming mode */
   EECR = (0<<EEPM1)|(0<<EEPM0);
   /* Set up address and data registers */
   EEAR = ucAddress;
   EEDR = ucData;
   /* Write logical one to EEMPE */
   EECR |= (1<<EEMPE);
   /* Start eeprom write by setting EEPE */
   EECR |= (1<<EEPE);
   SREG=wsreg;
   sei();
}

//****************************************************ISR*******************************************************

ISR(TIMER1_COMPA_vect) 
{ // this function is called every time the timer reaches the threshold we set
   cli();
   sec++;
   //in_cnt(sec);
   dis_sec++;
   dis_blink++;
   
   if(dis_blink>=1)       //BLINKING DISPLAY 1SEC ON ___ 1SEC OFF
   {
	   blink=!blink;
	   dis_blink=0;
   }
   
   if(dis_sec>=10)               //DISPLAY HR for 10SEC FLIP TO MIN for 10SEC FLIP
   {   
	   act_time=(hour*3600)+(min*60);
	   dif_time=(set_hr*3600)+(set_min*60);
	   dif_time=dif_time-act_time;
	   dif_min=dif_time/60;
	   dif_hr=0;

	   while(dif_min>=60)
	   {
		   dif_min=dif_min-60;
		   dif_hr++;
	   }
	   
	   dis_time++;
	   disflag=!disflag;           
	   dis_sec=0;   
   }
   
   if(dis_time>=12)				//DISPLAY ON FOR 2 MINS
   {   
	   dis_show=0;
	   
	   if(set_time==1)
	   DDI();
	   sleep_dis=1;
	   dis_time=0;
   }
   
   if (sec>=60)
   {
      min++;
	  dis_min++;
      sec=0;
	  //in_cnt(sec);
   }
   
   if (min>=60)
   {
      hour++;
	  dis_min=0;
      min=0;
   }
   
   if (hour>=24)
   {
      hour=0;
   }
   
   if((set_time!=1)&&(min==2))                              //SWITCH OFF DISPLAY AFTER 2 mins. 
   {
	   DDI();
	   min=0;
   }
   
   if ((dis_min<5)&&(set_time==1))
   {  
	   if(disflag==0)
	   {   
		   if(dif_hr==0)
		   in_cnt(dif_min);
		   else
		   in_cnt(dif_hr);   
	   }
	   
	   BLINK();
	   
	   if(disflag==1)
	   {   
		   if(dif_min==0)
		   in_cnt(dif_hr);
		   else
		   in_cnt(dif_min);
	   }
	   
   }
   
   
   if ((set_time==1)&&(hour==set_hr))                   // TIMER FINISH COUNTS  COMPARES HRS AND MINS
   {
	   if(min==set_min)
	   {
		   RLY_OFF();
		   DDI();
		   RST_U();
		   RST_T();
		   set_time=0;
		   set_hr=0;
		   set_min=0;
		   hour=0;
		   min=0;
		   blink=0;
		   
	   }
	   
   }
   sei();
}

//*************************************************************************************************************************
int main(void)
{   
   count=0;      //INTIALIZE VARIABLES;
   set_hr=0;
   set_min=0;
   hour=0;
   min=0;
   sec=0;
   set_sec=0;
   set_time=0;
   hr_mode=0;
   min_mode=0;
   disflag=0;
   dis_blink=0;
   dis_sec=0;
   dis_show=0;
   units=0;
   tens=0;
   
   
//PORTS INTIALIZATION  *****************************************************************************************************
   
   PORTA=0;			//RESET ZERO INTIALIZE
   PORTB=0;			//RESET ZERO INTIALIZE
   DDRA=0xCF;		// INTIALIZE PORT 
   DDRB=0x01;      // INTIALIZE PORT
   
   
   DEI();		//ENABLE DISPLAY
   RST_U();
   RST_T();
   
//************* TIMER INTIALIZATION *****************************************************************************************
   
    TCCR1B |= (1 << WGM12);						// configure timer1 for CTC mode
    TIMSK1 |= (1 << OCIE1A);					// enable the CTC interrupt
    sei();										// enable global interrupts
    OCR1A   = 8083;								// set the CTC compare value
    TCCR1B |= ((1 << CS10) | (1 << CS12));      // start the timer at 1MHz/1024
	
   
//*******************************INTIAL DISPLAY & PLUG RELAY *****************************************************************

//set_sleep_mode(SLEEP_MODE_IDLE);

/****************************************************************************************************************************
   eflag=EEPROM_read(eeflag);
   
   if (eflag==11)
   {
	   count=88;
	   RLY_OFF();
	   DEI();
	   in_cnt(count);
	   EEPROM_write(eeflag,00);
   }
   
   else if (eflag==00)
   {
	   old_hr=EEPROM_read(eeprom_hr);
	   _delay_us(1);
	   old_min=EEPROM_read(eeprom_min);
	   _delay_us(1);
	   old_sethr=EEPROM_read(eepromset_hr);
	   _delay_us(1);
	   old_setmin=EEPROM_read(eepromset_min);
	   
	   if ((old_hr!=old_sethr)||(old_min!=old_setmin))
	   {
		   set_hr=old_sethr-old_hr;
		   set_min=old_setmin-old_min;
		   		   		  		 						
		   RLY_ON();
		   cli();
		   TCCR1B |= ((1 << CS10) | (1 << CS12));      // stop the timer
		  
		   hour=0;
		   min=0;
		   sec=0;
		   dis_sec=0;
		   dis_blink=0;
		   dis_time=0;
		   
		   sei();										// enable global interrupts
		   OCR1A   = 977;								// set the CTC compare value
		   TCCR1B |= ((1 << CS10) | (1 << CS12));      // start the timer at 1MHz/1024
		  
		   set_time=1;
		   hr_mode=0;
		   min_mode=0;
		   
	   }
	   
	   else 
	   {
		   RLY_ON();
		   count=88;
		   DEI();
		   in_cnt(count);
		   
	   }
	   
   }*/
   
   
   
 /* Replace with your application code */
    while (1) 
    { 
	
	  SCAN_BUTTON();							//BUTTONS SCAN 
	  
	 
	  if(((hr_mode==1)||(min_mode==1))&&(scan==0)&&(sec>(10+set_sec)))			//wait for 10 sec then set time
      {  
		 
		 RLY_ON(); 
		 cli();
		 TCCR1B |= ((1 << CS10) | (1 << CS12));      // stop the timer 
		 
		 hour=0;
		 min=0;
		 sec=0;
		 
		 dis_sec=0;
		 dis_blink=0;
		 dis_time=0;
		 
		 sei();										// enable global interrupts
		 OCR1A   = 8083;								// set the CTC compare value
		 TCCR1B |= ((1 << CS10) | (1 << CS12));      // start the timer at 1MHz/1024
		  
         set_time=1;
		 hr_mode=0;
		 min_mode=0;
		 dis_sec=0;
		 disflag=0;
		 act_time=0;
		 dis_show=1;
		 
		 //EEPROM_write(eepromset_hr,set_hr);
		 //_delay_us(1);
		 //EEPROM_write(eepromset_min,set_min);
		 
		 //act_time=(hour*3600)+(min*60);
		 dif_time=(set_hr*3600)+(set_min*60);
		 dif_time=dif_time-act_time;
		 dif_min=dif_time/60;
		 dif_hr=0;

		 while(dif_min>=60)
		 {
			 dif_min=dif_min-60;
			 dif_hr++;
		 }
		  
      }
	    
	  if (set_time==1)
	  {   
		  
				 if ((set_hr>=1)||(set_min>=30))                    //EVERY 15 MIN SAVE INTO EEPROM
				  {   
					  comp=min;
					  if (min==comp+15)
					  {
						comp=min;
						//EEPROM_write(eeprom_hr,hour);
						//_delay_us(1);
						//EEPROM_write(eeprom_min,min);
					  }
				  }      //END OF EEPROM TIME SAVE
		  
		  
		  if (hour==set_hr)                 // TIMER FINISH COUNTS  COMPARES HRS AND MINS
			  {
				  if(min==set_min)
				  {
					  /*EEPROM_write(eeprom_hr,00);
					  _delay_us(1);
					  EEPROM_write(eeprom_min,00);
					  _delay_us(1);
					  EEPROM_write(eepromset_hr,00);
					  _delay_us(1);
					  EEPROM_write(eepromset_min,00);
					  _delay_us(1);
					  EEPROM_write(eeflag,11);*/
			  
				  }       //END OF CLEAR EEPROM
		  
			   }
			   
			   
		  
	  /*if (dis_show==1)						// SHOW TIME WHEN KEY IS PRESSED FOR 2 MINS 
		  {   
			  //dis_min=0;
		  }    //END OF DISPLAY TIME
		
		if(sleep==1)
		{   
			cli();
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
			sleep=0;
			in_cnt(0);
		}*/
		
	  }   //END OF SET TIME
	    
    }   // END OF WHILE(1)
}