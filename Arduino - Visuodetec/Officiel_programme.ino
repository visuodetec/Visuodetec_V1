#include <Wire.h> // Bibliothèque I2C
#include <LiquidCrystal_I2C.h> // Inclure la bibliothèque de l'écran LCD
#include <OneButton.h>// Clicks encodeur rotatif 
#include <avr/wdt.h> // Bibliothèque pour reset

// Pins
// 1. Encodeur rotatif 
const int CLK = 7; //CLK pin sur rotary encoder (Évènement)
const int DT = 8; //DT pin sur rotary encoder (Sens)
const byte SW = 9; //SW pin sur rotary encoder (Fonction bouton)
OneButton rotarySwitch(SW, true); // Configure rotary Switch, true = INPUT_PULLUP
//2. Écran LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); // Déclarer l'adresse, le nb de colonnes et de lignes de l'écran.
//3. Buzzer
const int buzzerPin = 5; // BUZZER pin sur circuit
//4. DEL RGB
const int rougePin = 4;
const int bleuPin = 2;
const int vertPin = 3;

// Variables (surtout utilisées pour éviter boucles)
bool choixFait = false;
bool reponseOui = false;

bool choixFait2 = false;
bool reponseOui2 = false;

bool choixFait3 = false;
bool reponseOui3 = false;

bool q2Affiche = false;
bool renduTraitement2 = false;
bool modeAttenteXlus = false;
bool resultatMontres = false;
bool symptomesLus = false;
bool texte6affiche = false;
bool etatsTermines = false;
bool webRappelLu = false;
bool buttonPressed = false; //état départ du bouton
bool accueilTermine = false; //état départ accueil
int texteIndex = 0; // Numéros de texte

unsigned long timer2; // Déclarer le data type de timer2 (chronomètre)
unsigned long timer3;

// CLK + DT
int CLKnow; // Déclarer le data type de CLK (actuel)
int CLKlast; // Déclarer le data type de CLK (précédent)

// Symptômes
int counter = 0;
int counterlast; // La valeur sauvegardée à la fin de chaque question
bool question1fait = false; // Vérifier si la question 1 est fait ou non
bool question2fait = false;
bool question3fait = false;
bool afficherResultats = false;
bool modetraitementsXlus = false;
bool modeAttenteFait = false;
bool attenteFini = false;
bool ReboursFait = false;
int i = 20;

// Score
int totalScore = 0; // Le score de fatigue en lien avec les 3 questions symptômes

unsigned long questionStartTime = 0; // Définir un temps en millisecondes
const unsigned long questionDelay = 20000;// Définir  constammentun temps en millisecondes
int options[] = {1,2,3,4,5}; // Valeurs possibles (sous forme d'array)
int currentIndex = 0; // Commence à la première valeur (1)
int currentValue = options[currentIndex]; // Choisir l'index de la valeur de la liste

// Serial
String msg = ""; // Message lu
String buffer = "";
bool scoreRecu = false;

// États
enum Etat { ACCUEIL, TEXTE2, TEXTE3 }; // Énumération des textes
Etat etat = ACCUEIL; // Premier état (texte) au départ

enum EtatMenu { QUESTION1, WEBCAM, QUESTION2_AFFICHE, QUESTION2_ATTENTE, SYMPTOMES, QUESTIONSY2, QUESTIONSY3, RESULTATSY ,MODE_AFFICHE, MODE_ATTENTE, MODE, FIN};
EtatMenu etatMenu = QUESTION1;

////////////////////////////////////////////////////////

// Double Click fonction
void doubleClick(){
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("-  Double click:  -"));
  lcd.setCursor(0,1); lcd.print(F("desapprouve/choix"));
  lcd.setCursor(0,2); lcd.print(F("immediat a modifier."));
  lcd.setCursor(0,3); lcd.print(F("-   REDEMARRAGE!   -"));
  jouerMelodieRESET(); // Fonction mélodie reset par buzzer
  delay(5000);
  restartArduino(); // Reset l'arduino après double clicker l'encodeur rotatif
}

//Reset fonction
void restartArduino(){
  wdt_enable(WDTO_15MS);  // Watchdog timer
  while (1) {} // Redémarre
}

void setup() {
  // Configuraton Serial
  Serial.begin(9600); // Vitesse de communication serial
  //Configuration des pins
  // 1.Encodeur rotatif
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  CLKlast = digitalRead(CLK); // Lire état premier CLK
  rotarySwitch.attachDoubleClick(doubleClick);
  // 2.Écran LCD
  lcd.init(); // Intialiser l'écran
  lcd.backlight(); // Activer le rétroéclairage
  // 3.Buzzer
  pinMode(buzzerPin, OUTPUT); // OUTPUT: renvoie de l'information
  // 4.DEL RGB
  pinMode(rougePin, OUTPUT);
  pinMode(vertPin, OUTPUT);
  pinMode(bleuPin, OUTPUT);
}

//Fonction DEL RGB
void setColor(int rougeValue, int vertValue, int bleuValue){
  analogWrite(rougePin, rougeValue); // AnalogWrite: luminosité variable (signal 0-255)
  analogWrite(vertPin, vertValue);
  analogWrite(bleuPin, bleuValue);
}

