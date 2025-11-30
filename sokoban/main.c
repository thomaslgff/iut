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
typedef char t_deplacements[NB_D_MAX];

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
char retirer_sokoban(char contenu_case);
char afficher_sokoban(char contenu_case);
char afficher_caisse(char contenu_case);
char retirer_caisse(char contenu_case);
void deplacer_sokoban(t_Plateau plateau, char touche, int *ligSoko, int *colSoko, char *retourTouche);
void trouver_sokoban(t_Plateau plateau, int *ligSoko, int *colSoko);
bool gagner_partie(t_Plateau plateau);
void afficher_abandon(t_Plateau plateau);
bool afficher_recommencer(t_Plateau plateau, t_Plateau plateauInitial, int *nbDeplacements, int *ligSoko, int *colSoko);
void afficher_gagner(t_Plateau plateau, char fichier[50], int nbDeplacements, int zoom);
void conversion_retourtouche(char *retourTouche);
void undo_deplacement(t_Plateau plateau, int *ligSoko, int *colSoko, t_deplacements tableauDep, int *i);

/* ==== MAIN ==== */

int main(){
    char fichier[50];
    t_Plateau plateau, plateauInitial;
    t_deplacements tableauDep;
    int nbDeplacements = 0, zoom = 1;
    int ligSoko = 0, colSoko = 0;
    int i = 0;
    bool enCours = true, abandonner = false, gagner = false; 
    char touche = '\0';
    char retourTouche;
    

    /* Chargement de la partie */
    printf("nom fichier: ");
    scanf("%49s", fichier);

    afficher_entete(fichier, nbDeplacements);
    charger_partie(plateau, fichier);
    copier_plateau(plateauInitial, plateau);
    trouver_sokoban(plateau, &ligSoko, &colSoko);

    /* mémorisation de la position initiale une seule fois */
    ligSokoInitial = ligSoko;
    colSokoInitial = colSoko;

    /* Boucle principale du jeu */
    while (enCours) {
        system("clear");
        afficher_entete(fichier, nbDeplacements);
        afficher_plateau(plateau, zoom);

        /* attente d'une touche clavier */
        while (kb_hit() == 0) {}
        touche = getchar();

        /* gestion des déplacements */
        if (touche == HAUT || touche == BAS || touche == GAUCHE || touche == DROITE) {
            int lig_ancienne = ligSoko;
            int col_ancienne = colSoko;

            deplacer_sokoban(plateau, touche, &ligSoko, &colSoko, &retourTouche);

            /* on compte le déplacement seulement si on a bougé */
            if (lig_ancienne != ligSoko || col_ancienne != colSoko) {
                nbDeplacements++;
                tableauDep[i] = retourTouche;
                i++;

                /* test de victoire */
                if (gagner_partie(plateau)) {
                    gagner = true;
                    enCours = false;
                }
            }
        }
        /* abandon de la partie */
        else if (touche == ABANDONNER) {
            abandonner = true;
            enCours = false;
        }
        /* recommencer depuis le début */
        else if (touche == RECOMMENCER) {
            if (afficher_recommencer(plateau, plateauInitial, &nbDeplacements, &ligSoko, &colSoko)) {
                continue;
            }
        }
        else if(touche == UNDO){
            undo_deplacement(plateau, &ligSoko, &colSoko, tableauDep, &i);
        }
        else if (touche == ZOOMER && zoom < 3)
            zoom = zoom + 1;
        else if (touche == DEZOOMER && zoom > 1)
            zoom = zoom - 1;
    }

    /* gestion de fin de partie */
    if (abandonner) {
        afficher_abandon(plateau);
    }
    if (gagner) {
        afficher_gagner(plateau, fichier, nbDeplacements, zoom);
    }
    return 0;
}

/* ==== FONCTIONS ==== */

/* renvoie 1 si une touche est disponible, 0 sinon */
int kb_hit() {
    int un_caractere = 0;
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
        un_caractere = 1;
    }

    return un_caractere;
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
    printf("Q=Gauche | Z=Haut | S=Bas | D=Droite\n");
    printf("X=Abandonner | R=Recommencer\n");
    printf("Nombre de déplacements : %d\n", nbDeplacements);
}

