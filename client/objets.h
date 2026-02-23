#ifndef TETRISONLINE_OBJETS_H
#define TETRISONLINE_OBJETS_H

#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include "raylib.h"
#include <unordered_set>

// ─── Multiplication matricielle ───────────────
std::vector<std::vector<int>> multiply(const std::vector<std::vector<int>>& A,
                                       const std::vector<std::vector<int>>& B) {
    int n = A.size(), p = A[0].size(), m = B[0].size();
    std::vector<std::vector<int>> C(n, std::vector<int>(m, 0));
    for (int i=0;i<n;i++) for (int j=0;j<m;j++) for (int k=0;k<p;k++) C[i][j]+=A[i][k]*B[k][j];
    return C;
}

class object {
public:
    int line, column, cellsize;
    std::vector<std::vector<int>> matrice;

    // ─── Constructeurs ────────────────────────
    object(int l, int c) : line(l), column(c), cellsize(30),
        matrice(l, std::vector<int>(c, 0)) {}

    object() : line(10), column(20), cellsize(30),
        matrice(10, std::vector<int>(20, 0)) {}

    object& operator=(const object& other) {
        line=other.line; column=other.column;
        cellsize=other.cellsize; matrice=other.matrice;
        return *this;
    }

    // ─── Utilitaires ──────────────────────────
    void set_zero() {
        for (int i=0;i<line;i++) for (int j=0;j<column;j++) matrice[i][j]=0;
    }

    std::vector<std::vector<int>> get_pos() {
        std::vector<int> l(4,0), c(4,0);
        int k=0;
        for (int i=0;i<line;i++)
            for (int j=0;j<column;j++)
                if (matrice[i][j]!=0) { l[k]=i; c[k]=j; k++; }
        return {l, c};
    }

    std::vector<Color> GetCellColors() {
        return {
            {26,31,40,255},   // 0 vide
            {47,230,23,255},  // 1 vert   (garbage)
            {232,18,18,255},  // 2 rouge  O
            {226,116,17,255}, // 3 orange T
            {237,234,4,255},  // 4 jaune  L
            {166,0,247,255},  // 5 violet J
            {21,204,209,255}, // 6 cyan   Z
            {13,64,216,255},  // 7 bleu   S
        };
    }

    void add(object other) {
        if (other.line==line && other.column==column)
            for (int i=0;i<line;i++) for (int j=0;j<column;j++)
                matrice[i][j]+=other.matrice[i][j];
        else std::cout<<"erreur add\n";
    }

    // ─── Pièces ───────────────────────────────
    void make_I() { set_zero(); int m=line/2-2; for(int i=m;i<m+4;i++) matrice[i][0]=1; }
    void make_O() { set_zero(); for(int i=0;i<2;i++){matrice[line/2][i]=2;matrice[line/2-1][i]=2;} }
    void make_T() { set_zero(); int m=line/2-1; for(int i=m;i<m+3;i++) matrice[i][0]=3; matrice[m+1][1]=3; }
    void make_L() { set_zero(); int m=line/2-1; for(int i=m;i<m+3;i++) matrice[i][0]=4; matrice[m][1]=4; }
    void make_J() { set_zero(); int m=line/2-1; for(int i=m;i<m+3;i++) matrice[i][0]=5; matrice[m+2][1]=5; }
    void make_Z() { set_zero(); int m=line/2-1; for(int i=m;i<m+2;i++) matrice[i][0]=6; for(int i=m+1;i<m+3;i++) matrice[i][1]=6; }
    void make_S() { set_zero(); int m=line/2-1; for(int i=m;i<m+2;i++) matrice[i][1]=7; for(int i=m+1;i<m+3;i++) matrice[i][0]=7; }