//Fonction bouton
void gererBouton(){
  int currentSW = digitalRead(SW);
  // Si le bouton est relâché, le bouton n'est pas pressé: faux
  if (currentSW == HIGH) buttonPressed = false; 
   // Si le bouton est appuyé (LOW) ET que le bouton n'est pas encore appuyé (vrai) 
  if (currentSW == LOW && !buttonPressed) { 
    buttonPressed = true; // Bouton appuyé: vrai
    if (etat == ACCUEIL){ // Si on est à l'état et que le bouton est appuyé
      etat = TEXTE2; // On passe au texte 2 (afficher texte 2)
    }
    else if (etat == TEXTE2){ // Si on est à l'état et que le bouton est appuyé
      etat = TEXTE3; // Si on est à l'état et que le bouton est appuyé
    }
  }
}

//Fonction loop
void loop(){
  rotarySwitch.tick();
  gererBouton();
  if (!etatsTermines){
    switch (etat) {
      case ACCUEIL: // Si on est à l'état posible
        afficherAccueil(); // Afficher le texte
        break; // Sortie du switch
      case TEXTE2:
        afficherTexte2();
        break;
      case TEXTE3:
        afficherTexte3();
        break;
    }
  } 
  else if (etatsTermines){
    if (etatMenu == QUESTION1){ // Si on est rendu à la question 1
      afficherTexte4(); // Question traitement #1
      
      OuiNon1(); // Sélection oui/non
      if (choixFait){ // Si le choix est fait (dans la fonction Oui/non)
        if (reponseOui){ // Si reponseOui était True dans la fonction Oui/non: OUI: on passe au 1er traitement (webcam).
          choixFait = false; // Ne plus rentrer dans la fonction (choixfait)
          setColor(0, 0, 0);
          etatMenu = WEBCAM; // Rendre à l'état RESULTSY après que ça soit fini (POUR AFFICHER RÉSULTAT) !!!!!!!!!!!!!!!
        }
        else{  // NON: on passe au 2ème traitement. 
          choixFait = false; // Ne plus rentrer dans la fonction
          setColor(0, 0, 0);
          etatMenu = QUESTION2_AFFICHE; // On passe à l'état de la question 2 (deuxième traitement)
        }
      }
    }
    else if (etatMenu == WEBCAM){  // Pyserial et webcam - 1er traitement
      // MESSAGE DE DÉBUT: FIXER LA CAMÉRA, PRÉPARER OBJET À RAPPROCHER 
      bool dejaEnvoye = true;
      if (!webRappelLu){
        webcamRappel();
        webRappelLu = true;
        dejaEnvoye = false;
      }
      // DÉBUT - ENVOI
      if (!dejaEnvoye){
        Serial.println("START"); // Envoie signal de départ au python
        dejaEnvoye = true; // Message d'initialisation envoyé
        lcd.clear();
        lcd.setCursor(0,0); lcd.print(F("-- MESSAGE envoye --"));
        lcd.setCursor(0,1); lcd.print(F("- Analyse en cours -"));
        lcd.setCursor(0,2); lcd.print(F("- Suivez regles -"));
        return; // Fait sortir du loop()
      }
      // FIN - Réponse
      else if (Serial.available()>0){ // Reçoit le data (bytes) stockée dans serial       
        msg = Serial.readStringUntil("\n"); // Lit le message envoyé sur le serial (le score accumulé par le programme python) jusqu'au \n (un espace déterminant la longueur du message (string) envoyé)
        msg.trim(); // Enlève espaces ou retours (\n ou \r)
        Serial.print("Received: ["); Serial.print(msg); Serial.println("]"); // Test pour voir si le message est bien reçu par python dans le serial monitor
        if (msg.startsWith("SCORE:")){ // Si le message envoyé par Python (pyserial) sur le serial commence par SCORE: -->
          String scoreStr = msg.substring(6); // Substring à la 6ème position du string (valeur du score) stockée dans une variable.
          scoreStr.trim();  // S'assure d'enlèver espaces ou retours (\n ou \r)
          if(scoreStr.length() > 0){
            int scr = scoreStr.toInt(); // Stock la valeur scoreStr transformé en integer (chiffre/nombre entier)
            totalScore += scr; // Ajout du score de la partie Python (OpenCV) au score total (incluant celui pour les symptômes)
            lcd.clear();
            lcd.setCursor(0,0); lcd.print(F("FIN 1er traitement:"));
            lcd.setCursor(0,1); lcd.print(F("SCORE:"));
            lcd.setCursor(0,2); lcd.print(totalScore); // Affiche score sur le LCD
            delay(8000);
            etatMenu = QUESTION2_AFFICHE; // Passage au 2ème traitement;
          }
        }
      }
    }
    else if (etatMenu == QUESTION2_AFFICHE){ 
      if (!q2Affiche){
        afficherTexteSy(); // AVANT: Question sur le traitement #2
        CLKlast = digitalRead(CLK); // Reset encodeur (position)
        choixFait2 = false;          // Reset flag
        q2Affiche = true;
      }
      etatMenu = QUESTION2_ATTENTE; // Passe à l'étape de la sélection du traitement #2 (Oui/Non) 
    }
    else if (etatMenu == QUESTION2_ATTENTE){
      OuiNon2(); // Sélection oui/non de la même manière
      if (choixFait2){
        if(reponseOui2){ // Si OUI: on passe au 2ème traitement
          setColor(0, 0, 0);
          lcd.clear();
          CLKlast = digitalRead(CLK);
          choixFait2 = false; // Sortir du if condition
          etatMenu = SYMPTOMES;// OUI: on passe à l'étape pour le 2ème traitement
        }
        else{ // Si NON: on passe au 3ème traitement (mode)
          lcd.clear();
          setColor(0, 0, 0);
          choixFait2 = false; 
          etatMenu = MODE_AFFICHE; //NON: Rendu à l'état du mode
        }
      }
    }
    else if (etatMenu == SYMPTOMES){
      // Questions symptômes - 2ème traitement ------
      if (!symptomesLus){
       symptomes1();
       symptomesLus = true;
      }
      QuestionSy1();
      if (question1fait){
        etatMenu = QUESTIONSY2;
      }
    }
    else if (etatMenu == QUESTIONSY2){ // Deuxième question symptômes
      QuestionSy2();
      if (question2fait){
        etatMenu = QUESTIONSY3;
      }
    }
    else if (etatMenu == QUESTIONSY3){ 
      QuestionSy3();
      if (question3fait){
        etatMenu = RESULTATSY;
      }
    }
    else if (etatMenu == RESULTATSY){ 
      if (!afficherResultats){
        afficherResultatSY();      // Afficher résultats (totalScore)
        afficherResultats = true;
        etatMenu = MODE_AFFICHE;   // Passer au prochain état: MODE_AFFICHE
      }
    }
    else if (etatMenu == MODE_AFFICHE){
      lcd.clear(); 
      afficherTexte5(); 
      CLKlast = digitalRead(CLK); 
      choixFait3 = false;         
      etatMenu = MODE_ATTENTE; 
    }
    else if (etatMenu == MODE_ATTENTE){ // Question mode: 3ème traitement
      OuiNon3();
      if (choixFait3){ 
        if (reponseOui3){ // OUI: on accepte le troisième traitement 
          setColor(0, 0, 0);
          choixFait3 = false; 
          etatMenu = MODE;
        }
        else{  // NON: on n'accepte pas, c'est la FIN.
          setColor(0, 0, 0);
          choixFait = false; 
          etatMenu = FIN;
        }
      }
    }
    else if (etatMenu == MODE){ // Fonctionnement du mode - 3ème traitement
      if (!modetraitementsXlus){ // Annonce - Moyens de traiter la fatigue oculaire proposés.
        modeTraitements();
        modetraitementsXlus = true;
      }
      else if (!modeAttenteXlus){
        timer2 = millis(); // Commence Timer 
        timer3 = millis();
        i = 20;
        if (!modeAttenteFait){
          modeAttente(); // Annonce d'attente
          modeAttenteFait = true;
          modeAttenteXlus = true;
        }
      }
      else if (millis() - timer3 >= 60000){
        if (!ReboursFait){
          lcd.clear();
          i -= 1 ;
          lcd.setCursor(0,0); lcd.print(F("-- TEMPS RESTANT --"));
          lcd.setCursor(2,1); lcd.print(i);
          lcd.setCursor(4,1); lcd.print(F(" minute(s) avant")); // Affiche le temps restant
          lcd.setCursor(0,2); lcd.print(F("une nouvelle pause!")); 
          timer3 = millis();
        }
      }
      else if (i == 0){
        ReboursFait = true;
      }
      else if (millis() - timer2 >= 1200000){ // Si 20 minutes sont passées (1200000ms)
        modeRappel(); // Rappeler l'utilisateur
        modeAttenteXlus = false; // Relancer la pause
        modeAttenteFait = false;
        ReboursFait = false;
      }
    }
    else if (etatMenu == FIN){  // Message de fin
      if (!texte6affiche){ // Si le texte 6 n'a pas été afficher: (X LOOP X)
        lcd.clear();
        setColor(0, 0, 255);
        afficherTexte6();
        texte6affiche = true;
      }
    }
  }
}

