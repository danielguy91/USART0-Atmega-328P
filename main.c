/*
 * USART0.c
 *
 * Created: 17/8/2021 18:19:00
 * Author : Daniel Guy
 */ 
#define F_CPU 16000000
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1                              // Calculo de baudrate 

#include <avr/io.h>
#include <avr/interrupt.h>                                 // Libreria de interrupciones 
#include <util/delay.h>

//........................ PROTOTIPOS .....................................

void InitUSART( unsigned char baudrate );
void USART_Transmit( unsigned char data );
void  initTimers10ms ();
void initPort ();
void DecodeHeader();
void CksVerif();
void CMD();
//.........................................................................


// .......................VARIABLE GLOBAL..................................

volatile  uint16_t timeInitms,timeFinms,TimeInterval; 
 
volatile uint8_t buffer[256];
volatile uint8_t indexWrite,indexRead;
volatile uint8_t status,cks,cksTX,cmdPos_inBuff;
volatile int8_t header,nBytes;
volatile uint8_t sendData;

  
typedef union {
	uint16_t value;
	uint8_t v[2];
} unionValue;

unionValue proxBytes;

//typedef
typedef union{
	struct{
		uint8_t b0: 1;
		uint8_t b1: 1;
		uint8_t b2: 1;
		uint8_t b3: 1;
		uint8_t b4: 1;
		uint8_t b5: 1;
		uint8_t b6: 1;
		uint8_t b7: 1;
	}bit;
	uint8_t byte;
}_sFlag;
//..



// Defines
#define INCREMENTAR flag1.bit.b0 //
#define DECREMENTAR flag1.bit.b1 //
volatile _sFlag flag1;

//.........................................................................

void InitUSART( unsigned char baudrate ) {                 
	
	/* Configuración del USART como UART */

	// USART como UART
	UCSR0C &=~ (1<<UMSEL00);
	UCSR0C &=~ (1<<UMSEL01);

	// Paridad desactivada
	UCSR0C &=~ (1<<UPM00);
	UCSR0C &=~ (1<<UPM01);

	// Stops = 1
	UCSR0C &=~ (1<<USBS0);

	// Datos de 8 bits
	UCSR0C |=  (1<<UCSZ00);
	UCSR0C |=  (1<<UCSZ01);
	UCSR0B &=~ (1<<UCSZ02);
	
	// Calculo del baudrate
	UCSR0A &=~ (1<<U2X0);
	UBRR0 = baudrate;

	UCSR0B |= (1<<TXEN0); //activo recepcion de datos
	UCSR0B |= (1<<RXEN0); //activo envio de datos

	UCSR0B |= (1<<RXCIE0); //interrupcion de recepcion completada
}

void USART_Transmit( unsigned char data )
{
	while ( !( UCSR0A & (1<<UDRE0)) );                          // Esperar a que el búfer de transmisión esté vacío  UDRE0 cuando tenga el valor de 1 esta listo para ser usado 
    UDR0 = data; 
	//indexRead++;                                               // Pone los datos en el buffer, envía los datos UDR0 buffer de datos
}

void initPort () {
   DDRB |= (1<< DDB5);                              // Configuro como salida el puerto DDB5   
   PORTB &= ~(1 << PORTB5);                          // El puerto esta en 0
    
   DDRB |= (1<< DDB0); 
    
}

void  initTimers10ms () {
	OCR1A =625 ;                                      // Valor de compracion
	TCNT1 = 0x00 ;                                    // temporizador inicializo
	TIMSK1 = ( 1 << OCIE1A);                          // interrupción de compracion en modo CTC OCR1A = TCNT1
	TCCR1B = ( 1 << WGM12) | ( 1 << CS12) ;           // PRESCALAR EN 256 y MODO CTC
}

