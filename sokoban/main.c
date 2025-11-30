#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#define NB_LIG 12
#define NB_COL 12
#define TAILLE 12
#define NB_D_MAX 1000
#define SOKOBAN '@'
#define SOKOBAN_SUR_CIBLE '+'
#define MUR '#'
#define CAISSE '$'
#define CAISSE_SUR_CIBLE '*'
#define CIBLE '.'
#define VIDE ' '
#define HAUT 'z'
#define GAUCHE 'q'
#define DROITE 'd'
#define BAS 's'
#define ABANDONNER 'x'
#define RECOMMENCER 'r'
#define OUI 'o'
#define NON 'n'
#define ZOOMER '+'
#define DEZOOMER '-'
#define UNDO 'u'
#define SOKOG 'g'
#define SOKOH 'h'
#define SOKOB 'b'
#define SOKOD 'd'
#define SOKOCG 'G'
#define SOKOCH 'H'
#define SOKOCB 'B'
#define SOKOCD 'D'

typedef char t_Plateau[NB_LIG][NB_COL];
typedef char t_tabDeplacement[NB_D_MAX];

/* position initiale de Sokoban */
int ligSokoInitial = 0;
int colSokoInitial = 0;

/* ==== DECLARATION FONCTIONS / PROCEDURES ==== */

int kb_hit();
void charger_partie(t_Plateau plateau, char fichier[50]);
void afficher_entete(char fichier[50], int nbDeplacements);
void afficher_plateau(t_Plateau plateau, int zoom);
void enregistrer_partie(t_Plateau plateau, char fichier[50]);
void copier_plateau(t_Plateau destination, t_Plateau source);
char retirer_sokoban(char contenuCase);
char afficher_sokoban(char contenuCase);
char afficher_caisse(char contenuCase);
char retirer_caisse(char contenuCase);
void deplacer_sokoban(t_Plateau plateau, char touche, int *ligSoko, int *colSoko, char *retourTouche);
void trouver_sokoban(t_Plateau plateau, int *ligSoko, int *colSoko);
bool gagner_partie(t_Plateau plateau);
void afficher_abandon(t_Plateau plateau, t_tabDeplacement t, int nb);
bool afficher_recommencer(t_Plateau plateau, t_Plateau plateauInitial, int *nbDeplacements, int *ligSoko, int *colSoko);
void afficher_gagner(t_Plateau plateau, char fichier[50], int nb, int zoom, t_tabDeplacement t);
void conversion_retourTouche(char *retourTouche);
void undo_deplacement(t_Plateau plateau, int *ligSoko, int *colSoko, t_tabDeplacement tableauDep, int *i);
void enregistrerDeplacements(t_tabDeplacement t, int nb, char fic[]);


/* ==== MAIN ==== */

int main() {
    char fichier[50], touche = '\0', retourTouche = 0;
    t_Plateau plateau, plateauInitial;
    t_tabDeplacement tableauDep;
    int nbDeplacements = 0, zoom = 1, ligSoko = 0, colSoko = 0, i = 0;
    bool enCours = true, abandonner = false, gagner = false;

    printf("nom fichier: ");
    scanf("%49s", fichier);

    afficher_entete(fichier, nbDeplacements);
    charger_partie(plateau, fichier);
    copier_plateau(plateauInitial, plateau);
    trouver_sokoban(plateau, &ligSoko, &colSoko);

    ligSokoInitial = ligSoko;  /* mémorisation de la position initiale une seule fois */
    colSokoInitial = colSoko;

    while (enCours) {
        system("clear");
        afficher_entete(fichier, nbDeplacements);
        afficher_plateau(plateau, zoom);

        while (kb_hit() == 0) {} // attendre qu'une touche soit pressée
        touche = getchar();
        // DETERMINATION DES ACTIONS EN FONCTION DE LA TOUCHE PRESSEE
        if (touche == HAUT || touche == BAS || touche == GAUCHE || touche == DROITE) {
            int ligAncienne = ligSoko, colAncienne = colSoko;
            deplacer_sokoban(plateau, touche, &ligSoko, &colSoko, &retourTouche); // GESTION DES DEPLACEMENTS

            if (ligAncienne != ligSoko || colAncienne != colSoko) { // ON AUNGMENTE LE NB DE DEPLACEMENT SI SOKOBAN CHANGE DE CASE
                nbDeplacements++;
                tableauDep[i++] = retourTouche;
                if (gagner_partie(plateau)) { gagner = true; enCours = false; } // VERIFIE SI LA PARTIE EST GAGNEE
            }
        } else if (touche == ABANDONNER) {
            abandonner = true; enCours = false; // ABANDON
        } else if (touche == RECOMMENCER) {
            if (afficher_recommencer(plateau, plateauInitial, &nbDeplacements, &ligSoko, &colSoko)) { i = 0; continue; } // RECOMMENCEMMENT
        } else if (touche == UNDO) {
            if (nbDeplacements > 0 && i > 0) {
                undo_deplacement(plateau, &ligSoko, &colSoko, tableauDep, &i); // UNDO (annuler deplacement)
                nbDeplacements--;
            }
        } else if (touche == ZOOMER && zoom < 3) { // ZOOMER
            zoom++;
        } else if (touche == DEZOOMER && zoom > 1) { // DEZOOMER
            zoom--; 
        }
    }
    //affichage de fin de partie
    if (abandonner)
        afficher_abandon(plateau, tableauDep, nbDeplacements);
    if (gagner)
        afficher_gagner(plateau, fichier, nbDeplacements, zoom, tableauDep);

    return 0;
}