void QuestionSy1(){ // Question 1 symptôme
    if (question1fait) return;

    if (questionStartTime == 0){
      lcd.setCursor(0,0); lcd.print(F("Vision trouble,"));
      lcd.setCursor(0,1); lcd.print(F("floue, double, gene"));
      lcd.setCursor(0,2); lcd.print(F("ou eblouissements?"));
      questionStartTime = millis(); // Marque le début du timer, pour ensuite attendre un certain temps.
    };

    CLKnow = digitalRead(CLK); 
    if (CLKnow != CLKlast && CLKnow == HIGH){ 
      if (digitalRead(DT) != CLKnow){ 
        currentIndex++;
      } else {
        currentIndex--;
      }
        if (currentIndex < 0) currentIndex = 0; // Limites pour éviter de choisir en dessous de 0
        if (currentIndex > 4) currentIndex = 4; // Limites pour éviter de choisir au dessous de 4

        currentValue = options[currentIndex];

        lcd.setCursor(0,3); 
        lcd.print(F("Niveau: ")); 
        lcd.setCursor(8,3); lcd.print(currentValue);
        lcd.print(F("   "));
        tone(buzzerPin, 800, 50);
      };
      CLKlast = CLKnow; 

      if (millis() - questionStartTime >= questionDelay){
        question1fait = true;
        counterlast = currentValue; // Sauvegarde si besoin
        totalScore += counterlast; // Ajout des scores de chaque question
        lcd.clear();
        questionStartTime = 0;
      }
}