void DecodeHeader()
{
		
	while((indexRead!=indexWrite)&&(status==1)) {
		
		switch(header) {
			case 0://U
			
			    if(buffer[indexRead]=='U') {
				cks='U';
				header++;
				indexRead++;                         
									 
										   }
			else {
				indexRead--;
				header--;
			}
			break;
			case 1://UN
			if(buffer[indexRead]=='N') {
				cks^='N';
				header++;
				indexRead++;
			}
			else {
				indexRead--;
				header--;
			}
			break;
			case 2://UNE
			if(buffer[indexRead]=='E') {
				cks^='E';
				header++;
				indexRead++;
			}
			else {
				indexRead--;
				header--;
			}
			break;
			case 3://UNER
			    if(buffer[indexRead]=='R') {
				cks^='R';
				header++;
				indexRead++;
				
			                               }
			    else {
				 indexRead--;
				 header--;
			          }
			    break;
			 case 4: 
					if(buffer[indexRead] > 0x00) {
					    nBytes = buffer[indexRead];
						cks^= nBytes;
						header++;
						indexRead++;
						
					}
					else {
						indexRead--;
						header--;
					}
					break;
			 case 5:
			 if(buffer[indexRead]==0x00) {
				cks^= 0x00;
				 header++;
				 indexRead++;
				 
			 }
			 else {
				 indexRead--;
				 header--;
			 }
			 break;
			  case 6:
			  if(buffer[indexRead]==':') {
				  cks^= ':';
				  header++;
				  indexRead++;
				  
			  }
			  else {
				  indexRead--;
				  header--;
			  }
			  break;
			   case 7:
			   if(buffer[indexRead] != 0x00) {
				   cks^= buffer[indexRead] ;
				   cmdPos_inBuff=indexRead;
				   header++;
				   indexRead++;
				   
			   }
			   else {
				   indexRead--;
				   header--;
			   }
			   break;		
			case 8:                                //cks
				if(cks == buffer[indexRead]) {
					
					 status=2;					 
				     indexRead=indexWrite;
					 header=0;
				
				}
				else {
					indexRead--;
					header--;
				     }
				break;
		}
	}
}

//void CksVerif(){
	//static uint8_t p=1;
	//while((indexRead!=indexWrite)&&(p<nBytes)){       
		//if(p==1){
		//cmdPos_inBuff=indexRead;
	    //cks^=buffer[indexRead];
		//indexRead++;
		//p++;
		//}
	//}
	//if( (p==nBytes)&&(indexRead==indexWrite) ){
		//p=1;
		//if(cks==buffer[indexRead]){
			//status=3;
		//}
		//else{
			//status=1;
			//indexRead=indexWrite;
		//}
		//header=0;
		//
	//}
//}

void CMD()  {                                                                   // Lectura de codigos
	
	switch(buffer[cmdPos_inBuff]) {
	    case '+':
		    
            INCREMENTAR=1;     
		
		break;
		
		case '-':
			DECREMENTAR=1;
		     
		break;
	}
}


int main (void) {
	 DECREMENTAR=0;
	 INCREMENTAR=0;                                    // 1 segundo apagado y 1 segundo prendido 
	InitUSART(MYUBRR);   
	initPort ();
    initTimers10ms ();	
	 sei();
     timeInitms= 10;                    // tiempo inicio en 50 ms  (5*10)= 50 ms 
	 indexWrite=0;
	 indexRead=0;
	 header=0;
	 status=1;
	 timeFinms=0;
	 sendData=0;
	
	while(1){
		
      if((indexRead!=indexWrite) && INCREMENTAR==0 && DECREMENTAR ==0){
	      switch(status) {
		    case 1:
		         DecodeHeader();
		         
			  break;
		       
			case 2:
			   
		         CMD();
		  
		     break;
	                       }

	                            }
		if (INCREMENTAR==1 && DECREMENTAR ==0 )
		{
			 if (timeInitms<=300){
			   
			    timeInitms +=10;
			 }
			 else{
				timeInitms=300; 
			 }
	         status=1;	
			INCREMENTAR = 0;
		
			}
		if (DECREMENTAR==1 && INCREMENTAR==0)
		{
		           if(timeInitms > 10){
				   timeInitms -=10;
				   }
				   else{
					timeInitms=10;   
				   }
					
					
					status=1;
					DECREMENTAR = 0;
					
				}
			if (!timeFinms){
				timeFinms = timeInitms;
				//(1 << PORTB5 1) desplazado por B5 es igual a 00100000
				if(PORTB & (1 << PORTB5)) //desplazo al numero 1 cinco veces para que aparezca en la posicion 5
				PORTB &= ~(1 << PORTB5); //~ (1 << PORTB5) = 11011111 hago cero el bit 5 de PORTB5
				else
				PORTB |= (1 << PORTB5);
			}				
		
}
}

ISR(USART_RX_vect){
	   buffer[indexWrite] = UDR0;
        indexWrite++;
		}
	

ISR (TIMER1_COMPA_vect) {
        
	  if (timeFinms){
		  timeFinms--;
	                 }
}