    // ─── Translations ─────────────────────────
    void translate_d() {
        int sum=0; for(int i=0;i<column;i++) sum+=matrice[line-1][i];
        if(sum==0){
            std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<column;i++){m[0][i]=0;for(int j=1;j<line;j++)m[j][i]=matrice[j-1][i];}
            matrice=m;
        }
    }
    void translate_g() {
        int sum=0; for(int i=0;i<column;i++) sum+=matrice[0][i];
        if(sum==0){
            std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<column;i++){m[line-1][i]=0;for(int j=0;j<line-1;j++)m[j][i]=matrice[j+1][i];}
            matrice=m;
        }
    }
    void translate_bas() {
        int sum=0; for(int i=0;i<line;i++) sum+=matrice[i][column-1];
        if(sum==0){
            std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<line;i++){m[i][0]=0;for(int j=1;j<column;j++)m[i][j]=matrice[i][j-1];}
            matrice=m;
        }
    }
    void translate_haut() {
        int sum=0; for(int i=0;i<line;i++) sum+=matrice[i][0];
        if(sum==0){
            std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<line;i++){m[i][column-1]=0;for(int j=0;j<column-1;j++)m[i][j]=matrice[i][j+1];}
            matrice=m;
        }
    }

    // ─── Rotation ─────────────────────────────
    void rotate() {
        std::vector<std::vector<int>> R = {{0,-1},{1,0}};
        std::vector<std::vector<int>> P = get_pos();
        int ind_color = matrice[P[0][0]][P[1][0]];
        int minxp = *min_element(P[0].begin(),P[0].end());
        int minyp = *min_element(P[1].begin(),P[1].end());
        auto P1 = multiply(R, P);
        int minx = *min_element(P1[0].begin(),P1[0].end());
        int miny = *min_element(P1[1].begin(),P1[1].end());
        for(int i=0;i<4;i++){P1[0][i]+=-minx+minxp; P1[1][i]+=-miny+minyp;}
        int maxp1x=*max_element(P1[0].begin(),P1[0].end());
        if(maxp1x>=line) for(int i=0;i<4;i++) P1[0][i]-=(maxp1x-line+1);
        int maxp1y=*max_element(P1[1].begin(),P1[1].end());
        if(maxp1y>=column) for(int j=0;j<4;j++) P1[1][j]-=(maxp1y-column+1);
        set_zero();
        for(int i=0;i<4;i++) matrice[P1[0][i]][P1[1][i]]=ind_color;
    }

    // ─── Collisions ───────────────────────────
    bool check_collision(object& other) {
        auto P = get_pos();
        int maxy = *max_element(P[1].begin(),P[1].end());
        if(maxy==column-1) return true;
        for(int i=0;i<4;i++) if(other.matrice[P[0][i]][P[1][i]+1]!=0) return true;
        return false;
    }
    bool check_left(object& other) {
        auto P = get_pos();
        for(int i=0;i<4;i++){if(P[0][i]==0) return false; if(other.matrice[P[0][i]-1][P[1][i]]!=0) return false;}
        return true;
    }
    bool check_right(object& other) {
        auto P = get_pos();
        for(int i=0;i<4;i++){if(P[0][i]==line-1) return false; if(other.matrice[P[0][i]+1][P[1][i]]!=0) return false;}
        return true;
    }
    bool check_rotate(object& other) {
        object temp=object(); temp.matrice=matrice; temp.rotate();
        auto P=temp.get_pos();
        for(int i=0;i<4;i++) if(other.matrice[P[0][i]][P[1][i]]!=0) return false;
        return true;
    }
    bool checkintersection(object& other) {
        auto P=get_pos();
        for(int i=0;i<4;i++) if(other.matrice[P[0][i]][P[1][i]]!=0) return false;
        return true;
    }

    // ─── Rendu ────────────────────────────────
    // Dessine la grille complète
    void dessiner() {
        auto colors = GetCellColors();
        for(int i=0;i<line;i++)
            for(int j=0;j<column;j++)
                DrawRectangle(i*cellsize+1, j*cellsize+1,
                              cellsize-1, cellsize-1, colors[matrice[i][j]]);
    }

    // Dessine uniquement les 4 cellules non-nulles (pour pièce courante ou ghost)
    void dessiner_piece(Color override_color, bool use_override) {
        auto P = get_pos();
        auto colors = GetCellColors();
        for(int i=0;i<4;i++){
            int x = P[0][i]*cellsize+1;
            int y = P[1][i]*cellsize+1;
            if (use_override)
                DrawRectangleLines(x, y, cellsize-1, cellsize-1, override_color);
            else
                DrawRectangle(x, y, cellsize-1, cellsize-1, colors[matrice[P[0][i]][P[1][i]]]);
        }
    }

    // ─── Lignes ───────────────────────────────
    std::vector<int> find_not_null() {
        std::vector<int> not_null;
        for(int j=column-1;j>=0;j--){
            bool found=true;
            for(int i=0;i<line;i++) if(matrice[i][j]==0){found=false;break;}
            if(found) not_null.push_back(j);
        }
        return not_null;
    }

    void destroyline(int indline) {
        for(int i=0;i<line;i++) matrice[i][indline]=0;
        object temp=object(); temp=*this;
        for(int i=0;i<line;i++) for(int j=indline;j<column;j++) temp.matrice[i][j]=0;
        column=indline;
        temp.translate_bas();
        set_zero();
        column=temp.column;
        this->add(temp);
    }

    // Gravité : après destruction, les blocs flottants retombent
    // Pour chaque colonne X, on compacte les valeurs non-nulles vers le bas (j élevé)
    void apply_gravity() {
        for (int i = 0; i < line; i++) {
            // Collecter les blocs non-nuls de cette colonne (de haut en bas)
            std::vector<int> blocks;
            for (int j = 0; j < column; j++)
                if (matrice[i][j] != 0) blocks.push_back(matrice[i][j]);

            // Remplir la colonne : vide en haut, blocs en bas
            int empty = column - (int)blocks.size();
            for (int j = 0; j < empty; j++)       matrice[i][j] = 0;
            for (int j = 0; j < (int)blocks.size(); j++) matrice[i][empty + j] = blocks[j];
        }
    }

    int destroy() {
        auto not_null=find_not_null();
        int n=not_null.size();
        if(n==0) return 0;
        for(int i=n-1;i>=0;i--) destroyline(not_null[i]);
        return n;
    }
};

#endif // TETRISONLINE_OBJETS_H
