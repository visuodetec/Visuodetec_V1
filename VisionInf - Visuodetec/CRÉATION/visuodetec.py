import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
import matplotlib.pyplot as plt
import time
import numpy as np
import serial
import threading
from flask import Flask, jsonify
from flask_cors import CORS

# Comptage des indicateurs/indices visuels
CLIGNEMENT_count = 0
CLIGNEMENT600ms_count = 0
INTERC_count = 0
INTERC_normal_count = []
SACCADES_count = 0
SACCADES_REG_count = 0
SACCADES_REG_deux_count = 0
FIXATIONS_count = 0
FIXATIONS_count_deux = 0
CONVERGENCE_count = 0
Moyenne_convergence = []
score_instabilité = 0

# Configuration Serial
ser = serial.Serial(port='/dev/tty.usbserial-AR0KFRXX', baudrate=9600, timeout=1) 
time.sleep(2) # IMPORTANT: laisser Arduino reset

# Configuration MediaPipe Tasks
BaseOptions = mp.tasks.BaseOptions
FaceLandmarker = mp.tasks.vision.FaceLandmarker
FaceLandmarkerOptions = mp.tasks.vision.FaceLandmarkerOptions
VisionRunningMode = mp.tasks.vision.RunningMode

# Initialisation du détecteur (nécessite face_landmarker.task dans le dossier) avec mode vidéo
options = FaceLandmarkerOptions(
    base_options=BaseOptions(model_asset_path='face_landmarker.task'),
    running_mode=VisionRunningMode.VIDEO,
    num_faces=1)

data_to_send = { 
}

def opencv_thread():
    app = Flask(__name__)
    CORS(app, origins=["http://127.0.0.1:5500"])

    @app.route("/") # Route pour le serveur
    def data(): # Information (data) à transférer
        return jsonify(data_to_send)    

    app.run(debug=False, use_reloader = False, port=5000)

threading.Thread(target=opencv_thread).start()

