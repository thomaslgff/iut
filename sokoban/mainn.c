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
typedef char t_Plateau[NB_LIG][NB_COL];

// appel des procédures / fonctions:
int kbhit();
void chargerPartie(t_Plateau plateau, char fichier[50]);
void afficherEntete(char fichier[50], int nbDeplacements);
void afficherPlateau(t_Plateau plateau);
void enregistrerPartie(t_Plateau plateau, char fichier[50]);
void copierPlateau(t_Plateau destination, t_Plateau source);
char retirerSokoban(char contenuCase);
char afficherSokoban(char contenuCase);
char afficherCaisse(char contenuCase);
char retirerCaisse(char contenuCase);
void deplacer(t_Plateau plateau, char touche, int *lig_soko, int *col_soko);
void trouverSokoban(t_Plateau plateau, int *lig_soko, int *col_soko);
bool gagnerPartie(t_Plateau plateau);

int main(){
    char fichier[50];
    t_Plateau plateau;
    t_Plateau plateauInitial;
    int nbDeplacements = 0;
    int lig_soko = 0, col_soko = 0;
    bool enCours = true;
    bool abandonner = false;
    bool gagner = false;
    char touche = '\0';

    printf("nom fichier: ");
    scanf("%49s", fichier);
    afficherEntete(fichier, nbDeplacements);
    chargerPartie(plateau, fichier);
    copierPlateau(plateauInitial, plateau);
    trouverSokoban(plateau, &lig_soko, &col_soko);

    while(enCours){
        system("clear");
        afficherEntete(fichier, nbDeplacements);
        afficherPlateau(plateau);
        while (kbhit() == 0){}
        touche = getchar();
        if (touche == HAUT || touche == BAS || touche == GAUCHE || touche == DROITE){
            int ligAncienne = lig_soko;
            int colAncienne = col_soko;
            deplacer(plateau, touche, &lig_soko, &col_soko);
            if (ligAncienne != lig_soko || colAncienne != col_soko){
                nbDeplacements++;
                if (gagnerPartie(plateau)){
                    gagner = true;
                    enCours = false;
                }   
            }
        }
        else if(touche == ABANDONNER){
            abandonner = true;
            enCours = false;
        }
        else if(touche == RECOMMENCER){
            char reponse;
            printf("Recommencer depuis le début ? (o/n) : ");
            scanf(" %c", &reponse);

            if (reponse == OUI) {
                copierPlateau(plateau, plateauInitial);
                nbDeplacements = 0;
                trouverSokoban(plateau, &lig_soko, &col_soko);
                continue;
            }
            else {
                continue;
            }
        }
    }
    if (abandonner){
        char fichier[50];
        char reponse;
        printf("Partie abandonnée\n");
        printf("voulez vous sauvegarder la partie ? (o/n) : ");
        scanf(" %c", &reponse);
        if (reponse == OUI){
            printf("nom de la sauvegarde (nom.sok): ");
            scanf("%49s", fichier);
            enregistrerPartie(plateau, fichier);
        }
        else if (reponse == NON){
            printf("partie non sauvegardée\n");
        }
        else{
            printf("réponse invalide, partie non sauvegardée\n");
        }
    }
    if(gagner){
        printf("VICTOIRE !\n");
    }
    return 0;
}






int kbhit(){
	// la fonction retourne :
	// 1 si un caractere est present
	// 0 si pas de caractere présent
	int unCaractere=0;
	struct termios oldt, newt;
	int ch;
	int oldf;

	// mettre le terminal en mode non bloquant
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
	ch = getchar();

	// restaurer le mode du terminal
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
 
	if(ch != EOF){
		ungetc(ch, stdin);
		unCaractere=1;
	} 
	return unCaractere;
}

void chargerPartie(t_Plateau plateau, char fichier[50]){
    FILE * f;
    char finDeLigne;

    f = fopen(fichier, "r");
    if (f==NULL){
        printf("ERREUR SUR FICHIER");
        exit(EXIT_FAILURE);
    } else {
        for (int ligne=0 ; ligne<TAILLE ; ligne++){
            for (int colonne=0 ; colonne<TAILLE ; colonne++){
                fread(&plateau[ligne][colonne], sizeof(char), 1, f);
            }
            fread(&finDeLigne, sizeof(char), 1, f);
        }
        fclose(f);
    }
}

void enregistrerPartie(t_Plateau plateau, char fichier[50]){
    FILE * f;
    char finDeLigne='\n';

    f = fopen(fichier, "w");
    for (int ligne=0 ; ligne<TAILLE ; ligne++){
        for (int colonne=0 ; colonne<TAILLE ; colonne++){
            fwrite(&plateau[ligne][colonne], sizeof(char), 1, f);
        }
        fwrite(&finDeLigne, sizeof(char), 1, f);
    }
    fclose(f);
}

