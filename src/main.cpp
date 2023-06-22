#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <TimerOne.h>
#include <Wire.h>
/* 
  Retencion de inicio para pulsadores inicio e incremento MEF
  Pantalla lcd funcionando con todos los mensajes         MEF
  Deteccion de los cambios de estado de los infras        MEF
  Con la deteccion de los viajes se cambia el led
  Al cambiar el led se esperan instrucciones para la grua 

  Probar con la grua
  Pensar bien como es el final del programa

  Agregar accesorios al final, ej contador de pulsaciones por dedo
*/

#define FALSE 0
#define TRUE 1

#define incremento 10
#define inicio 12

#define infra1 4
#define infra2 A3
#define infra3 A2
#define infra4 A1
#define infra5 A0

#define pinLatch 7  // 74hc595
#define clockPin 8 
#define dataPin 5  

Servo miservo_1, miservo_2, miservo_3;

LiquidCrystal_I2C lcd(0x3F, 16, 2);

volatile int tIncremento = 0;
volatile int tInicio = 0;
volatile int taux = 0;
volatile int tauxmili = 0;
volatile int tlcd = 0;
volatile int tmin = 0;
volatile int tseg = 0;
volatile int thora = 0;

volatile int estadoPrograma = 1;
volatile int estadoRetencionIncremento = 1;
volatile int estadoRetencionInicio = 1;
volatile int estadoInfras = 0;
volatile int estadoLcd = 0;
volatile int estadoBluetooth = 0;

volatile int numViajes = 0;
volatile int contadorViajes = 0;
volatile int aleatorio = 0;
volatile int numAnterior = 0;

int grados1 = 0;
int grados2 = 0;
int grados3 = 90;

bool flagPulsoIncremento = FALSE;
bool flagPulsoInicio = FALSE;

volatile int menique = 0;
volatile int indice = 0;
volatile int anular = 0;
volatile int mayor = 0;
volatile int pulgar = 0;

volatile int estadoRetencionDedos  = 1;
volatile int estadoDedos  = 1;
bool flagPulsoDedos = FALSE;

volatile int tmensajefinal = 0;

void actualizarLcd();
void juego();

