#ifndef TETRISONLINE_OBJETS_H
#define TETRISONLINE_OBJETS_H

#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include "raylib.h"
#include <unordered_set>

std::vector<std::vector<int>> multiply(const std::vector<std::vector<int>>& A,
                                       const std::vector<std::vector<int>>& B) {
    int n=A.size(), p=A[0].size(), m=B[0].size();
    std::vector<std::vector<int>> C(n,std::vector<int>(m,0));
    for(int i=0;i<n;i++) for(int j=0;j<m;j++) for(int k=0;k<p;k++) C[i][j]+=A[i][k]*B[k][j];
    return C;
}

class object {
public:
    int line, column, cellsize;
    std::vector<std::vector<int>> matrice;

    object(int l,int c): line(l),column(c),cellsize(30),matrice(l,std::vector<int>(c,0)){}
    object(): line(10),column(20),cellsize(30),matrice(10,std::vector<int>(20,0)){}
    object& operator=(const object& o){
        line=o.line;column=o.column;cellsize=o.cellsize;matrice=o.matrice;return *this;
    }

    void set_zero(){for(int i=0;i<line;i++)for(int j=0;j<column;j++)matrice[i][j]=0;}

    std::vector<std::vector<int>> get_pos(){
        std::vector<int> l(4,0),c(4,0);int k=0;
        for(int i=0;i<line;i++)for(int j=0;j<column;j++)
            if(matrice[i][j]!=0){l[k]=i;c[k]=j;k++;}
        return {l,c};
    }

    std::vector<Color> GetCellColors(){
        return {
            {15, 15, 40,255},  // 0 vide
            {47,230, 23,255},  // 1 vert   garbage
            {232,18, 18,255},  // 2 rouge  O
            {226,116,17,255},  // 3 orange T
            {237,234, 4,255},  // 4 jaune  L
            {166,  0,247,255}, // 5 violet J
            { 21,204,209,255}, // 6 cyan   Z
            { 13, 64,216,255}, // 7 bleu   S
        };
    }

    void add(object other){
        if(other.line==line&&other.column==column)
            for(int i=0;i<line;i++)for(int j=0;j<column;j++)matrice[i][j]+=other.matrice[i][j];
    }

    void make_I(){set_zero();int m=line/2-2;for(int i=m;i<m+4;i++)matrice[i][0]=1;}
    void make_O(){set_zero();for(int i=0;i<2;i++){matrice[line/2][i]=2;matrice[line/2-1][i]=2;}}
    void make_T(){set_zero();int m=line/2-1;for(int i=m;i<m+3;i++)matrice[i][0]=3;matrice[m+1][1]=3;}
    void make_L(){set_zero();int m=line/2-1;for(int i=m;i<m+3;i++)matrice[i][0]=4;matrice[m][1]=4;}
    void make_J(){set_zero();int m=line/2-1;for(int i=m;i<m+3;i++)matrice[i][0]=5;matrice[m+2][1]=5;}
    void make_Z(){set_zero();int m=line/2-1;for(int i=m;i<m+2;i++)matrice[i][0]=6;for(int i=m+1;i<m+3;i++)matrice[i][1]=6;}
    void make_S(){set_zero();int m=line/2-1;for(int i=m;i<m+2;i++)matrice[i][1]=7;for(int i=m+1;i<m+3;i++)matrice[i][0]=7;}