/* affiche le plateau de jeu */
void afficher_plateau(t_Plateau plateau, int zoom) {
    for (int lig = 0; lig < NB_LIG*zoom; lig++) {
        printf("\n");
        for (int col = 0; col < NB_COL*zoom; col++) {
            char c = plateau[lig/zoom][col/zoom];

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
char retirer_sokoban(char contenu_case) {
    if (contenu_case == SOKOBAN_SUR_CIBLE) return CIBLE;
    if (contenu_case == SOKOBAN) return VIDE;
    return contenu_case;
}

/* place le Sokoban sur une case */
char afficher_sokoban(char contenu_case) {
    if (contenu_case == CIBLE) return SOKOBAN_SUR_CIBLE;
    if (contenu_case == VIDE) return SOKOBAN;
    return contenu_case;
}

/* enlève une caisse d'une case */
char retirer_caisse(char contenu_case) {
    if (contenu_case == CAISSE_SUR_CIBLE) return CIBLE;
    if (contenu_case == CAISSE) return VIDE;
    return contenu_case;
}

/* place une caisse sur une case */
char afficher_caisse(char contenu_case) {
    if (contenu_case == CIBLE) return CAISSE_SUR_CIBLE;
    if (contenu_case == VIDE) return CAISSE;
    return contenu_case;
}

/* gère un déplacement du Sokoban (avec ou sans caisse) */
void undo_deplacement(t_Plateau plateau, int *ligSoko, int *colSoko, t_deplacements tableauDep, int *i) {
    int dlig = 0, dcol = 0, signalCais = 0;

    switch (tableauDep[*i]) {
        case SOKOH: dlig = 1; signalCais = 0; break;
        case SOKOB: dlig = -1; signalCais = 0; break;
        case SOKOG: dcol = -1; signalCais = 0; break;
        case SOKOD: dcol = 1; signalCais = 0; break;
        case SOKOCH: dlig = 1; signalCais = 1; break;
        case SOKOCB: dlig = -1; signalCais = 1; break;
        case SOKOCG: dcol = -1; signalCais = 1; break;
        case SOKOCD: dcol = 1; signalCais = 1; break;
        default: return;
    }

    int ligPre = *ligSoko + dlig;
    int colPre = *colSoko + dcol;
    int ligCais = *colSoko - dlig;
    int colCais = *ligSoko - dcol;

    char casePre = plateau[ligPre][colPre];
    char caseSoko = plateau[*ligSoko][*ligSoko];
    char caseCais = plateau[ligCais][colCais];


    plateau[*ligSoko][*colSoko] = retirer_sokoban(caseSoko);
    plateau[ligPre][colPre] = afficher_sokoban(casePre);

    if(signalCais == 1){
        plateau[ligCais][colCais] = retirer_caisse(caseCais);
        plateau[*ligSoko][*colSoko] = afficher_caisse(caseSoko);
    }
    (*i)--;    
}


void deplacer_sokoban(t_Plateau plateau, char touche, int *ligSoko, int *colSoko, char *retourTouche) {
    int dlig = 0, dcol = 0;

    switch (touche) {
        case HAUT: dlig = -1; *retourTouche = SOKOH; break;
        case BAS: dlig = 1; *retourTouche = SOKOB; break;
        case GAUCHE: dcol = 1; *retourTouche = SOKOG; break;
        case DROITE: dcol = -1; *retourTouche = SOKOD; break;
        default: return;
    }

    int lig_suiv = *ligSoko + dlig;
    int col_suiv = *colSoko + dcol;
    int lig_der = lig_suiv + dlig;
    int col_der = col_suiv + dcol;

    if (lig_suiv < 0 || lig_suiv >= NB_LIG || col_suiv < 0 || col_suiv >= NB_COL) return;

    char case_suiv = plateau[lig_suiv][col_suiv];

    if (case_suiv == MUR) return;

    if (case_suiv == VIDE || case_suiv == CIBLE) {
        plateau[*ligSoko][*colSoko] = retirer_sokoban(plateau[*ligSoko][*colSoko]);
        plateau[lig_suiv][col_suiv] = afficher_sokoban(plateau[lig_suiv][col_suiv]);
        *ligSoko = lig_suiv;
        *colSoko = col_suiv;
        return;
    }

    if (case_suiv == CAISSE || case_suiv == CAISSE_SUR_CIBLE) {
        conversion_retourtouche(*retourTouche);
        if (lig_der < 0 || lig_der >= NB_LIG || col_der < 0 || col_der >= NB_COL) return;

        char case_der = plateau[lig_der][col_der];

        if (case_der == MUR || case_der == CAISSE || case_der == CAISSE_SUR_CIBLE) return;

        plateau[lig_der][col_der] = afficher_caisse(case_der);
        plateau[lig_suiv][col_suiv] = retirer_caisse(plateau[lig_suiv][col_suiv]);
        plateau[*ligSoko][*colSoko] = retirer_sokoban(plateau[*ligSoko][*colSoko]);
        plateau[lig_suiv][col_suiv] = afficher_sokoban(plateau[lig_suiv][col_suiv]);
        *ligSoko = lig_suiv;
        *colSoko = col_suiv;
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
void afficher_abandon(t_Plateau plateau) {
    char fichier[50], rep;

    printf("Partie abandonnée\n");
    printf("Sauvegarder ? (o/n) : ");
    scanf(" %c", &rep);

    if (rep == OUI) {
        printf("Nom fichier : ");
        scanf("%49s", fichier);
        enregistrer_partie(plateau, fichier);
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
void afficher_gagner(t_Plateau plateau, char fichier[50], int nbDeplacements, int zoom) {
    system("clear");
    afficher_entete(fichier, nbDeplacements);
    afficher_plateau(plateau, zoom);
    printf("\nVICTOIRE !\n");
}
void conversion_retourtouche(char *retourTouche){
    switch (*retourTouche) {
        case SOKOH: *retourTouche = SOKOCH; break;
        case SOKOB: *retourTouche = SOKOCB; break;
        case SOKOG: *retourTouche = SOKOCG; break;
        case SOKOD: *retourTouche = SOKOCD; break;
        default: return;
    }
}
