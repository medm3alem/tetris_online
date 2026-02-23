#ifndef TETRISONLINE_OBJETS_H
#define TETRISONLINE_OBJETS_H

#include <vector>
#include <algorithm>
#include "raylib.h"

// ─── Rotation matricielle 90° sens horaire ────────────────────────
// Multiplie A (2×4) par B (4×k)
static std::vector<std::vector<int>> mat_mul(
    const std::vector<std::vector<int>>& A,
    const std::vector<std::vector<int>>& B)
{
    int n=(int)A.size(), p=(int)A[0].size(), m=(int)B[0].size();
    std::vector<std::vector<int>> C(n, std::vector<int>(m,0));
    for(int i=0;i<n;i++) for(int j=0;j<m;j++) for(int k=0;k<p;k++)
        C[i][j]+=A[i][k]*B[k][j];
    return C;
}

// ─── Palette couleurs (index = valeur dans matrice) ───────────────
static const std::vector<Color> PALETTE = {
    {15,  15,  40, 255},  // 0 vide
    {47, 230,  23, 255},  // 1 vert   (garbage)
    {232,  18,  18, 255}, // 2 rouge  O
    {226, 116,  17, 255}, // 3 orange T
    {237, 234,   4, 255}, // 4 jaune  L
    {166,   0, 247, 255}, // 5 violet J
    { 21, 204, 209, 255}, // 6 cyan   Z
    { 13,  64, 216, 255}, // 7 bleu   S
};

// ═════════════════════════════════════════════════════════════════
// Représente soit une pièce (4 cellules non-nulles) soit la grille.
// Axes : matrice[ligne][colonne] → ligne=X horizontal, colonne=Y vertical.
// ═════════════════════════════════════════════════════════════════
class Piece {
public:
    int W, H, CS;  // largeur (nb lignes), hauteur (nb colonnes), cell size px
    std::vector<std::vector<int>> cells;

    Piece() : W(10), H(20), CS(30), cells(10, std::vector<int>(20,0)) {}
    Piece(int w, int h) : W(w), H(h), CS(30), cells(w, std::vector<int>(h,0)) {}

    Piece& operator=(const Piece& o) {
        W=o.W; H=o.H; CS=o.CS; cells=o.cells; return *this;
    }

    void clear() { for(auto& r:cells) std::fill(r.begin(),r.end(),0); }

    // Positions des 4 cellules non-nulles → {{rows},{cols}}
    std::vector<std::vector<int>> pos() const {
        std::vector<int> rs(4,0), cs(4,0); int k=0;
        for(int i=0;i<W;i++) for(int j=0;j<H;j++)
            if(cells[i][j]!=0){ rs[k]=i; cs[k]=j; k++; }
        return {rs,cs};
    }

    // ─── Formes ───────────────────────────────────────────────────
    void make_I(){ clear(); int m=W/2-2; for(int i=m;i<m+4;i++) cells[i][0]=1; }
    void make_O(){ clear(); cells[W/2-1][0]=2; cells[W/2][0]=2;
                            cells[W/2-1][1]=2; cells[W/2][1]=2; }
    void make_T(){ clear(); int m=W/2-1;
                   for(int i=m;i<m+3;i++) cells[i][0]=3; cells[m+1][1]=3; }
    void make_L(){ clear(); int m=W/2-1;
                   for(int i=m;i<m+3;i++) cells[i][0]=4; cells[m][1]=4; }
    void make_J(){ clear(); int m=W/2-1;
                   for(int i=m;i<m+3;i++) cells[i][0]=5; cells[m+2][1]=5; }
    void make_Z(){ clear(); int m=W/2-1;
                   cells[m][0]=6; cells[m+1][0]=6;
                   cells[m+1][1]=6; cells[m+2][1]=6; }
    void make_S(){ clear(); int m=W/2-1;
                   cells[m+1][0]=7; cells[m+2][0]=7;
                   cells[m][1]=7;   cells[m+1][1]=7; }

    // ─── Translations ─────────────────────────────────────────────
    void move_right() {
        for(int j=0;j<H;j++) if(cells[W-1][j]) return;
        Piece t(W,H); t.CS=CS;
        for(int j=0;j<H;j++){ t.cells[0][j]=0; for(int i=1;i<W;i++) t.cells[i][j]=cells[i-1][j]; }
        cells=t.cells;
    }
    void move_left() {
        for(int j=0;j<H;j++) if(cells[0][j]) return;
        Piece t(W,H); t.CS=CS;
        for(int j=0;j<H;j++){ t.cells[W-1][j]=0; for(int i=0;i<W-1;i++) t.cells[i][j]=cells[i+1][j]; }
        cells=t.cells;
    }
    void move_down() {
        for(int i=0;i<W;i++) if(cells[i][H-1]) return;
        Piece t(W,H); t.CS=CS;
        for(int i=0;i<W;i++){ t.cells[i][0]=0; for(int j=1;j<H;j++) t.cells[i][j]=cells[i][j-1]; }
        cells=t.cells;
    }

    // ─── Rotation 90° sens horaire ────────────────────────────────
    void rotate() {
        auto P = pos();
        int color = cells[P[0][0]][P[1][0]];
        int r0=*std::min_element(P[0].begin(),P[0].end());
        int c0=*std::min_element(P[1].begin(),P[1].end());

        std::vector<std::vector<int>> R={{0,-1},{1,0}};
        auto P2 = mat_mul(R,P);

        int nr=*std::min_element(P2[0].begin(),P2[0].end());
        int nc=*std::min_element(P2[1].begin(),P2[1].end());
        for(int i=0;i<4;i++){ P2[0][i]+=r0-nr; P2[1][i]+=c0-nc; }

        int mr=*std::max_element(P2[0].begin(),P2[0].end());
        if(mr>=W) for(int i=0;i<4;i++) P2[0][i]-=(mr-W+1);
        int mc=*std::max_element(P2[1].begin(),P2[1].end());
        if(mc>=H) for(int i=0;i<4;i++) P2[1][i]-=(mc-H+1);

        clear();
        for(int i=0;i<4;i++) cells[P2[0][i]][P2[1][i]]=color;
    }