void QuestionSy2(){ // Question 2 symptôme
    if (question2fait) return;

    if (questionStartTime == 0){
      lcd.setCursor(0,0); lcd.print(F("Irritation, brulure,"));
      lcd.setCursor(0,1); lcd.print(F("picotement, rougeur,"));
      lcd.setCursor(0,2); lcd.print(F("secheresse, larmes?"));
      questionStartTime = millis(); 
    };

    CLKnow = digitalRead(CLK); 
    if (CLKnow != CLKlast && CLKnow == HIGH){ 
      if (digitalRead(DT) != CLKnow){ 
        currentIndex++;
      } else {
        currentIndex--;
      }
        if (currentIndex < 0) currentIndex = 0;
        if (currentIndex > 4) currentIndex = 4; 

        currentValue = options[currentIndex];

        lcd.setCursor(0,3); 
        lcd.print(F("Niveau: ")); 
        lcd.setCursor(8,3); lcd.print(currentValue);
        lcd.print(F("   "));
        tone(buzzerPin, 800, 50);
      };

      CLKlast = CLKnow; 

      if (millis() - questionStartTime >= questionDelay){
        question2fait = true;
        counterlast = currentValue; 
        totalScore += counterlast; 
        lcd.clear();
        questionStartTime = 0;
      }
}

void QuestionSy3(){ 
    if (question3fait) return;

    if (questionStartTime == 0){
      lcd.setCursor(0,0); lcd.print(F("Maux de tete, mal"));
      lcd.setCursor(0,1); lcd.print(F("concentration,"));
      lcd.setCursor(0,2); lcd.print(F("douleurs posture?"));    
      questionStartTime = millis(); 
    };

    CLKnow = digitalRead(CLK); 
    if (CLKnow != CLKlast && CLKnow == HIGH){ 
      if (digitalRead(DT) != CLKnow){ 
        currentIndex++;
      } else {
        currentIndex--;
      }
        if (currentIndex < 0) currentIndex = 0;
        if (currentIndex > 4) currentIndex = 4; 

        currentValue = options[currentIndex];

        lcd.setCursor(0,3); 
        lcd.print(F("Niveau: ")); 
        lcd.setCursor(8,3); lcd.print(currentValue);
        lcd.print(F("   "));
        tone(buzzerPin, 800, 50);
      };
      CLKlast = CLKnow; 

      if (millis() - questionStartTime >= questionDelay){
        question3fait = true;
        counterlast = currentValue; 
        totalScore += counterlast; 
        lcd.clear();
        questionStartTime = 0;
      }
}