/* ==== FONCTIONS ==== */

/* renvoie 1 si une touche est disponible, 0 sinon */
int kb_hit() {
    int unCaractere = 0;
    struct termios oldt, newt;
    int ch, oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        unCaractere = 1;
    }

    return unCaractere;
}

/* charge un plateau depuis un fichier texte */
void charger_partie(t_Plateau plateau, char fichier[50]) {
    FILE *f = fopen(fichier, "r");
    char fin;

    if (f == NULL) {
        printf("ERREUR FICHIER\n");
        exit(EXIT_FAILURE);
    }

    for (int lig = 0; lig < TAILLE; lig++) {
        for (int col = 0; col < TAILLE; col++) {
            fread(&plateau[lig][col], sizeof(char), 1, f);
        }
        fread(&fin, sizeof(char), 1, f);
    }

    fclose(f);
}

/* sauvegarde un plateau dans un fichier texte */
void enregistrer_partie(t_Plateau plateau, char fichier[50]) {
    FILE *f = fopen(fichier, "w");
    char fin = '\n';

    for (int lig = 0; lig < TAILLE; lig++) {
        for (int col = 0; col < TAILLE; col++) {
            fwrite(&plateau[lig][col], sizeof(char), 1, f);
        }
        fwrite(&fin, sizeof(char), 1, f);
    }

    fclose(f);
}

/* affiche les infos de la partie (en-tête) */
void afficher_entete(char fichier[50], int nbDeplacements) {
    printf("Partie %s \n", fichier);
    printf("q = Gauche | z = Haut | s = Bas | d = Droite\n");
    printf("x = Abandonner | r = Recommencer | u = undo (annuler déplacement)\n");
    printf("+ = Zoomer | - = Dezoomer\n");
    printf("Nombre de déplacements : %d\n", nbDeplacements);
}

/* affiche le plateau de jeu */
void afficher_plateau(t_Plateau plateau, int zoom) {
    for (int lig = 0; lig < NB_LIG * zoom; lig++) {
        printf("\n");
        for (int col = 0; col < NB_COL * zoom; col++) {
            char c = plateau[lig / zoom][col / zoom];

            /* on remplace les états "sur cible" par les symboles normaux */
            if (c == SOKOBAN_SUR_CIBLE) c = SOKOBAN;
            if (c == CAISSE_SUR_CIBLE) c = CAISSE;

            printf("%c", c);
        }
    }
}

/* copie un plateau dans un autre */
void copier_plateau(t_Plateau destination, t_Plateau source) {
    for (int lig = 0; lig < NB_LIG; lig++) {
        for (int col = 0; col < NB_COL; col++) {
            destination[lig][col] = source[lig][col];
        }
    }
}

/* enlève le Sokoban d'une case */
char retirer_sokoban(char contenuCase) {
    if (contenuCase == SOKOBAN_SUR_CIBLE) return CIBLE;
    if (contenuCase == SOKOBAN) return VIDE;
    return contenuCase;
}

/* place le Sokoban sur une case */
char afficher_sokoban(char contenuCase) {
    if (contenuCase == CIBLE) return SOKOBAN_SUR_CIBLE;
    if (contenuCase == VIDE) return SOKOBAN;
    return contenuCase;
}

/* enlève une caisse d'une case */
char retirer_caisse(char contenuCase) {
    if (contenuCase == CAISSE_SUR_CIBLE) return CIBLE;
    if (contenuCase == CAISSE) return VIDE;
    return contenuCase;
}