# Le face landmarker est créé et initialisé avec le mode vidéo
with FaceLandmarker.create_from_options(options) as landmarker: 
    # Variable pour l'analyse
    print_error = False

    prev_pos = None
    blink_start = 0
    is_blinking = False
    Debut_fixation = 0
    is_saccade = False
    Interval_blink = 0
    intervalle_prise = False
    Convergence_Compté = False
    avertissement_envoyé = False

    fixation_values = []
    fixation_values_two = []
    fixation_values_three = []
    Fix_fréquence_values = []

    ear_values = []
    variation_values = []
    frame_numbers = []
    frame_count = 0
    max_frames = 100
    EAR_THRESHOLD = 30

    Clignement_FAIT = False
    Saccades_FAIT = True 
    Vergence_FAIT = True
    distance_repos = None
    bilan_fixations = False

    Fini = True

    # Variables pour le temps et les séquences 
    bilan_fait = False
    bilan_fait_two = False
    bilan_fait_three = False
    analyse_fini = False
    cap = None
    running = False
    score_final = 0

    while True:
        if ser.in_waiting > 0: # Retourne le nombre de bytes (data) qui attendent d'êtres lus dans le serial
            msg =  ser.readline().decode().strip() # Lis le message envoyée par Serial.println("START") sur le serial à partir de l'arduino
            
            if msg == "START" and not running: # Si le message lu est une annonçe String "Start", ouvrir la caméra.
                running = True
                if cap is None:
                    cap = cv2.VideoCapture(0) # Capturer la vidéo dès que le message est reçu
                    EXPERIENCE_START = time.monotonic()

        if not running:
            time.sleep(0.01) # Pause et retourne la boucle
            continue

        ret, frame = cap.read()
        if not ret:
            print("Frame non lue")
            continue

        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape

        # Conversion de l'image frame reçu par OpenCV en objet Image Mediapipe valide
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)) 
        
        # Détection synchrone pour vidéo et horodatage (frame en ms) (exécution de la tâche)
        detection_result = landmarker.detect_for_video(mp_image, int(time.time() * 1000))

        if detection_result.face_landmarks:
            landmarks = detection_result.face_landmarks[0]

            # COORDONNÉES DES PUPILLES
            pupil_left_idx = 468    # Centre iris gauche
            pupil_right_idx = 473   # Centre iris droit
            joue_left_idx = 234
            joue_right_idx = 454

            pupil_left = np.array([landmarks[pupil_left_idx].x * w, landmarks[pupil_left_idx].y * h])
            pupil_right = np.array([landmarks[pupil_right_idx].x * w, landmarks[pupil_right_idx].y * h])
            joue_left = np.array([landmarks[joue_left_idx].x * w, landmarks[joue_left_idx].y * h])
            joue_right = np.array([landmarks[joue_right_idx].x * w, landmarks[joue_right_idx].y * h])

            #1. CLIGNEMENT (EAR) ---------------------------------
            if not Clignement_FAIT : 
                try:
                    cv2.putText(frame, "A) CLIGNEMENTS", (1350, 100), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 0, 255), 5)
                    cv2.putText(frame, "1. Clignement (Garder visage droit - 1min)", (30, 750), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                    cv2.putText(frame, "- Visage detendu vers l'avant", (30, 850), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                    cv2.putText(frame, "- Rester a 45cm de la camera", (30, 950), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                    Temps_passé = time.monotonic() - EXPERIENCE_START
                    cv2.putText(frame, f"- TEMPS PASSE: {Temps_passé:.2f} sec", (30, 1050), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                    
                    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)                    
                    timestamp = int(time.time() * 1000)
            
                    face_width = np.linalg.norm(joue_left - joue_right) # Distance de la largeur du visage (proportion)
                    if int(face_width) < 500 or int(face_width) > 600:
                        cv2.putText(frame, "AJUSTEZ VOTRE POSITION!", (30, 200), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 0, 255), 3)
                        avertissement_envoyé = False
                        if not avertissement_envoyé:
                            score_instabilité += 1
                            avertissement_envoyé = True
                    if int(face_width) < 500:
                        cv2.putText(frame, "- AVANCEZ -", (30, 325), cv2.FONT_HERSHEY_SIMPLEX, 2.5, (0, 0, 255), 3)
                    if int(face_width) > 600:
                        cv2.putText(frame, "- RECULEZ -", (30, 325), cv2.FONT_HERSHEY_SIMPLEX, 2.5, (0, 0, 255), 3)

                    #Détection durée des clignements (EAR)
                    ear = abs(landmarks[159].y - landmarks[145].y) # Déterminer ouverture de l'oeil
                    if ear >= 0.015:
                        if not intervalle_prise:
                            inter_counted = False
                            Interval_blink = time.monotonic() # Add a true/false condition
                            intervalle_prise = True
                    if ear < 0.015:
                        if not is_blinking:
                            CLIGNEMENT_count += 1 # Compter un clignement
                            data_to_send["Clignements_normaux"] = CLIGNEMENT_count
                            duree_deux = (time.monotonic() - Interval_blink) * 1000 # Intervalle inter-clignement (temps)
                            INTERC_normal_count.append(duree_deux)
                            if 1500 >= duree_deux and not inter_counted:
                                if not inter_counted:
                                    INTERC_count += 1
                                    data_to_send["Inter-clignement_irreguliers"] = INTERC_count
                                    inter_counted = True
                                    intervalle_prise = False
                            blink_start = time.time() # Marque du début de durée  
                            is_blinking = True # Loop et passe à l'étape de durée clignement
                    else:
                        if is_blinking:
                            duree = (time.time() - blink_start) * 1000 #Compte la durée en ms (temps actuel - temps marqué au début)
                            if 1000 > duree > 600: # Compte les clignements entre 600 et 1000 ms
                                cv2.putText(frame, "CLIGNEMENT > 600ms!", (30, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                                CLIGNEMENT600ms_count += 1
                                if CLIGNEMENT600ms_count > 0:
                                    data_to_send["Clignements_600ms"] = CLIGNEMENT600ms_count
                                elif CLIGNEMENT600ms_count < 1:
                                    CLIGNEMENT600ms_count = 0
                                    data_to_send["Clignements_600ms"] = CLIGNEMENT600ms_count

                                print(f"Clignement : {int(duree)}ms | Intervalle: {int(duree_deux)}ms | Nb clignements : {CLIGNEMENT_count} |Nb (600) clignements : {CLIGNEMENT600ms_count}| Inter-clignement irréguliers : {INTERC_count} | Temps passé : {Temps_passé:.2f}sec | FACE_WIDTH: {face_width:.2f}")                        
                                is_blinking = False
                                intervalle_prise = False
                            else:
                                cv2.putText(frame, "CLIGNEMENT!", (30, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                                print(f"Clignement : {int(duree)}ms | Intervalle: {int(duree_deux)}ms | Nb clignements : {CLIGNEMENT_count} |Nb (600) clignements : {CLIGNEMENT600ms_count}| Inter-clignement irréguliers : {INTERC_count} | Temps passé : {Temps_passé:.2f}sec | FACE_WIDTH: {face_width:.2f}")                        
                                is_blinking = False
                                intervalle_prise = False
                    
                    if not bilan_fait and (time.monotonic() - EXPERIENCE_START) >= 60: # Après 1 minute (60 secondes) - Bilan
                        if len(INTERC_normal_count) > 0:
                            Pourcentage_interclignement = ((INTERC_count / len(INTERC_normal_count)) * 100)
                        else: 
                            Pourcentage_interclignement = 0.0
                        data_to_send["Pourcentage_inter-clign"] = f"{round(Pourcentage_interclignement)}"
                        print(f"1. | A) Nb (600) clignements: {CLIGNEMENT600ms_count}| B) Inter-clignement irréguliers : {INTERC_count} / POURCENTAGE INT: {Pourcentage_interclignement:.2f}% | Nb clignements: {CLIGNEMENT_count} | LIST: {len(INTERC_normal_count)}") 
                        if CLIGNEMENT600ms_count > 3: # 1 = Normal/occasionnel ; 2 = possible hasard ; 3 = mauvais hasard ; 4+ = tendance remarquable
                            score_final += 1
                        elif Pourcentage_interclignement > 25:  # Comptabiliser 25% et plus des intervalles sont irrégulières
                            score_final += 1
                        bilan_fait = True
                        Clignement_FAIT = True
                        Saccades_FAIT = False
                        EXPERIENCE_START_TWO = time.monotonic()
                
                except ZeroDivisionError as err:
                    if not print_error:
                        print(f"{err}: aucun clignement n'a été détecté!")
                        print_error = True

            # 2. FIXATIONS ET SACCADES ---------------------------------
            if not Saccades_FAIT:
                cv2.putText(frame, "B) FIXATIONS", (1350, 100), cv2.FONT_HERSHEY_SIMPLEX, 2, (255, 0, 0), 5)
                cv2.putText(frame, "2. Fixations (Garder le visage droit)", (30, 750), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                cv2.putText(frame, "- Liser les textes sur l'ecran LCD (1 min)", (30, 850), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                cv2.putText(frame, "- Visage detendu vers l'avant", (30, 950), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                Temps_passé_two = time.monotonic() - EXPERIENCE_START_TWO
                cv2.putText(frame, f"- Rester a 45cm de la camera | TEMPS PASSE: {Temps_passé_two:.2f} sec", (30, 1050), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)

                face_width = np.linalg.norm(joue_left - joue_right) # Distance de la largeur du visage (proportion)
                if int(face_width) < 500 or int(face_width) > 600:
                    cv2.putText(frame, "AJUSTEZ VOTRE POSITION!", (30, 200), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 0, 255), 3)
                    avertissement_envoyé = False
                    if not avertissement_envoyé:
                        score_instabilité += 1
                        avertissement_envoyé = True
                if int(face_width) < 500:
                    cv2.putText(frame, "- AVANCEZ -", (30, 325), cv2.FONT_HERSHEY_SIMPLEX, 2.5, (0, 0, 255), 3)
                if int(face_width) > 600:
                    cv2.putText(frame, "- RECULEZ -", (30, 325), cv2.FONT_HERSHEY_SIMPLEX, 2.5, (0, 0, 255), 3)

                # AXE DES PUPILLES
                pupil = landmarks[468] # (Point 468 = Centre de l'iris gauche)
                px, py, pz = int(pupil.x * w), int(pupil.y * h), pupil.z
                pupil_b = landmarks[473] # (Point 468 = Centre de l'iris gauche)
                px_b, py_b, pz_b = int(pupil_b.x * w), int(pupil_b.y * h), pupil_b.z

                # AFFICHAGE
                cv2.circle(frame, (px, py), 4, (255, 0, 0), -1)
                cv2.circle(frame, (px_b, py_b), 4, (255, 0, 0), -1)

                # DÉTECTION DES SACCADES 
                curr_pos = np.array([px, py])
                if prev_pos is not None:
                    ear = abs(landmarks[159].y - landmarks[145].y) # Déterminer ouverture de l'oeil
                    if ear >= 0.025:
                        vitesse = np.linalg.norm(curr_pos - prev_pos) # Amplitude de déplacement de l'oeil
                        if vitesse >= 1.15: # Seuil de saccade (5-6 secondes de sensitivité)
                            cv2.putText(frame, "SACCADE!", (30, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                            SACCADES_count += 1 # Compte des saccades (déviation du Z-index, de la position de la pupille)
                        if vitesse >= 3.5: # 3-4
                            cv2.putText(frame, "SACCADE REGRESSIVE!", (30, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                            if 30 >= (time.monotonic() - EXPERIENCE_START_TWO) >= 0:
                                SACCADES_REG_count += 1 # Compte des saccades (déviation du Z-index, de la position de la pupille)
                            if 60 >= (time.monotonic() - EXPERIENCE_START_TWO) > 30: 
                                SACCADES_REG_deux_count += 1
                        if vitesse < 1.15:
                                if not is_saccade: # Fixation
                                    Debut_fixation = time.monotonic() # Add a true/false condition
                                    is_saccade = True
                                    # C’est ce que font la plupart des études d’asténopie : la durée et le nombre de fixations sont plus fiables que le comptage direct de micro-saccades.
                        else: 
                            if is_saccade:
                                duree_trois = (time.monotonic() - Debut_fixation) * 1000
                                print(f'| EAR: {ear:.2f} | Temps passé: {Temps_passé_two:.2f} sec | {int(duree_trois)} ms de fixation! | (30s) - Nb fixations: {FIXATIONS_count} / (60s) - Nb fixations: {FIXATIONS_count_deux} | FACE_WIDTH: {face_width:.2f} |') 
                                is_saccade = False
                                if 30 >= (time.monotonic() - EXPERIENCE_START_TWO) >= 0: # Moyenne #1
                                    FIXATIONS_count += 1
                                    fixation_values.append(duree_trois)
                                    fixation_moy = sum(fixation_values) / len(fixation_values)
                                if 60 >= (time.monotonic() - EXPERIENCE_START_TWO) > 30: # Moyenne #2
                                    FIXATIONS_count_deux += 1
                                    fixation_values_three.append(duree_trois)
                                    fixation_moy_three = sum(fixation_values_three) / len(fixation_values_three)

                prev_pos = curr_pos 
            
                if not bilan_fait_two and (time.monotonic() - EXPERIENCE_START_TWO) > 60: # Après 1 minute (60 secondes) - Bilan                                          
                        pourcentage = ((fixation_moy_three / fixation_moy) - 1) * 100
                        data_to_send["Pourcentage_duree-fix"] = int(pourcentage)
                        m = ((fixation_moy_three - fixation_moy) / 60) # X: temps écoulé en 1 minutes (60 secondes) Y: Durée des fixations (ms)
                        data_to_send["Pente_duree-fix"] = f"{m:.2f}"
                        pourcentage_deux = ((FIXATIONS_count_deux / FIXATIONS_count) - 1) * 100
                        data_to_send["Frequence_nb_fix"] = round(pourcentage_deux)
                        pourcentage_trois = ((SACCADES_REG_deux_count / SACCADES_REG_count) - 1) * 100
                        data_to_send["Frequence_nb_reg"] = round(pourcentage_trois)

                        print(f"2. | A) DURÉE --- POURCENTAGE: {int(pourcentage)}% / PENTE: {m:.2f} / Durée (moy): {fixation_moy:.2f} & {fixation_moy_three:.2f} | B) FRÉQUENCE (NB - FIX) -- POURCENTAGE (30s / 60s): {int(pourcentage_deux)}% | C) FRÉQUENCE (NB - RÉG) -- POURCENTAGE (30s / 60s): {int(pourcentage_trois)}% ; Nb saccades régressives: {SACCADES_REG_count}")
                        if m > 0 and pourcentage > 0: # 1) Durée des fixations augmentant
                            score_final += 1
                        if pourcentage_deux > 0: # 2) Fréquence (nb) des fixations augmentant
                            score_final += 1
                        if pourcentage_trois > 0: # 3) Fréquence (nb) des régressions augmentant
                            score_final += 1
                        bilan_fait_two = True
                        Saccades_FAIT = True
                        Vergence_FAIT = False
                        EXPERIENCE_START_THREE = time.monotonic()

            # 3. CONVERGENCE BINOCULAIRE ---------------------------------
            if not Vergence_FAIT:
                cv2.putText(frame, "C) CONVERGENCE", (1350, 100), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 5)
                cv2.putText(frame, "3. Convergence (Rester a 45cm de camera)", (30, 750), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                cv2.putText(frame, "- Approchez votre crayon/doigts 10 FOIS", (30, 850), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                cv2.putText(frame, "- Lentement, toucher le nez avec", (30, 950), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                Temps_passé_three = time.monotonic() - EXPERIENCE_START_THREE
                cv2.putText(frame, f"- Analyse: 45 secondes | - TEMPS PASSE: {Temps_passé_three:.2f} sec", (30, 1050), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)

                face_width = np.linalg.norm(joue_left - joue_right) # Distance de la largeur du visage (proportion)

                if int(face_width) < 500 or int(face_width) > 600:
                    cv2.putText(frame, "AJUSTEZ VOTRE POSITION!", (30, 200), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 0, 255), 3)
                    avertissement_envoyé = False
                    if not avertissement_envoyé:
                        score_instabilité += 1
                        avertissement_envoyé = True
                    if int(face_width) < 500:
                        cv2.putText(frame, "- AVANCEZ -", (30, 325), cv2.FONT_HERSHEY_SIMPLEX, 2.5, (0, 0, 255), 3)
                    if int(face_width) > 600:
                        cv2.putText(frame, "- RECULEZ -", (30, 325), cv2.FONT_HERSHEY_SIMPLEX, 2.5, (0, 0, 255), 3)

                # === DISTANCE ENTRE PUPILLES
                distance = (np.linalg.norm(pupil_left - pupil_right) / face_width) # Distance proportionnelle à la largeur du visage (algèbre linéaire)
                if distance_repos is None:
                    distance_repos = distance # Distance des pupilles aux repos (premier aperçu)
                    print(f"Distance pupilles au repos : {distance_repos:.2f} px") 

                deviation = distance_repos - distance  # Plus la distance diminue, plus la convergence binoculaire augmente. Variation de la distance inter-pupillaire par rapport à celle au repos.
                deviation_percent = (deviation / distance_repos) * 100 # Pourcentage de déviation (variation relative: diminution en % de sa taille d'origine)

                if deviation_percent > 8: # and not Pause_Convergence
                    if not Convergence_Compté:
                        Convergence_Compté = True
                        duree_quatre = time.monotonic()
                    else:
                        if Convergence_Compté and (time.monotonic() - duree_quatre) > 1:
                            CONVERGENCE_count += 1
                            data_to_send["Convergence_Count"] = CONVERGENCE_count
                            Moyenne_convergence.append(deviation_percent)         
                            print(f"Convergence réussite : {deviation_percent:.2f}% ! | FACE_WIDTH: {face_width:.2f}") 
                            Convergence_Compté = False

                # AFFICHAGE
                pupil = landmarks[468] # (Point 468 = Centre de l'iris gauche)
                px, py, pz = int(pupil.x * w), int(pupil.y * h), pupil.z
                pupil_b = landmarks[473] # (Point 468 = Centre de l'iris gauche)
                px_b, py_b, pz_b = int(pupil_b.x * w), int(pupil_b.y * h), pupil_b.z

                cv2.circle(frame, (px, py), 4, (255, 0, 0), -1)
                cv2.circle(frame, (px_b, py_b), 4, (255, 0, 0), -1)
                cv2.putText(frame, f"Distance: {distance:.1f}px", (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 1)
                cv2.putText(frame, f"Deviation: {deviation_percent:.1f}%", (30, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)

                if not bilan_fait_three and (time.monotonic() - EXPERIENCE_START_THREE) > 45: # Après 45 secondes - Bilan    
                    try:
                        if CONVERGENCE_count < 8:
                            score_final += 1
                            print(f"Convergence non réussite : {CONVERGENCE_count} < 8 ") 
                        else:
                            print(f"Convergence réussite : {CONVERGENCE_count} >= 8 ")     
                        if len(Moyenne_convergence) > 0:
                            Pourcentage_Convergence = sum(Moyenne_convergence)/len(Moyenne_convergence)
                        else:
                            Pourcentage_Convergence = 0.0
                        print(f"Pourcentage de convergence réussites(moyenne): {Pourcentage_Convergence:.2f}")
                        data_to_send["Pourcentage_convergence"] = f"{Pourcentage_Convergence:.2f}"
                        bilan_fait_three = True
                        Vergence_FAIT = True
                        print(f"\nSCORE: {score_final}")
                        print(f"\nINSTABILITÉ: {score_instabilité} fois avertis!")
                        Fini = False  

                    except ZeroDivisionError as err:
                        if not print_error:
                            print(f"{err}: aucune (0) convergence n'a été détecté!")
                            Fini = False
                            print_error = True      

            # FIN ---------------------------------
            if not Fini:
                data_to_send["Score_final"] = score_final
                analyse_fini = True
          
            cv2.imshow("Analyse Fatigue - Version Tasks", frame)
            if cv2.waitKey(1) & 0xFF == 27 or analyse_fini: 
                break

    cap.release()
    cv2.destroyAllWindows()

    # Retour des données récoltées après l'analyse (Pyserial - Arduino)

    ser.write(f"SCORE:{score_final}\n".encode()) # Renvoie la valeur (write) le score final avec un f string (pour insérer variable) et l'encode (string à bytes)
    ser.flush() # assure que tout est envoyé
    time.sleep(0.05) # 50ms pour que l’Arduino ait le temps de lire
    print("Score envoyé:", f"SCORE:{score_final}") # Test

    print("Analyse terminée: serveur toujours actif.")
    while True:
        time.sleep(1)
