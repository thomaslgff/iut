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
int lig_soko_initial = 0;
int col_soko_initial = 0;

/* ==== DECLARATION FONCTIONS / PROCEDURES ==== */

int kb_hit();
void charger_partie(t_Plateau plateau, char fichier[50]);
void afficher_entete(char fichier[50], int nb_deplacements);
void afficher_plateau(t_Plateau plateau, int zoom);
void enregistrer_partie(t_Plateau plateau, char fichier[50]);
void copier_plateau(t_Plateau destination, t_Plateau source);
char retirer_sokoban(char contenu_case);
char afficher_sokoban(char contenu_case);
char afficher_caisse(char contenu_case);
char retirer_caisse(char contenu_case);
void deplacer_sokoban(t_Plateau plateau, char touche, int *lig_soko, int *col_soko, char retourTouche);
void trouver_sokoban(t_Plateau plateau, int *lig_soko, int *col_soko);
bool gagner_partie(t_Plateau plateau);
void afficher_abandon(t_Plateau plateau);
bool afficher_recommencer(t_Plateau plateau, t_Plateau plateau_initial, int *nb_deplacements, int *lig_soko, int *col_soko);
void afficher_gagner(t_Plateau plateau, char fichier[50], int nb_deplacements, int zoom);
void conversion_retourtouche(char *retourTouche);

/* ==== MAIN ==== */

int main() {
    char fichier[50];
    t_Plateau plateau, plateau_initial;
    t_deplacements tableauDep[NB_D_MAX];
    int nb_deplacements = 0, zoom = 1;
    int lig_soko = 0, col_soko = 0;
    bool en_cours = true, abandonner = false, gagner = false; 
    char touche = '\0';
    char retourTouche;
    

    /* Chargement de la partie */
    printf("nom fichier: ");
    scanf("%49s", fichier);

    afficher_entete(fichier, nb_deplacements);
    charger_partie(plateau, fichier);
    copier_plateau(plateau_initial, plateau);
    trouver_sokoban(plateau, &lig_soko, &col_soko);

    /* mémorisation de la position initiale une seule fois */
    lig_soko_initial = lig_soko;
    col_soko_initial = col_soko;

    /* Boucle principale du jeu */
    while (en_cours) {
        system("clear");
        afficher_entete(fichier, nb_deplacements);
        afficher_plateau(plateau, zoom);

        /* attente d'une touche clavier */
        while (kb_hit() == 0) {}
        touche = getchar();

        /* gestion des déplacements */
        if (touche == HAUT || touche == BAS || touche == GAUCHE || touche == DROITE) {
            int lig_ancienne = lig_soko;
            int col_ancienne = col_soko;

            deplacer_sokoban(plateau, touche, &lig_soko, &col_soko, &retourTouche);

            /* on compte le déplacement seulement si on a bougé */
            if (lig_ancienne != lig_soko || col_ancienne != col_soko) {
                nb_deplacements++;
                for(int i = 0; i< NB_D_MAX; i++){
                    tableauDep[i] = retourTouche;
                }
                

                /* test de victoire */
                if (gagner_partie(plateau)) {
                    gagner = true;
                    en_cours = false;
                }
            }
        }
        /* abandon de la partie */
        else if (touche == ABANDONNER) {
            abandonner = true;
            en_cours = false;
        }
        /* recommencer depuis le début */
        else if (touche == RECOMMENCER) {
            if (afficher_recommencer(plateau, plateau_initial, &nb_deplacements, &lig_soko, &col_soko)) {
                continue;
            }
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
        afficher_gagner(plateau, fichier, nb_deplacements, zoom);
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
void afficher_entete(char fichier[50], int nb_deplacements) {
    printf("Partie %s \n", fichier);
    printf("Q=Gauche | Z=Haut | S=Bas | D=Droite\n");
    printf("X=Abandonner | R=Recommencer\n");
    printf("Nombre de déplacements : %d\n", nb_deplacements);
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
void deplacer_sokoban(t_Plateau plateau, char touche, int *lig_soko, int *col_soko, char *retourTouche) {
    int dlig = 0, dcol = 0;

    switch (touche) {
        case HAUT: dlig = -1; *retourTouche = SOKOH; break;
        case BAS: dlig = 1; *retourTouche = SOKOB; break;
        case GAUCHE: dcol = -1; *retourTouche = SOKOG; break;
        case DROITE: dcol = 1; *retourTouche = SOKOD; break;
        default: return;
    }

    int lig_suiv = *lig_soko + dlig;
    int col_suiv = *col_soko + dcol;
    int lig_der = lig_suiv + dlig;
    int col_der = col_suiv + dcol;

    if (lig_suiv < 0 || lig_suiv >= NB_LIG || col_suiv < 0 || col_suiv >= NB_COL) return;

    char case_suiv = plateau[lig_suiv][col_suiv];

    if (case_suiv == MUR) return;

    if (case_suiv == VIDE || case_suiv == CIBLE) {
        plateau[*lig_soko][*col_soko] = retirer_sokoban(plateau[*lig_soko][*col_soko]);
        plateau[lig_suiv][col_suiv] = afficher_sokoban(plateau[lig_suiv][col_suiv]);
        *lig_soko = lig_suiv;
        *col_soko = col_suiv;
        return;
    }

    if (case_suiv == CAISSE || case_suiv == CAISSE_SUR_CIBLE) {
        conversion_retourtouche(*retourTouche);
        if (lig_der < 0 || lig_der >= NB_LIG || col_der < 0 || col_der >= NB_COL) return;

        char case_der = plateau[lig_der][col_der];

        if (case_der == MUR || case_der == CAISSE || case_der == CAISSE_SUR_CIBLE) return;

        plateau[lig_der][col_der] = afficher_caisse(case_der);
        plateau[lig_suiv][col_suiv] = retirer_caisse(plateau[lig_suiv][col_suiv]);
        plateau[*lig_soko][*col_soko] = retirer_sokoban(plateau[*lig_soko][*col_soko]);
        plateau[lig_suiv][col_suiv] = afficher_sokoban(plateau[lig_suiv][col_suiv]);
        *lig_soko = lig_suiv;
        *col_soko = col_suiv;
    }
}

/* recherche la position du Sokoban sur le plateau */
void trouver_sokoban(t_Plateau plateau, int *lig_soko, int *col_soko) {
    for (int lig = 0; lig < NB_LIG; lig++) {
        for (int col = 0; col < NB_COL; col++) {
            if (plateau[lig][col] == SOKOBAN || plateau[lig][col] == SOKOBAN_SUR_CIBLE) {
                *lig_soko = lig;
                *col_soko = col;
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
bool afficher_recommencer(t_Plateau plateau, t_Plateau plateau_initial, int *nb_deplacements, int *lig_soko, int *col_soko) {
    char rep;

    printf("Recommencer ? (o/n) : ");
    scanf(" %c", &rep);

    if (rep == OUI) {
        copier_plateau(plateau, plateau_initial);
        *nb_deplacements = 0;
        *lig_soko = lig_soko_initial;
        *col_soko = col_soko_initial;
        return true;
    }

    return false;
}

/* affiche l'écran final de victoire avec le plateau */
void afficher_gagner(t_Plateau plateau, char fichier[50], int nb_deplacements, int zoom) {
    system("clear");
    afficher_entete(fichier, nb_deplacements);
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