/* place une caisse sur une case */
char afficher_caisse(char contenuCase) {
    if (contenuCase == CIBLE) return CAISSE_SUR_CIBLE;
    if (contenuCase == VIDE) return CAISSE;
    return contenuCase;
}

/* UNDO : annule le dernier déplacement */
void undo_deplacement(t_Plateau plateau, int *ligSoko, int *colSoko, t_tabDeplacement tableauDep, int *i) {
    // *i = indice de la prochaine case libre dans tableauDep
    if (*i <= 0) {
        return; // rien à annuler
    }

    (*i)--; // on revient sur le dernier coup
    char code = tableauDep[*i];

    int dlig = 0, dcol = 0;
    int signalCaisse = 0;

    // dlig/dcol = direction de l'UNDO (inverse du coup initial)
    switch (code) {
        case SOKOH:  dlig =  1; signalCaisse = 0; break;
        case SOKOB:  dlig = -1; signalCaisse = 0; break;
        case SOKOG:  dcol =  1; signalCaisse = 0; break;
        case SOKOD:  dcol = -1; signalCaisse = 0; break;

        case SOKOCH: dlig =  1; signalCaisse = 1; break;
        case SOKOCB: dlig = -1; signalCaisse = 1; break;
        case SOKOCG: dcol =  1; signalCaisse = 1; break;
        case SOKOCD: dcol = -1; signalCaisse = 1; break;

        default: return;
    }

    int ligAct = *ligSoko;
    int colAct = *colSoko;

    int ligPre = ligAct + dlig; // ancienne position de Soko
    int colPre = colAct + dcol;

    char caseSokoAct = plateau[ligAct][colAct];
    char fondSousSoko = retirer_sokoban(caseSokoAct);

    if (!signalCaisse) {
        // UNDO sans caisse
        plateau[ligAct][colAct] = fondSousSoko;
        plateau[ligPre][colPre] = afficher_sokoban(plateau[ligPre][colPre]);

        *ligSoko = ligPre;
        *colSoko = colPre;
    } else {
        // UNDO avec caisse
        int ligCaisseApres = ligAct - dlig; // position actuelle de la caisse
        int colCaisseApres = colAct - dcol;

        char caseCaisseApres = plateau[ligCaisseApres][colCaisseApres];
        char fondSousCaisseApres = retirer_caisse(caseCaisseApres);

        plateau[ligCaisseApres][colCaisseApres] = fondSousCaisseApres;         // 1. enlever la caisse de sa position actuelle
        plateau[ligAct][colAct] = fondSousSoko;        // 2. enlever Sokoban de sa position actuelle
        plateau[ligPre][colPre] = afficher_sokoban(plateau[ligPre][colPre]);        // 3. remettre Sokoban à son ancienne position
        plateau[ligAct][colAct] = afficher_caisse(plateau[ligAct][colAct]);        // 4. remettre la caisse sur l'ancienne case de Sokoban (après le coup initial)

        *ligSoko = ligPre;
        *colSoko = colPre;
    }
}

/* gère un déplacement du Sokoban (avec ou sans caisse) */
void deplacer_sokoban(t_Plateau plateau, char touche, int *ligSoko, int *colSoko, char *retourTouche) {
    int dlig = 0, dcol = 0;

    switch (touche) {
        case HAUT:   dlig = -1; *retourTouche = SOKOH; break;
        case BAS:    dlig =  1; *retourTouche = SOKOB; break;
        case GAUCHE: dcol = -1; *retourTouche = SOKOG; break;
        case DROITE: dcol =  1; *retourTouche = SOKOD; break;
        default: return;
    }

    int ligSuiv = *ligSoko + dlig;
    int colSuiv = *colSoko + dcol;
    int ligDer  = ligSuiv + dlig;
    int colDer  = colSuiv + dcol;

    if (ligSuiv < 0 || ligSuiv >= NB_LIG || colSuiv < 0 || colSuiv >= NB_COL) return;

    char caseSuiv = plateau[ligSuiv][colSuiv];

    if (caseSuiv == MUR) return;

    if (caseSuiv == VIDE || caseSuiv == CIBLE) {
        plateau[*ligSoko][*colSoko] = retirer_sokoban(plateau[*ligSoko][*colSoko]);
        plateau[ligSuiv][colSuiv] = afficher_sokoban(plateau[ligSuiv][colSuiv]);
        *ligSoko = ligSuiv;
        *colSoko = colSuiv;
        return;
    }

    if (caseSuiv == CAISSE || caseSuiv == CAISSE_SUR_CIBLE) {
        if (ligDer < 0 || ligDer >= NB_LIG || colDer < 0 || colDer >= NB_COL) return;

        char caseDer = plateau[ligDer][colDer];

        if (caseDer == MUR || caseDer == CAISSE || caseDer == CAISSE_SUR_CIBLE) return;

        /* on marque que ce déplacement poussait une caisse */
        conversion_retourTouche(retourTouche);

        plateau[ligDer][colDer]   = afficher_caisse(caseDer);
        plateau[ligSuiv][colSuiv] = retirer_caisse(plateau[ligSuiv][colSuiv]);
        plateau[*ligSoko][*colSoko] = retirer_sokoban(plateau[*ligSoko][*colSoko]);
        plateau[ligSuiv][colSuiv] = afficher_sokoban(plateau[ligSuiv][colSuiv]);
        *ligSoko = ligSuiv;
        *colSoko = colSuiv;
    }
}

