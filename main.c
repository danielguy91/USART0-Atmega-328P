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
unsigned char USART_Receive( void );
void  initTimers10ms ();
void ConfPort (uint8_t  RDireccion,uint8_t  RPuerto,unsigned char PDireccion, unsigned char Direccion , unsigned char PEstado, unsigned char Estado  );
void DecodeHeader();
void CksVerif();
//.........................................................................


// .......................VARIABLE GLOBAL..................................
char mubuffer= ' ';

volatile  uint16_t timeOut10ms,TimeInterval; 
 
volatile uint8_t buffer[256];
volatile uint8_t indexWrite,indexRead,indexWriteTX,indexReadTX;
volatile uint8_t status,cks,cksTX,cmdPos_inBuff;
volatile int8_t header,nBytes;
volatile uint8_t timeOutRX;
volatile uint8_t sendData;

  
typedef union {
	uint16_t value;
	uint8_t v[2];
} unionValue;

unionValue proxBytes;


//.........................................................................

void InitUSART( unsigned char baudrate ) {                  //Configurando la USART en modo asincrona 
	
	UBRR0 = baudrate;                                       //Seleccionando la velocidad
	UCSR0C |= (3<<UCSZ00);                                  // Trasmitir datos de 8 bits   3 = 0b011
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);                         //Habilitando la transmisión y recepción
	UCSR0B |= (1<<RXCIE0);                                  //Habilitando interrupcion por recepcion 
}


void USART_Transmit( unsigned char data )
{
	while ( !( UCSR0A & (1<<UDRE0)) );                          // Esperar a que el búfer de transmisión esté vacío  UDRE0 cuando tenga el valor de 1 esta listo para ser usado 
    UDR0 = data; 
	//indexRead++;                                               // Pone los datos en el buffer, envía los datos UDR0 buffer de datos
}

unsigned char USART_Receive( void )
{
	
	while ( !(UCSR0A & (1<<RXC0)) );                            // Esperar a recibir datos
	return UDR0;
	                                                // Obtener y devolver los datos recibidos del buffer 
}

void ConfPort (uint8_t  RDireccion,uint8_t  RPuerto,unsigned char PDireccion, unsigned char Direccion ,unsigned char PEstado, unsigned char Estado  ) {
   RDireccion |= (Direccion<< PDireccion);
   RPuerto |= (Estado << PEstado);

}
/* ConfPort() es una funcion que permite configurar cualquier puerto del microcontrolador ATMega 328P 
              RDireccion = es el registro donde se configurar
			  PDireccion = es la direccion (bit) del Puerto a configurar 
			  Direccion = Es una variable para configurar el puerto como entrada (1)  o salida (0) 
	          RPuerto = Es el registro de estado del puerto 
			  PEstado = Es bit del puerto a configurar
			  Estado =  Si esta configurado como encendido (1) o apagado (0)
*/     

void  initTimers10ms () {
	OCR1A =625 ;                                      // Valor de compracion
	TCNT1 = 0x00 ;                                    // temporizador inicializo
	TIMSK1 = ( 1 << OCIE1A);                          // interrupción de compracion en modo CTC OCR1A = TCNT1
	TCCR1B = ( 1 << WGM12) | ( 1 << CS12) ;           // PRESCALAR EN 256 y MODO CTC
}