void afficherResultatSY(){
  if (!resultatMontres){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Votre score apres"));
    lcd.setCursor(0,1); lcd.print(F("le(s) traitement(s):"));
    float totalScoreMoy = totalScore / 3.0; // Moyenne pour afficher résultats (if)
    lcd.setCursor(0,2); lcd.print(totalScoreMoy);  lcd.print(F("      ")); 
     // RÉSULTATS: PLANIFIER LE BUZZER ET LA LED RGB
    if (totalScoreMoy >= 1 && totalScoreMoy <= 2){
        lcd.setCursor(0,3); lcd.print(F("Jamais/aucun prob"));
        setColor(0, 255, 0); // Coleur Verte
        jouerMelodieVIC();
        delay(8000);
        setColor(0, 0, 0);
        resultatMontres = true; 
    }
    else if (totalScoreMoy > 2 && totalScoreMoy <= 3){
        lcd.setCursor(0,3); lcd.print(F("Parfois/tolerable")); 
        setColor(0, 255, 0); // Coleur Verte
        jouerMelodieVIC();
        delay(8000);
        setColor(0, 0, 0);
        resultatMontres = true; 
    }
    else if (totalScoreMoy > 3 && totalScoreMoy <= 4){
        lcd.setCursor(0,3); lcd.print(F("Souvent/intolerable")); 
        setColor(255, 255, 0); // Coleur Jaune
        jouerMelodieMID();
        delay(8000);
        setColor(0, 0, 0);
        resultatMontres = true; 
    }
    else if (totalScoreMoy > 4 && totalScoreMoy <= 5){
        lcd.setCursor(0,3); lcd.print(F("Constamment/irritant")); 
        setColor(255, 0, 0); // Coleur Rouge
        jouerMelodieECHEC();
        delay(8000);
        setColor(0, 0, 0);
        resultatMontres = true; 
    }
    else if (totalScoreMoy > 5){
        lcd.setCursor(0,3); lcd.print(F("Constamment/severe")); 
        setColor(255, 0, 0); // Coleur Rouge
        jouerMelodieECHEC();
        delay(8000);
        setColor(0, 0, 0);
        resultatMontres = true; 
    }
  }
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("Ne vous inquiete"));
  lcd.setCursor(0,1); lcd.print(F("pas, il s'agit"));
  lcd.setCursor(0,2); lcd.print(F("d'un signe etant"));
  lcd.setCursor(0,3); lcd.print(F("eventuel/potentiel."));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("A l'instant, des"));
  lcd.setCursor(0,1); lcd.print(F("moyens/conseils vous"));
  lcd.setCursor(0,2); lcd.print(F("sont proposes pour"));
  lcd.setCursor(0,3); lcd.print(F("attenuer la fatigue:"));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("1.Pauses regulieres"));
  lcd.setCursor(0,1); lcd.print(F("Chaque 20 min,"));
  lcd.setCursor(0,2); lcd.print(F("regarder a 20 pieds"));
  lcd.setCursor(0,3); lcd.print(F("(6m) durant 20 sec."));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("2.Eloigner vous des"));
  lcd.setCursor(0,1); lcd.print(F("des ecrans, surtout"));
  lcd.setCursor(0,2); lcd.print(F("avant de dormir."));
  lcd.setCursor(0,3); lcd.print(F("Aussi:hydratez-vous."));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("3.Cligner des yeux"));
  lcd.setCursor(0,1); lcd.print(F("plus frequemment."));
  lcd.setCursor(0,2); lcd.print(F("Etirer vous pour"));
  lcd.setCursor(0,3); lcd.print(F("posture (dos/cou)"));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("4.Ajuster eclairage"));
  lcd.setCursor(0,1); lcd.print(F("Nettoyez vos ecrans."));
  lcd.setCursor(0,2); lcd.print(F("Evitez eblouissement"));
  lcd.setCursor(0,3); lcd.print(F("exterieur fenetres."));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("5.Filtre d'ecrans"));
  lcd.setCursor(0,1); lcd.print(F("anti-reflet, regler"));
  lcd.setCursor(0,2); lcd.print(F("contraste/luminosite"));
  lcd.setCursor(0,3); lcd.print(F("+ taille police."));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("6.Ajuster angle vue"));
  lcd.setCursor(0,1); lcd.print(F("Distance 50-70cm"));
  lcd.setCursor(0,2); lcd.print(F("ecran-yeux. Posture"));
  lcd.setCursor(0,3); lcd.print(F("droite, pieds sol."));
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("7.Exercices orthopie"));
  lcd.setCursor(0,1); lcd.print(F("Fixer crayon/doigts"));
  lcd.setCursor(0,2); lcd.print(F("rapproche du nez."));
  lcd.setCursor(0,3); lcd.print(F("Rotation yeux, paume"));
  delay(2000);
}

void OuiNon1(){
  if (choixFait) return; // Si le choix est fait (true), on le retourne dans le loop.
  CLKnow = digitalRead(CLK); //Lire l'état actuel du CLK
  if (CLKnow != CLKlast){ // Si le CLK a changer de position (en tournant l'enocdeur).
    if (digitalRead(DT) != CLKnow){ // OUI (Si DTnow et CLKnow ne sont pas à la même valeur binaire: sens horaire pour OUI)
      reponseOui = true;
    } else {
      reponseOui = false;  // Sinon, NON 
    }
    choixFait = true; // Après la sélection, le choix est fait (VRAI)
  }
  CLKlast = CLKnow; // Update CLK maintenant pour le prochain
}

void OuiNon2(){
  if (choixFait2) return; 
  CLKnow = digitalRead(CLK);
  if (CLKnow != CLKlast){
    if (digitalRead(DT) != CLKnow){ // OUI 
      reponseOui2 = true;
    } else {
      reponseOui2 = false;
    }
    choixFait2 = true;
  }
  CLKlast = CLKnow;
}

void OuiNon3(){
  if (choixFait3) return; 
  CLKnow = digitalRead(CLK);
  if (CLKnow != CLKlast){
    if (digitalRead(DT) != CLKnow){ // OUI 
      reponseOui3 = true;
    } else {
      reponseOui3 = false;
    }
    choixFait3 = true;
  }
  CLKlast = CLKnow;
}

void afficherAccueil(){
  static bool dejaAffiche = false;  //Au départ: n'est pas affiché
  // Si ce n'est PAS affiché, alors c'est vrai:
  if (!dejaAffiche){
    lcd.setCursor(0,0); lcd.print(F("Bienvenue!")); 
    lcd.setCursor(0,1); lcd.print(F("Ici Visuodetec!"));
    lcd.setCursor(0,2); lcd.print(F("Appuyez sur le"));
    lcd.setCursor(0,3); lcd.print(F("bouton pour debuter!"));
    jouerMelodieVIC2();
    dejaAffiche = true; //C'est déjà été affiché: false --> sortie de cette condition
  }
}