void setup(){

  //Inicializacion del Timer2
  cli(); 
  TCCR2A = 0; 
  TCCR2B = 0; 
  TCNT2 = 0;  

  OCR2A = 255; 
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (0b00000111); //1024 (preescala)
  TIMSK2 |= (1 << OCIE2A);
  sei(); 

  Serial.begin(57600); 

  miservo_1.attach(9, 350, 2900); //servo base, derecha-izquierda
  miservo_1.write(grados1); 

  miservo_2.attach(6, 1000, 2000); //servo de la derecha, adelante-atras
  miservo_2.write(grados2); 

  miservo_3.attach(11, 1000, 2000); //servo de la izqueirda, abajo
  miservo_3.write(grados3);
  delay(500);

  lcd.init();
  lcd.backlight();
  //Mensaje de bienvenida
  lcd.setCursor(0, 0);
  lcd.print("  Bienvenido a  ");
  lcd.setCursor(0, 1);
  lcd.print("  Super Guanti  ");
  delay(1000);  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Una creacion de:");
  lcd.setCursor(0, 1);
  lcd.print("     M.A.L.     ");
  delay(1000);
  lcd.clear();

  pinMode(incremento, INPUT);
  pinMode(inicio, INPUT);

  pinMode(pinLatch, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  //Sin estas lineas hay un led que empieza encendido, copiar esto tmb al final del programa
  digitalWrite(pinLatch, LOW);              
  shiftOut(dataPin, clockPin, MSBFIRST, 0); 
  digitalWrite(pinLatch, HIGH);
}

ISR(TIMER2_COMPA_vect){ 
/* Esta funcion se interrumpe cada 32.77us
   La cuenta no es exacta. Salida ejemplo: 4:30(salida arduino) 4:26(cronometro)
*/
  tauxmili++;
  if (tauxmili >= 30) {
    tIncremento++;
    tInicio++;
    taux++;
    
    if(taux >= 60){
      tlcd--;
      taux = 0;
      if(estadoLcd == 2){
        tseg++;
        if(tseg >= 60){
          tmin++;
          tseg = 0;
          if(tmin >= 60){
            thora++;
            tmin = 0;
          }
        }
      }
    }
  } 
}

void loop(){
  
  actualizarLcd();

  switch(estadoPrograma){
    case 1:
    /* En este caso se hace la eleccion de la cantidad de viajes a realizar y se da inicio al juego
    * Las MEF son para la retencion de los pulsadores de incremento de viajes y de inicio 
    */
      switch(estadoRetencionIncremento){
        case 1:
          flagPulsoIncremento = FALSE;

          if(digitalRead(incremento) == HIGH)
            estadoRetencionIncremento = 2;

          if(digitalRead(incremento) == LOW){
            tIncremento = 0;
            estadoRetencionIncremento = 2;
          }
        break;
        case 2:
          if(tIncremento < 30)
            estadoRetencionIncremento = 2;
          if(tIncremento >= 30)
            estadoRetencionIncremento = 3;
        break;
        case 3: 
          if(digitalRead(incremento) == LOW){
            flagPulsoIncremento  = TRUE;
            estadoRetencionIncremento  = 1;
          }
          else{
            flagPulsoIncremento = FALSE;
            estadoRetencionIncremento = 1;
          }
        break;
      }
      switch(estadoRetencionInicio){
        case 1:
          flagPulsoInicio = FALSE;

          if(digitalRead(inicio) == HIGH)
            estadoRetencionInicio = 1;

          if(digitalRead(inicio) == LOW){
            tInicio = 0;
            estadoRetencionInicio = 2;
          }
        break;
        case 2:
          if(tInicio < 30)
            estadoRetencionInicio = 2;
          if(tInicio >= 30)
            estadoRetencionInicio = 3;
        break;
        case 3: 
          if(digitalRead(inicio) == LOW){
            flagPulsoInicio = TRUE;
            estadoRetencionInicio = 1;
          }
          else{
            flagPulsoInicio = FALSE;
            estadoRetencionInicio = 1;
          }
        break;
      }
      /*Si el pulsador verdaderamente esta presionado, se incrementa una vez la variable*/
      if(flagPulsoIncremento == TRUE){
        numViajes++;
      }
      if(estadoLcd == 2){ //condicion para salir de este estado, le puse esta para no repetir la condicion del estado del lcd
        juego(); //llamo para encender el primer led
        estadoPrograma = 2;
        tmin = 0;
        tseg = 0;
        thora = 0;
      }
    break;
    case 2:
      /*Cuando se detecta algun infra se va al sig estado del programa donde se cuentan bien la cantidad de viajes 
        Despues de contar la cantidad de viajes, se llama a la funcion juego, la cual despues de prender el sig led viene a este estado de programa
      */
      if(digitalRead(infra1) == LOW || digitalRead(infra2) == LOW || digitalRead(infra3) == LOW || digitalRead(infra4) == LOW || digitalRead(infra5) == LOW){
        estadoPrograma = 3;
      }

      if (Serial.available()){

        estadoBluetooth = Serial.read(); 

        ///SERVO 1 -- DERECHA IZQUIERDA -- 9/// COPIAR ESTO PARA TODAS LAS INSTRUCCIONES DE SERVO
        if(estadoBluetooth == '1'){

          grados1++;
          if(grados1 >= 180){
            grados1 = 180;
          }
          miservo_1.write(grados1); //,0 para velocidad 
        }

        if(estadoBluetooth == '3'){

          grados1--;
          if(grados1 <= 0){
            grados1 = 0;
          }
          miservo_1.write(grados1);
        }
        ///SERVO 2 -- ADELANTE ATRAS -- 6///
        if(estadoBluetooth == '5'){

          grados2++;
          if(grados2 >= 180){
            grados2 = 180;
          }
          miservo_2.write(grados2);
        }

        if(estadoBluetooth == '7'){


          grados2--;
          if(grados2 <= 0){
            grados2 = 0;
          }
          miservo_2.write(grados2);
        }
        ///SERVO 3 -- ABAJO -- 11///
        if(estadoBluetooth == '9'){    

          grados3--;        
          if(grados3<=0){
            grados3 = 90;
          }
          miservo_3.write(grados3);
        }  
      }
    break;
    case 3:
    /*  Se detectan los viajes 
    *  La deteccion se produce cuando los infra cambian de estado, es decir que un viaje
    *  se considera valido cuando el bloque se levanta de la plataforma
    */
      switch (estadoInfras)
      {
        case 0:
          if(digitalRead(infra1) == HIGH && digitalRead(infra2) == HIGH && digitalRead(infra3) == HIGH && digitalRead(infra4) == HIGH && digitalRead(infra5) == HIGH){
            estadoInfras = 0;
          }
          if(digitalRead(infra1) == LOW || digitalRead(infra2) == LOW || digitalRead(infra3) == LOW || digitalRead(infra4) == LOW || digitalRead(infra5) == LOW){
            estadoInfras = 1;
          }
        break;
        case 1:
          // if(tinfra >= 3){
          //   contadorViajes++;
          //   juego();
          //   estadoInfras = 0;
          // }
          if(digitalRead(infra1) == LOW || digitalRead(infra2) == LOW || digitalRead(infra3) == LOW || digitalRead(infra4) == LOW || digitalRead(infra5) == LOW){
            
            estadoInfras = 1;
            estadoPrograma = 2;
          }
          if(digitalRead(infra1) == HIGH && digitalRead(infra2) == HIGH && digitalRead(infra3) == HIGH && digitalRead(infra4) == HIGH && digitalRead(infra5) == HIGH){
            contadorViajes++;
            juego();
            estadoInfras = 0;
          }
        break;
      }
    break;
  }
}

void actualizarLcd(){
  /* En esta MEF estan agrupadas todas las salidas en pantalla con sus respectivas condiciones  
   * para cambiar de estado
  */
  switch (estadoLcd)
  {
    case 0:
      lcd.setCursor(0,0);
      lcd.print("Cant de viajes: ");
      lcd.setCursor(0,1);
      lcd.print(numViajes);

      if(flagPulsoInicio == FALSE){
        estadoLcd = 0;
      }
      else{
        tlcd = 5;
        estadoLcd = 1;
      }
    break;
    case 1:
      lcd.setCursor(0,0);
      lcd.print("El juego inicia");
      lcd.setCursor(0, 1);
      lcd.print("     en: ");
      lcd.print(tlcd);

      if(tlcd > 0)
        estadoLcd = 1;
      else{
        lcd.clear();
        estadoLcd = 2;
      }
    break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("    A JUGAR!    ");
      lcd.setCursor(4, 1);
      lcd.print(thora);
      lcd.print(":");
      lcd.print(tmin);
      lcd.print(":");
      lcd.print(tseg);

      if(contadorViajes != numViajes)
        estadoLcd = 2;
      else
        estadoLcd = 3;
    break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("  Felicidades!  ");
      lcd.setCursor(4, 1);
      lcd.print(thora);
      lcd.print(":");
      lcd.print(tmin);
      lcd.print(":");
      lcd.print(tseg);

      if(tmensajefinal = 1){

        estadoLcd = 4;
      }
      else {

        estadoLcd = 3;
      }

      // if(finMensajeFinal == FALSE) //esta flag se usaria para demostrar que se termino de imprimir el mensaje final
      //   estadoLcd = 4;
      // else
      //   estadoLcd = 1;
    break;
    case 4:

    lcd.setCursor(0,0);
    lcd.print("me:");
    lcd.print(menique);
    lcd.print(" in:");
    lcd.print(indice);
    lcd.print(" pul:");
    lcd.print(pulgar);
    lcd.setCursor(0, 1);
    lcd.print("an:");
    lcd.print(anular);
    lcd.print(" ma:");
    lcd.print(mayor);
  }
}

void juego(){
  /* En esta funcion se cambia el led que esta encendido, con la condicion de que no se prenda dos veces el mismo
   * Esta funcion es llamada cuando se detecta como valido un viaje  
   * Luego de encender el led se va al estadoPrograma 2 donde se reciben instrucciones para la grua
   */
  while(aleatorio == numAnterior){
    aleatorio = random(0, 5);
  }
  
  switch (aleatorio)
  {
    case 0:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 1); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 0;
      estadoPrograma = 2;
    break;
    case 1:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 2); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 1;
      estadoPrograma = 2;
    break;
    case 2:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 4); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 2;
      estadoPrograma = 2;
    break;
    case 3:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 8);
      digitalWrite(pinLatch, HIGH);
      numAnterior = 3;
      estadoPrograma = 2;
    break;
    case 4:
      digitalWrite(pinLatch, LOW);               
      shiftOut(dataPin, clockPin, MSBFIRST, 16); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 4;
      estadoPrograma = 2;
    break;
  }
}