int relais1 = ;
int relais2 = ;
int relais3 = ;
int relais4 = ;

int temp1 = ;
int temp2 = ;
int temp3 = ;

int 

void setup() {
  // put your setup code here, to run once:
    
}

void loop() {
  // put your main code here, to run repeatedly:
      //commande de désactivation
      digitalWrite(relais,HIGH);
      //retardement de 2 seconde et demis
      delay(2500);
      //initialisation du pin en mode output ce qui met la pin en mode LOW et active le relai
      pinMode(relais, OUTPUT); 
      //Commande d'activation de relai
      digitalWrite(relais,LOW);
      //retardement de 2 seconde et demis
      delay(2500);
}