void afficherTexte2(){
  static byte etape = 0; // Num. de l'étape conservée (static) à travers le loop. Byte: moins de mémoire à la place d'integer(0-255)
  static unsigned long timer = 0; // Variable de data longue (millisecondes) pour établir le timer.

  if (etape == 0) {
      lcd.setCursor(0,0); lcd.print(F("1. Clause de"));
      lcd.setCursor(0,1); lcd.print(F("non responsabilite:"));
      lcd.setCursor(0,2); lcd.print(F("informations a"));
      lcd.setCursor(0,3); lcd.print(F("fins educatifs.   -")); 
      lcd.write(0b01111110); 
      timer = millis();
      etape = 1; 
  }
  // Si est à la prochaine étape ET que le temps écoulé depuis le début du timer est égal ou plus grand que 3 secondes (8000ms), passer au prochain texte.
  else if (etape == 1 && millis() - timer >= 2000) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("N'est pas un"));
    lcd.setCursor(0,1); lcd.print(F("diagnostic medical."));
    lcd.setCursor(0,2); lcd.print(F("Appuyer 1x bouton"));
    lcd.setCursor(0,3); lcd.print(F("pour accepter."));
    timer = millis();
    etape = 2; 
  }
}

void afficherTexte3(){
  static byte etape = 0;
  static unsigned long timer = 0; 

  if (etape == 0) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("2. Droit au retrait"));
    lcd.setCursor(0,1); lcd.print(F("ou changement de"));
    lcd.setCursor(0,2); lcd.print(F("votre consentement"));
    lcd.setCursor(0,3); lcd.print(F("vous est permis!"));
    timer = millis();
    etape = 1;
  } 

  else if (etape == 1 && millis() - timer >= 2000) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Pour le faire,"));
    lcd.setCursor(0,1); lcd.print(F("vous n'avez"));
    lcd.setCursor(0,2); lcd.print(F("qu'appuyer 2x"));
    lcd.setCursor(0,3); lcd.print(F("le bouton!"));
    timer = millis();
    etape = 2;
  }
  else if (etape == 2 && millis() - timer >= 2000){
    etape = 0;
    etatsTermines = true;
  }
}

void afficherTexte4(){
  static byte etape = 0;
  static unsigned long timer = 0; 

  if (etape == 0) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Traitement #1"));
    lcd.setCursor(0,1); lcd.print(F("Souhaitez-vous"));
    lcd.setCursor(0,2); lcd.print(F("activer collecte"));
    lcd.setCursor(0,3); lcd.print(F("donnees perso.?"));
    timer = millis();
    etape = 1;
  } 

  else if (etape == 1 && millis() - timer >= 2000) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("En acceptant:"));
    lcd.setCursor(0,1); lcd.print(F("webcam ouvre"));
    lcd.setCursor(0,2); lcd.print(F("et analyse du"));
    lcd.setCursor(0,3); lcd.print(F("visage + yeux."));
    timer = millis();
    etape = 2;
  }

  else if (etape == 2 && millis() - timer >= 2000) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Choisissez: "));
    lcd.setCursor(0,1); lcd.print(F("   Gauche | Droite "));
    lcd.setCursor(0,2); lcd.print(F("      OUI | NON "));
    setColor(0, 0, 255);
    timer = millis();
    etape = 3;
  }
}