/* recherche la position du Sokoban sur le plateau */
void trouver_sokoban(t_Plateau plateau, int *ligSoko, int *colSoko) {
    for (int lig = 0; lig < NB_LIG; lig++) {
        for (int col = 0; col < NB_COL; col++) {
            if (plateau[lig][col] == SOKOBAN || plateau[lig][col] == SOKOBAN_SUR_CIBLE) {
                *ligSoko = lig;
                *colSoko = col;
            }
        }
    }
}

/* teste si la partie est gagnée (toutes les cibles ont une caisse) */
bool gagner_partie(t_Plateau plateau) {
    for (int lig = 0; lig < NB_LIG; lig++) {
        for (int col = 0; col < NB_COL; col++) {
            if (plateau[lig][col] == CIBLE || plateau[lig][col] == SOKOBAN_SUR_CIBLE) {
                return false;
            }
        }
    }
    return true;
}

/* gère l'abandon : propose de sauvegarder */
void afficher_abandon(t_Plateau plateau, t_tabDeplacement t, int nb) {
    char fichier[50], fic[50], rep;


    printf("Partie abandonnée\n");
    printf("Sauvegarder ? (o/n) : ");
    scanf(" %c", &rep);

    if (rep == OUI) {
        printf("Nom fichier : ");
        scanf("%49s", fichier);
        enregistrer_partie(plateau, fichier);
    }

    printf("enregistrer déplacements dans un fichier .dep ? (o/n)");
    scanf(" %c", &rep);

    if (rep == OUI) {
        printf("Nom fichier (.dep) : ");
        scanf("%49s", fic);
        enregistrerDeplacements(t, nb, fic);
    }

}

/* demande si on recommence, remet le plateau et les compteurs */
bool afficher_recommencer(t_Plateau plateau, t_Plateau plateauInitial, int *nbDeplacements, int *ligSoko, int *colSoko) {
    char rep;

    printf("Recommencer ? (o/n) : ");
    scanf(" %c", &rep);

    if (rep == OUI) {
        copier_plateau(plateau, plateauInitial);
        *nbDeplacements = 0;
        *ligSoko = ligSokoInitial;
        *colSoko = colSokoInitial;
        return true;
    }

    return false;
}

/* affiche l'écran final de victoire avec le plateau */
void afficher_gagner(t_Plateau plateau, char fichier[50], int nb, int zoom, t_tabDeplacement t) {
    char fic[50], rep;

    system("clear");
    afficher_entete(fichier, nb);
    afficher_plateau(plateau, zoom);
    printf("\nVICTOIRE !\n");

    printf("enregistrer déplacements dans un fichier .dep ? (o/n)");
    scanf(" %c", &rep);

    if (rep == OUI) {
        printf("Nom fichier (.dep) : ");
        scanf("%49s", fic);
        enregistrerDeplacements(t, nb, fic);
    }
}

void conversion_retourTouche(char *retourTouche) {
    switch (*retourTouche) {
        case SOKOH: *retourTouche = SOKOCH; break;
        case SOKOB: *retourTouche = SOKOCB; break;
        case SOKOG: *retourTouche = SOKOCG; break;
        case SOKOD: *retourTouche = SOKOCD; break;
        default: break;
    }
}

void enregistrerDeplacements(t_tabDeplacement t, int nb, char fic[]){
    FILE * f;

    f = fopen(fic, "w");
    fwrite(t,sizeof(char), nb, f);
    fclose(f);
}