    // ─── Collisions ───────────────────────────────────────────────

    // Touche le bas ou un bloc posé en dessous
    bool hits_bottom(const Piece& grid) const {
        auto P=pos();
        for(int i=0;i<4;i++){
            if(P[1][i]==H-1) return true;
            if(grid.cells[P[0][i]][P[1][i]+1]!=0) return true;
        }
        return false;
    }
    bool can_go_left(const Piece& grid) const {
        auto P=pos();
        for(int i=0;i<4;i++){
            if(P[0][i]==0) return false;
            if(grid.cells[P[0][i]-1][P[1][i]]!=0) return false;
        }
        return true;
    }
    bool can_go_right(const Piece& grid) const {
        auto P=pos();
        for(int i=0;i<4;i++){
            if(P[0][i]==W-1) return false;
            if(grid.cells[P[0][i]+1][P[1][i]]!=0) return false;
        }
        return true;
    }
    bool can_rotate(const Piece& grid) const {
        Piece tmp=*this; tmp.rotate();
        auto P=tmp.pos();
        for(int i=0;i<4;i++) if(grid.cells[P[0][i]][P[1][i]]!=0) return false;
        return true;
    }
    // Vrai si aucune cellule de this ne chevauche grid
    bool no_overlap(const Piece& grid) const {
        auto P=pos();
        for(int i=0;i<4;i++) if(grid.cells[P[0][i]][P[1][i]]!=0) return false;
        return true;
    }

    // ─── Fusion ───────────────────────────────────────────────────
    void merge(const Piece& other) {
        for(int i=0;i<W;i++) for(int j=0;j<H;j++)
            cells[i][j]+=other.cells[i][j];
    }

    // ─── Gestion des lignes ───────────────────────────────────────

    // Retourne les indices de colonnes (j) complètes
    std::vector<int> full_lines() const {
        std::vector<int> full;
        for(int j=0;j<H;j++){
            bool ok=true;
            for(int i=0;i<W;i++) if(!cells[i][j]){ok=false;break;}
            if(ok) full.push_back(j);
        }
        return full;
    }

    // Détruit toutes les lignes complètes en une seule passe.
    // Algorithme : on recopie les colonnes non-complètes depuis le bas,
    // puis on remplit le reste avec des zéros (gravité correcte pour N lignes).
    int destroy_full_lines() {
        auto fl = full_lines();
        if(fl.empty()) return 0;

        // Marquer les colonnes (j) à supprimer dans un set rapide
        std::vector<bool> to_delete(H, false);
        for(int j : fl) to_delete[j] = true;

        // Reconstruire colonne par colonne (j=H-1 = bas)
        // On parcourt de droite (bas) à gauche (haut) et on compacte
        std::vector<std::vector<int>> new_cells(W, std::vector<int>(H, 0));
        int write = H - 1; // curseur d'écriture (bas de la grille)
        for(int j = H-1; j >= 0; j--) {
            if(to_delete[j]) continue; // sauter les lignes complètes
            for(int i = 0; i < W; i++)
                new_cells[i][write] = cells[i][j];
            write--;
        }
        // Le reste (write >= 0) reste à 0 — lignes vides en haut

        cells = new_cells;
        return (int)fl.size();
    }

    // ─── Rendu ────────────────────────────────────────────────────

    // Fond + lignes de grille (pour la grille vide)
    void draw_background(int ox, int oy) const {
        DrawRectangle(ox, oy, W*CS, H*CS,(Color){10,10,30,255});
        Color gl={28,32,65,255};
        for(int i=0;i<=W;i++) DrawLine(ox+i*CS,oy, ox+i*CS,oy+H*CS, gl);
        for(int j=0;j<=H;j++) DrawLine(ox,oy+j*CS, ox+W*CS,oy+j*CS, gl);
    }

    // Tous les blocs non-nuls (pour la grille posée)
    void draw_blocks(int ox, int oy) const {
        for(int i=0;i<W;i++) for(int j=0;j<H;j++)
            if(cells[i][j])
                DrawRectangle(ox+i*CS+1, oy+j*CS+1, CS-2,CS-2, PALETTE[cells[i][j]]);
    }

    // Seulement les 4 cellules de la pièce courante
    void draw_piece(int ox, int oy) const {
        auto P=pos();
        for(int i=0;i<4;i++){
            int c=cells[P[0][i]][P[1][i]];
            DrawRectangle(ox+P[0][i]*CS+1, oy+P[1][i]*CS+1, CS-2,CS-2, PALETTE[c]);
        }
    }

    // Contour gris translucide (ghost)
    void draw_ghost(int ox, int oy) const {
        auto P=pos();
        for(int i=0;i<4;i++)
            DrawRectangleLines(ox+P[0][i]*CS+1, oy+P[1][i]*CS+1,
                               CS-2, CS-2,(Color){160,160,160,140});
    }

    // Flash blanc sur les lignes complètes (alpha 0..1, décroissant)
    void draw_flash(int ox, int oy, float alpha) const {
        Color fc={255,255,255,(unsigned char)(alpha*255)};
        for(int j:full_lines())
            DrawRectangle(ox, oy+j*CS, W*CS, CS, fc);
    }
};

#endif // TETRISONLINE_OBJETS_H