void afficherTexteSy(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Traitement #2"));
    lcd.setCursor(0,1); lcd.print(F("Questions symptomes"));
    lcd.setCursor(0,2); lcd.print(F("de fatigue oculaire."));
    lcd.setCursor(0,3); lcd.print(F("Consentement valide?"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("En acceptant:"));
    lcd.setCursor(0,1); lcd.print(F("on vous posera"));
    lcd.setCursor(0,2); lcd.print(F("des questions sur"));
    lcd.setCursor(0,3); lcd.print(F("symptomes ressentis."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Choisissez: "));
    lcd.setCursor(0,1); lcd.print(F("   Gauche | Droite "));
    lcd.setCursor(0,2); lcd.print(F("      OUI | NON "));
    setColor(0, 0, 255);
}

void afficherTexte5(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("L'analyse est finie:"));
    lcd.setCursor(0,1); lcd.print(F("un mode optionnel"));
    lcd.setCursor(0,2); lcd.print(F("vous est propose!"));
    lcd.setCursor(0,3); lcd.print(F("Consentement valide?"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("En acceptant:"));
    lcd.setCursor(0,1); lcd.print(F("+ d'infos prevention"));
    lcd.setCursor(0,2); lcd.print(F("et rappels de pause"));
    lcd.setCursor(0,3); lcd.print(F("toutes les 20 min."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Lorsque active:"));
    lcd.setCursor(0,1); lcd.print(F("debrancher les fils"));
    lcd.setCursor(0,2); lcd.print(F("USB pour arreter"));
    lcd.setCursor(0,3); lcd.print(F("l'appareil direct."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Choisissez: "));
    lcd.setCursor(0,1); lcd.print(F("   Gauche | Droite "));
    lcd.setCursor(0,2); lcd.print(F("      OUI | NON "));
    setColor(0, 0, 255);
}

void afficherTexte6(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Merci d'avoir"));
    lcd.setCursor(0,1); lcd.print(F("collabore! Pour"));
    lcd.setCursor(0,2); lcd.print(F("se renseigner sur"));
    lcd.setCursor(0,3); lcd.print(F("fatigue oculaire,"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Aller visiter mon"));
    lcd.setCursor(0,1); lcd.print(F("site web documentant"));
    lcd.setCursor(0,2); lcd.print(F("la fatigue oculaire:"));
    lcd.setCursor(0,3); lcd.print(F("[VisuoDetec]"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("D'autres fiables:"));
    lcd.setCursor(0,1); lcd.print(F("1.[ACO]"));
    lcd.setCursor(0,2); lcd.print(F("2.[OOQ]"));
    lcd.setCursor(0,3); lcd.print(F("3.[Index Sante]"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Par contre, si"));
    lcd.setCursor(0,1); lcd.print(F("symptomes s'aggravent"));
    lcd.setCursor(0,2); lcd.print(F("ou autres questions"));
    lcd.setCursor(0,3); lcd.print(F("sur fatigue oculaire"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("il est recommende"));
    lcd.setCursor(0,1); lcd.print(F("de prioriser la"));
    lcd.setCursor(0,2); lcd.print(F("consultation d'un"));
    lcd.setCursor(0,3); lcd.print(F("pro-sante oculaire."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("C'est fini!"));
    lcd.setCursor(0,1); lcd.print(F("Deconnectez fils"));
    lcd.setCursor(0,2); lcd.print(F("branches a l'ordi"));
    lcd.setCursor(0,3); lcd.print(F("(eteindre appareil)."));
}

void symptomes1(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Preparez vous"));
    lcd.setCursor(0,1); lcd.print(F("a repondre a"));
    lcd.setCursor(0,2); lcd.print(F("questions sur VOS"));
    lcd.setCursor(0,3); lcd.print(F("symptomes ressentis."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Celles-ci sont liees"));
    lcd.setCursor(0,1); lcd.print(F("a la fatigue"));
    lcd.setCursor(0,2); lcd.print(F("oculaire causee par"));
    lcd.setCursor(0,3); lcd.print(F("l'usage d'ecrans."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Notez de 1-5"));
    lcd.setCursor(0,1); lcd.print(F("la severite et"));
    lcd.setCursor(0,2); lcd.print(F("frequence des"));
    lcd.setCursor(0,3); lcd.print(F("symptomes nommes."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("- 3 QUESTIONS -"));
    lcd.setCursor(0,1); lcd.print(F("A quel point"));
    lcd.setCursor(0,2); lcd.print(F("vous ressentez l'un"));
    lcd.setCursor(0,3); lcd.print(F("de ces symptomes:"));
    delay(2000);
    lcd.clear();
}

void modeTraitements(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Bienvenue au mode"));
    lcd.setCursor(0,1); lcd.print(F("optionnel extra! 2"));
    lcd.setCursor(0,2); lcd.print(F("traitements fatigue"));
    lcd.setCursor(0,3); lcd.print(F("oculaire proposes:"));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Toutefois, ceux-ci"));
    lcd.setCursor(0,1); lcd.print(F("peuvent demande une"));
    lcd.setCursor(0,2); lcd.print(F("prescription oblige"));
    lcd.setCursor(0,3); lcd.print(F("par un optometriste."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Avant de chercher"));
    lcd.setCursor(0,1); lcd.print(F("/tenter ces moyens"));
    lcd.setCursor(0,2); lcd.print(F(", il est primordial"));
    lcd.setCursor(0,3); lcd.print(F("de se renseigner."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("1.Gouttes ophtalmo."));
    lcd.setCursor(0,1); lcd.print(F("(Collyre prescrit)"));
    lcd.setCursor(0,2); lcd.print(F("Maximum 4x/jour."));
    lcd.setCursor(0,3); lcd.print(F("Classique:1-2/usage."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("2.Port de lunettes"));
    lcd.setCursor(0,1); lcd.print(F("(Si trouble visuel)"));
    lcd.setCursor(0,2); lcd.print(F("Si c'est prescrit:"));
    lcd.setCursor(0,3); lcd.print(F("important a suivre."));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Passons aux RAPPELS!"));
    lcd.setCursor(0,1); lcd.print(F("Chaque 20 min, faites")); 
    // Démontrer nb minutes restantes
    lcd.setCursor(0,2); lcd.print(F("une pause des ecrans"));
    lcd.setCursor(0,3); lcd.print(F("et suiver le guide."));
    delay(10000);
    lcd.clear();
}

void modeAttente(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("ATTENTE!"));
    lcd.setCursor(0,1); lcd.print(F("Prochaine pause"));
    lcd.setCursor(0,2); lcd.print(F("dans moins de"));
    lcd.setCursor(0,3); lcd.print(F("20 minutes!"));
}

void modeRappel(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("PAUSE!")); 
    lcd.setCursor(0,1); lcd.print(F("PAUSE!"));
    lcd.setCursor(0,2); lcd.print(F("PAUSE!"));
    lcd.setCursor(0,3); lcd.print(F("PAUSE!"));
    jouerMelodieVIC3();
    delay(8000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("1.Regarder durant"));
    lcd.setCursor(0,1); lcd.print(F("20 sec a 20 pieds"));
    lcd.setCursor(0,2); lcd.print(F("(6m), clignez des"));
    lcd.setCursor(0,3); lcd.print(F("yeux plusieurs fois."));
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("2.Eloignez-vous des"));
    lcd.setCursor(0,1); lcd.print(F("ecrans. Etirez-vous"));
    lcd.setCursor(0,2); lcd.print(F("et hydratez-vous."));
    lcd.setCursor(0,3); lcd.print(F("Regler l'eclairage."));
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("3.Effectuez vos"));
    lcd.setCursor(0,1); lcd.print(F("exercices orthoptie:"));
    lcd.setCursor(0,2); lcd.print(F("rotation yeux, paume"));
    lcd.setCursor(0,3); lcd.print(F(", rapproche crayon!"));
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Fini? Continuez"));
    lcd.setCursor(0,1); lcd.print(F("ou vous etes rendu."));
    lcd.setCursor(0,2); lcd.print(F("Attente recommence"));
    lcd.setCursor(0,3); lcd.print(F("sous peu (...)!"));
    delay(5000);
}

void webcamRappel(){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("- Instructions -")); 
    lcd.setCursor(0,1); lcd.print(F("A.Premier test"));
    lcd.setCursor(0,2); lcd.print(F("B.Deuxieme test"));
    lcd.setCursor(0,3); lcd.print(F("C.Troisieme test"));
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("- Regles -")); 
    lcd.setCursor(0,1); lcd.print(F("1. Position droite"));
    lcd.setCursor(0,2); lcd.print(F("2. Distance Webcam:"));
    lcd.setCursor(0,3); lcd.print(F("3. 45cm (pas bouger)")); 
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("4. Ne pas regarder"));
    lcd.setCursor(0,1); lcd.print(F("   vers le bas"));
    lcd.setCursor(0,2); lcd.print(F("5. Restez en place")); 
    lcd.setCursor(0,2); lcd.print(F("6. Aucune distraction")); 
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("A. Placer votre")); 
    lcd.setCursor(0,1); lcd.print(F("visage devant la"));
    lcd.setCursor(0,2); lcd.print(F("Webcam (1 minute)"));
    lcd.setCursor(0,3); lcd.print(F("Ne pas la fixer.")); 
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("B. FIXEZ le"));
    lcd.setCursor(0,1); lcd.print(F("centre de la"));
    lcd.setCursor(0,2); lcd.print(F("WebCam (2 minutes)"));
    lcd.setCursor(0,3); lcd.print(F("Garder la position."));
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("C. Approchez un"));
    lcd.setCursor(0,1); lcd.print(F("crayon 10 FOIS"));
    lcd.setCursor(0,2); lcd.print(F("de votre nez"));
    lcd.setCursor(0,3); lcd.print(F("lentement (45sec)"));
    delay(4000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("   - ATTENTE -   ")); 
    lcd.setCursor(0,1); lcd.print(F("TRAITEMENT EN COURS"));
    lcd.setCursor(0,2); lcd.print(F("TRAITEMENT EN COURS"));
    lcd.setCursor(0,3); lcd.print(F("TRAITEMENT EN COURS"));
    delay(5000);
}

void jouerMelodieVIC(){
  tone(buzzerPin, 523, 150); // DO
  delay(180);
  tone(buzzerPin, 659, 150); // MI
  delay(180);
  tone(buzzerPin, 784, 150); // SOL
  delay(180);
  tone(buzzerPin, 1046, 300); // DO aigu
}

void jouerMelodieVIC2(){
  tone(buzzerPin, 523, 150); // DO
  setColor(0, 0, 255);
  delay(180);
  tone(buzzerPin, 659, 150); // MI
  setColor(0, 0, 200);
  delay(180);
  tone(buzzerPin, 784, 150); // SOL
  setColor(0, 0, 255);
  delay(180);
  tone(buzzerPin, 1046, 300); // DO aigu
  setColor(0, 0, 0);
}

void jouerMelodieVIC3(){
  tone(buzzerPin, 523, 150); // DO
  setColor(255, 0, 0);
  delay(180);
  tone(buzzerPin, 659, 150); // MI
  setColor(200, 0, 0);
  delay(180);
  tone(buzzerPin, 784, 150); // SOL
  setColor(255, 0, 0);
  delay(180);
  tone(buzzerPin, 1046, 300); // DO aigu
  setColor(0, 0, 0);
}

void jouerMelodieECHEC(){
  tone(buzzerPin, 600);
  delay(150);
  tone(buzzerPin, 400);
  delay(250);
  noTone(buzzerPin);
}

void jouerMelodieMID(){
  tone(buzzerPin, 900);
  delay(120);
  noTone(buzzerPin);
  delay(80);
  tone(buzzerPin, 900);
  delay(120);
  noTone(buzzerPin);
}

void jouerMelodieRESET(){
  tone(buzzerPin, 523, 200); 
  setColor(255, 0 , 0);
  delay(250);
  tone(buzzerPin, 659, 200); 
  setColor(120, 0 , 0);
  delay(250);
  tone(buzzerPin, 784, 200);
  setColor(255, 0 , 0);
  delay(250);
  tone(buzzerPin, 1046, 350);
  setColor(120, 0 , 0);
  delay(400);
  tone(buzzerPin, 784, 150);
  setColor(255, 0 , 0);
}