void DecodeHeader()
{
	uint8_t aux;
	aux=indexWrite;
	
	while((indexRead!=aux)&&(status==1)) {
		switch(header) {
			case 0://U
			if(buffer[indexRead]=='U') {
				cks='U';
				USART_Transmit( buffer[indexRead] );
			}
			else {
				indexRead=indexWrite;
				header=-1;
			}
			break;
			case 1://UN
			if(buffer[indexRead]=='N') {
				cks^='N';
				USART_Transmit( buffer[indexRead]);
			}
			else {
				indexRead--;
				header=-1;
			}
			break;
			case 2://UNE
			if(buffer[indexRead]=='E') {
				cks^='E';
	        USART_Transmit( buffer[indexRead]);
			}
			else {
				indexRead--;
				header=-1;
			}
			break;
			case 3://UNER
			if(buffer[indexRead]=='R') {
				cks^='R';
			    USART_Transmit( buffer[indexRead]);
			}
			else {
				indexRead--;
				header=-1;
			}
			break;

			case 4:                                         //byte menos significativo
			//if(buffer[indexRead]> 0x00) {
	            //proxBytes.v[0]=buffer[indexRead];
				nBytes=buffer[indexRead];
				cks^=nBytes;
				USART_Transmit( buffer[indexRead]);
				
			//}
			//else { 
				//indexRead--;
				//header=-1;
			//}
			break;
			
			case 5 :                                           //byte mas significativo
			//if(buffer[indexRead]==0x00) {
				//proxBytes.v[1]=buffer[indexRead];
				cks^=0X00;
				USART_Transmit( buffer[indexRead]);
				//PORTB|= (1<<PORTB7);
				
			//}
			//else {
				//USART_Transmit(indexRead);
				//indexRead--;
				//header=-1;
				//PORTB|= (1<<PORTB7);
			//}
			break;

			case 6: // ':'
			if(buffer[indexRead]==':') {
				cks^=buffer[indexRead];
				status=2;//llego toda la cabecera
				USART_Transmit( buffer[indexRead]);
			}
			else {
				indexRead--;
				header=-1;
			}
			break;
		}
		header++;
		indexRead++;
	}
}

void CksVerif(){
	static uint8_t p=1;
	while((indexRead!=indexWrite)&&(p<nBytes)){       //proxBytes.value
		if(p==1)
		cmdPos_inBuff=indexRead;
		PORTB|= (1<<PORTB7);
		cks^=buffer[indexRead];
		indexRead++;
		p++;
	}
	if( (p==proxBytes.value)&&(indexRead!=indexWrite) ){
		p=1;
		if(cks==buffer[indexRead]){
			status=3;
		}
		else{
			status=1;
			indexRead=indexWrite;
		}
		header=0;
	}
}


void CMD(){                                                                   // Lectura de codigos
	switch(buffer[cmdPos_inBuff]){
		case '+':
		status=1;
		TimeInterval=buffer[cmdPos_inBuff+1];                          //incremento 
		TimeInterval|=(uint16_t)buffer[cmdPos_inBuff+2]<<8;
		timeOut10ms = timeOut10ms + TimeInterval;     
	      
		break;
		case '-':
		status=1;
		TimeInterval=buffer[cmdPos_inBuff+1];                              // Decremento  tiempo
		TimeInterval|=(uint16_t)buffer[cmdPos_inBuff+2]<<8;
		timeOut10ms = timeOut10ms - TimeInterval;
		break;
	}
}



int main (void) {
	                                     // 1 segundo apagado y 1 segundo prendido 
	InitUSART(MYUBRR);   
	sei();
    initTimers10ms ();
    ConfPort (DDRB,PORTB,DDB5,1,PORTB5,1);
	ConfPort (DDRB,PORTB,DDB7,1,PORTB7,0);
      timeOut10ms=100;
	 indexWrite=0;
	 indexRead=0;
	 header=0;
	 status=1;
	 timeOutRX=10;
	 sendData=0;
	
	
	while(1){
		
      if(indexRead!=indexWrite){
	      switch(status) {
		    case 1:
		         DecodeHeader();
		      
			  break;
		    
			case 2:
			
		        CksVerif();
		      break;
		    
			case 3:
			     
		         indexRead++;
		         CMD();
		         status=1;
		     break;
	                       }

	                            }
             }
}

ISR(USART_RX_vect){
	   buffer[indexWrite] = UDR0;
        indexWrite++;
		}
	

ISR (TIMER1_COMPA_vect) {
	   timeOut10ms--;
	if (timeOut10ms == 0) {
		timeOut10ms=100;
		if (PORTB & ( 1 << PORTB5)){ // desplazo al numero 1 cinco veces para que aparezca en la posicion 5
		    PORTB &= ~ ( 1 << PORTB5); // ~ (1 << PORTB5) = 11011111 hago cero el bit 5 de PORTB5
	      }
		else {
		   PORTB |= ( 1 << PORTB5);
		  }

		  
	}
	
}