void afficherEntete(char fichier[50], int nbDeplacements){
    printf("Partie %s \n", fichier);
    printf("vers la gauche = Q | vers le haut = Z | vers le bas = S | vers la droite = D\n");
    printf("APPUYEZ SUR X POUR ABANDONNER LA PARTIE\n");
    printf("APPUYEZ SUR R POUR RECOMMENCER LA PARTIE\n");
    printf("nombre de déplacements: %d\n", nbDeplacements);
}

void afficherPlateau(t_Plateau plateau){
    for (int lig = 0 ; lig < NB_LIG ; lig++){
        printf("\n");
        for (int col = 0 ; col < NB_COL ; col++){
            char c = plateau[lig][col];
            if (c == SOKOBAN_SUR_CIBLE) c = '@';  
            if (c == CAISSE_SUR_CIBLE)  c = '$';  
            printf("%c", c);
        }
    }
}

void copierPlateau(t_Plateau destination, t_Plateau source){
    for (int lig = 0; lig < NB_LIG; lig++){
        for (int col = 0; col < NB_COL; col++){
            destination[lig][col] = source[lig][col];
        }
    }
}

char retirerSokoban(char contenuCase) {
    if (contenuCase == SOKOBAN_SUR_CIBLE) return CIBLE;
    if (contenuCase == SOKOBAN)           return VIDE;
    return contenuCase;
}

char afficherSokoban(char contenuCase) {
    if (contenuCase == CIBLE) return SOKOBAN_SUR_CIBLE;
    if (contenuCase == VIDE)  return SOKOBAN;
    return contenuCase;
}

char retirerCaisse(char contenuCase) {
    if (contenuCase == CAISSE_SUR_CIBLE) return CIBLE;
    if (contenuCase == CAISSE)           return VIDE;
    return contenuCase;
}

char afficherCaisse(char contenuCase) {
    if (contenuCase == CIBLE) return CAISSE_SUR_CIBLE;
    if (contenuCase == VIDE)  return CAISSE;
    return contenuCase;
}

void deplacer(t_Plateau plateau, char touche, int *lig_soko, int *col_soko){
    int decalageLig = 0; 
    int decalageCol = 0;

    switch(touche){
        case HAUT:    decalageLig--; break;
        case BAS:     decalageLig++; break;
        case GAUCHE:  decalageCol--; break;
        case DROITE:  decalageCol++; break;
        default: return;
    }

    int ligSuivante  = *lig_soko + decalageLig;
    int colSuivante  = *col_soko + decalageCol;
    int ligDerriere  = ligSuivante + decalageLig;
    int colDerriere  = colSuivante + decalageCol;

    if (ligSuivante < 0 || ligSuivante >= NB_LIG || colSuivante < 0 || colSuivante >= NB_COL){
        return;
    }

    char caseSuivante = plateau[ligSuivante][colSuivante];

    if (caseSuivante == MUR){
        return;
    } 
    if (caseSuivante == VIDE || caseSuivante == CIBLE) {
        plateau[*lig_soko][*col_soko]   = retirerSokoban(plateau[*lig_soko][*col_soko]);
        plateau[ligSuivante][colSuivante] = afficherSokoban(plateau[ligSuivante][colSuivante]);
        *lig_soko = ligSuivante;
        *col_soko = colSuivante;
        return;
    }
    if (caseSuivante == CAISSE || caseSuivante == CAISSE_SUR_CIBLE) {
        if (ligDerriere < 0 || ligDerriere >= NB_LIG || colDerriere < 0 || colDerriere >= NB_COL){
            return;
        }

        char caseDerriere = plateau[ligDerriere][colDerriere];

        if (caseDerriere == MUR || caseDerriere == CAISSE || caseDerriere == CAISSE_SUR_CIBLE){
            return;
        }
        plateau[ligDerriere][colDerriere] = afficherCaisse(caseDerriere);
        plateau[ligSuivante][colSuivante] = retirerCaisse(plateau[ligSuivante][colSuivante]);

        plateau[*lig_soko][*col_soko]   = retirerSokoban(plateau[*lig_soko][*col_soko]);
        plateau[ligSuivante][colSuivante] = afficherSokoban(plateau[ligSuivante][colSuivante]);

        *lig_soko = ligSuivante;
        *col_soko = colSuivante;
        return;
    }
}

void trouverSokoban(t_Plateau plateau, int *lig_soko, int *col_soko){
    for (int lig = 0 ; lig < NB_LIG ; lig++){
        for (int col = 0 ; col < NB_COL ; col++){
            if (plateau[lig][col] == SOKOBAN  || plateau[lig][col] == SOKOBAN_SUR_CIBLE){
                *lig_soko = lig;
                *col_soko = col;
            }
        }
    }
}

bool gagnerPartie(t_Plateau plateau){
    for (int lig = 0; lig < NB_LIG; lig++){
        for (int col = 0; col < NB_COL; col++){
            if (plateau[lig][col] == CIBLE ||           
                plateau[lig][col] == SOKOBAN_SUR_CIBLE) 
            {
                return false; 
            }
        }
    }
    return true;
}