    void translate_d(){
        int s=0;for(int i=0;i<column;i++)s+=matrice[line-1][i];
        if(s==0){std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<column;i++){m[0][i]=0;for(int j=1;j<line;j++)m[j][i]=matrice[j-1][i];}matrice=m;}
    }
    void translate_g(){
        int s=0;for(int i=0;i<column;i++)s+=matrice[0][i];
        if(s==0){std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<column;i++){m[line-1][i]=0;for(int j=0;j<line-1;j++)m[j][i]=matrice[j+1][i];}matrice=m;}
    }
    void translate_bas(){
        int s=0;for(int i=0;i<line;i++)s+=matrice[i][column-1];
        if(s==0){std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<line;i++){m[i][0]=0;for(int j=1;j<column;j++)m[i][j]=matrice[i][j-1];}matrice=m;}
    }
    void translate_haut(){
        int s=0;for(int i=0;i<line;i++)s+=matrice[i][0];
        if(s==0){std::vector<std::vector<int>> m(line,std::vector<int>(column,0));
            for(int i=0;i<line;i++){m[i][column-1]=0;for(int j=0;j<column-1;j++)m[i][j]=matrice[i][j+1];}matrice=m;}
    }

    void rotate(){
        std::vector<std::vector<int>> R={{0,-1},{1,0}};
        auto P=get_pos();
        int ic=matrice[P[0][0]][P[1][0]];
        int minxp=*min_element(P[0].begin(),P[0].end());
        int minyp=*min_element(P[1].begin(),P[1].end());
        auto P1=multiply(R,P);
        int minx=*min_element(P1[0].begin(),P1[0].end());
        int miny=*min_element(P1[1].begin(),P1[1].end());
        for(int i=0;i<4;i++){P1[0][i]+=-minx+minxp;P1[1][i]+=-miny+minyp;}
        int mx=*max_element(P1[0].begin(),P1[0].end());
        if(mx>=line)for(int i=0;i<4;i++)P1[0][i]-=(mx-line+1);
        int my=*max_element(P1[1].begin(),P1[1].end());
        if(my>=column)for(int j=0;j<4;j++)P1[1][j]-=(my-column+1);
        set_zero();
        for(int i=0;i<4;i++)matrice[P1[0][i]][P1[1][i]]=ic;
    }

    bool check_collision(object& o){
        auto P=get_pos();
        if(*max_element(P[1].begin(),P[1].end())==column-1)return true;
        for(int i=0;i<4;i++)if(o.matrice[P[0][i]][P[1][i]+1]!=0)return true;
        return false;
    }
    bool check_left(object& o){
        auto P=get_pos();
        for(int i=0;i<4;i++){if(P[0][i]==0)return false;if(o.matrice[P[0][i]-1][P[1][i]]!=0)return false;}
        return true;
    }
    bool check_right(object& o){
        auto P=get_pos();
        for(int i=0;i<4;i++){if(P[0][i]==line-1)return false;if(o.matrice[P[0][i]+1][P[1][i]]!=0)return false;}
        return true;
    }
    bool check_rotate(object& o){
        object t=object();t.matrice=matrice;t.rotate();
        auto P=t.get_pos();
        for(int i=0;i<4;i++)if(o.matrice[P[0][i]][P[1][i]]!=0)return false;
        return true;
    }
    bool checkintersection(object& o){
        auto P=get_pos();
        for(int i=0;i<4;i++)if(o.matrice[P[0][i]][P[1][i]]!=0)return false;
        return true;
    }

    // ─── Rendu avec offset ox, oy ─────────────
    // Toutes les fonctions de dessin prennent un offset pixel
    void dessiner(int ox=0, int oy=0){
        auto colors=GetCellColors();
        for(int i=0;i<line;i++)
            for(int j=0;j<column;j++)
                DrawRectangle(ox+i*cellsize+1, oy+j*cellsize+1,
                              cellsize-2, cellsize-2, colors[matrice[i][j]]);
    }

    void dessiner_piece(int ox, int oy, Color ghost_color, bool is_ghost){
        auto P=get_pos();
        auto colors=GetCellColors();
        for(int i=0;i<4;i++){
            int x=ox+P[0][i]*cellsize+1;
            int y=oy+P[1][i]*cellsize+1;
            if(is_ghost)
                DrawRectangleLines(x,y,cellsize-2,cellsize-2,ghost_color);
            else
                DrawRectangle(x,y,cellsize-2,cellsize-2,colors[matrice[P[0][i]][P[1][i]]]);
        }
    }

    // ─── Lignes ───────────────────────────────
    std::vector<int> find_not_null(){
        std::vector<int> r;
        for(int j=column-1;j>=0;j--){
            bool f=true;
            for(int i=0;i<line;i++)if(matrice[i][j]==0){f=false;break;}
            if(f)r.push_back(j);
        }
        return r;
    }

    void destroyline(int idx){
        for(int i=0;i<line;i++)matrice[i][idx]=0;
        object t=*this;
        for(int i=0;i<line;i++)for(int j=idx;j<column;j++)t.matrice[i][j]=0;
        column=idx;t.translate_bas();
        set_zero();column=t.column;add(t);
    }

    int destroy(){
        auto nn=find_not_null();int n=nn.size();
        if(n==0)return 0;
        for(int i=n-1;i>=0;i--)destroyline(nn[i]);
        return n;
    }
};

#endif
