#ifndef TETRISONLINE_OBJETS_H
#define TETRISONLINE_OBJETS_H

#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include "raylib.h"
#include <unordered_set>


// multiplication matricielle

std::vector<std::vector<int>> multiply(const std::vector<std::vector<int>> &A, const std::vector<std::vector<int>> &B) {
    int n = A.size();
    int p = A[0].size();
    int m = B[0].size();

    std::vector<std::vector<int>> C(n, std::vector<int>(m, 0));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            for (int k = 0; k < p; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return C;
}

//util

int nombreDistinct(const std::vector<int>& v) {
    std::unordered_set<int> s(v.begin(), v.end());
    return s.size();
}
class object {
    public:
    int line;
    int column;
    int cellsize;
    std::vector<std::vector<int>> matrice;


    void set_zero() {
        for (int i = 0; i < line; i++) {
            for (int j = 0; j < column; j++) {
                matrice[i][j] = 0;
            }
        }
    }
    object& operator=(const object& other) {
        line = other.line;
        column = other.column;
        matrice = other.matrice;
        cellsize = other.cellsize;
        return *this;
    }


    object(int line, int column) {
        this->line = line;
        this->column = column;
        cellsize = 30;
        matrice = std::vector<std::vector<int>>(line, std::vector<int>(column));
        set_zero();
    };


    object() {
        this->line = 10;
        this->column = 20;
        this->cellsize = 30;
        matrice = std::vector<std::vector<int>>(10, std::vector<int>(20));
        set_zero();
    };

    void add(object other) {
        if (other.line == line && other.column == column) {
            for (int i = 0; i < line; i++) {
                for (int j = 0; j < column; j++) {
                    matrice[i][j] = matrice[i][j] + other.matrice[i][j];
                }
            }
        }

        else std::cout << " erreur add\n";
    }

    void make_I() {
        set_zero();
        int m = line/2 -2;
        for (int i = m; i<m+4; i++) matrice[i][0] = 1;
    }

    void make_O() {
        set_zero();
        for (int i = 0; i < 2; i++) {
            matrice[line/2][i] = 2;
            matrice[line/2 -1][i] = 2;
        }
    }

    void make_T() {
        set_zero();
        int m = line/2 -1;
        for (int i = m; i<m+3; i++) matrice[i][0] = 3;
        matrice[m+1][1] = 3;
    }

    void make_L() {
        set_zero();
        int m = line/2 -1;
        for (int i = m; i<m+3; i++) matrice[i][0] = 4;
        matrice[m][1] = 4;
    }

    void make_J() {
        set_zero();
        int m = line/2 -1;
        for (int i = m; i<m+3; i++) matrice[i][0] = 5;
        matrice[m+2][1] = 5;
    }

    void make_Z() {
        set_zero();
        int m = line/2 -1;
        for (int i = m; i<m+2; i++) matrice[i][0] = 6;
        for (int i = m+1; i<m+3; i++) matrice[i][1] = 6;

    }

    void make_S() {
        set_zero();
        int m = line/2 -1;
        for (int i = m; i<m+2; i++) matrice[i][1] = 7;
        for (int i = m+1; i<m+3; i++) matrice[i][0] = 7;

    }


    std::vector<int> getsize() const{
        return std::vector<int>{line, column};
    }

    std::vector<std::vector<int>> getmatrice() const {
        return matrice;
    }

    void printmatrice() const {
        for (int i = 0; i < column; i++) {
            std :: cout << "\n";
            for (int j = 0; j < line; j++) {
                if (matrice[j][i]==0) std::cout << "-";
                else std::cout << "G";
            }
        }
        std :: cout << "\n";
    }


    void translate_d() {
        int sum = 0;
        for (int i=0; i < column; i++) sum += matrice[line-1][i];
        if (sum == 0) {
            std::vector<std::vector<int>> m(line, std::vector<int>(column));
            for (int i = 0; i < column; i++) {
                m[0][i] = 0;
                for (int j = 1; j < line; j++) {
                    m[j][i] = matrice[j-1][i];
                }
            }
            matrice = m;

        }
        //else std::cout << "le plus droite possible";

    }

    void translate_g() {
        int sum = 0;
        for (int i=0; i < column; i++) sum += matrice[0][i];
        if (sum == 0) {
            std::vector<std::vector<int>> m(line, std::vector<int>(column));
            for (int i = 0; i < column; i++) {
                m[line-1][i] = 0;
                for (int j = 0; j < line-1; j++) {
                    m[j][i] = matrice[j+1][i];
                }
            }
            matrice = m;

        }
        //else std::cout << "le plus gauche possible";

    }


    void translate_bas() {
        int sum = 0;
        for (int i=0; i < line; i++) sum += matrice[i][column-1];
        if (sum == 0) {
            std::vector<std::vector<int>> m(line, std::vector<int>(column));
            for (int i = 0; i < line; i++) {
                m[i][0] = 0;
                for (int j = 1; j < column; j++) {
                    m[i][j] = matrice[i][j-1];
                }
            }
            matrice = m;

        }
    //else std::cout << "le plus bas possible";
    }

    void translate_haut() {
        int sum = 0;
        for (int i=0; i < line; i++) sum += matrice[i][0];
        if (sum == 0) {
            std::vector<std::vector<int>> m(line, std::vector<int>(column));
            for (int i = 0; i < line; i++) {
                m[i][column-1] = 0;
                for (int j = 0; j < column-1; j++) {

                    m[i][j] = matrice[i][j+1];
                }
            }
            matrice = m;

        }
        //else std::cout << "le plus haut possible";
    }


    std::vector<std::vector<int>> get_pos() {
        std::vector<int> l(4, 0);
        std::vector<int> c(4, 0);
        std::vector<std::vector<int>> P(2, std::vector<int>(4));
        int k=0;
        for (int i=0; i < line; i++) {
            for (int j=0; j < column; j++) {
                if (matrice[i][j]!=0) {
                    l[k] = i;
                    c[k] = j;
                    k++;
                }
            }
        }
        P = std::vector{l,c};
        return P;
    }


    void rotate() {
        std::vector<std::vector<int>> P1(2, std::vector<int>(4));

        // matrice de rotation pi/2
        std::vector<std::vector<int>> R = {
            {0,-1},
            {1,0}
        };
        int ind_color = 1;
        // on remplit P par les positions des non nuls (il y a toujours quatre valeurs !=0)

        std::vector<std::vector<int>> P = get_pos();
        ind_color = matrice[P[0][0]][P[1][0]];
        int minval_xp = *min_element(P[0].begin(), P[0].end());
        int minval_yp = *min_element(P[1].begin(), P[1].end());
        P1 = multiply(R, P);
        int minval_x = *min_element(P1[0].begin(), P1[0].end());
        int minval_y = *min_element(P1[1].begin(), P1[1].end());


        // on doit faire une translation vers l'origine en faisant -(minval_x, minval_y) pour ajuster les valeurs négatifs (il s'agit d'une rotation de centre l'origine (0,0) et non sur elle meme)
        // on fait une autre translation vers la postion de l'objet P[0]


        for (int i=0; i < 4; i++) {
            P1[0][i] += -minval_x + minval_xp;
            P1[1][i] += -minval_y + minval_yp;

        }
        // P1 contient maintenant les position par rotation des nombres !=0



        // effet de bord
        int maxp1x = *max_element(P1[0].begin(), P1[0].end());
        if (maxp1x>=line) for (int i=0; i < 4; i++) P1[0][i] = (P1[0][i] - maxp1x + line-1);

        int maxp1y = *max_element(P1[1].begin(), P1[1].end());
        if (maxp1y>=column) for (int j=0; j < 4; j++) P1[1][j] = (P1[1][j] - maxp1y +column-1);

        set_zero();
        for (int i=0; i < 4; i++) {
            int x = P1[0][i];
            int y = P1[1][i];
            matrice[x][y] = ind_color;
        }


    }


    bool check_collision(object& other) {
        // !!!! other doit etre la base et this la piece contenant quatre valeurs !=0 !!!!
        // retourne true s'il n'y a plus de place (il y a contact entre l'objet et autre chose)

        std::vector<std::vector<int>> P = get_pos();
        int maxval_y = *max_element(P[1].begin(), P[1].end());
        if (maxval_y==column-1) return true;

        for (int i=0; i < 4; i++) {
            int x = P[0][i];
            int y = P[1][i];
            if ((other.matrice[x][y+1])!=0) {
                //other.printmatrice();
                return true;
            }
        }
        return false;
    }

    bool check_left(object& other) {
        std::vector<std::vector<int>> P = get_pos();
        for (int i=0; i<4; i++) {
            int x = P[0][i];
            int y = P[1][i];
            if (x==0) return false;
            if (other.matrice[x-1][y]!=0) return false;
        }
        return true;
    }

    bool check_right(object& other) {
        std::vector<std::vector<int>> P = get_pos();
        for (int i=0; i<4; i++) {
            int x = P[0][i];
            int y = P[1][i];
            if (x==line-1) return false;
            if (other.matrice[x+1][y]!=0) return false;
        }
        return true;
    }

    bool check_rotate(object& other) {
        object temp = object();
        temp.matrice = matrice;
        temp.rotate();
        std::vector<std::vector<int>> P = temp.get_pos();
        for (int i=0; i < 4; i++) {
            int x = P[0][i];
            int y = P[1][i];
            if (other.matrice[x][y]!=0) return false;
        }
        return true;
    }

    std :: vector<Color> GetCellColors() {
        Color darkGrey = {26, 31, 40, 255};
        Color green = {47, 230, 23, 255};
        Color red = {232, 18, 18, 255};
        Color orange = {226, 116, 17, 255};
        Color yellow = {237, 234, 4, 255};
        Color purple = {166, 0, 247, 255};
        Color cyan = {21, 204, 209, 255};
        Color blue = {13, 64, 216, 255};

        return {darkGrey, green, red, orange, yellow, purple, cyan, blue};
    }


    void dessiner() {
        for (int i=0; i < line; i++) {
            for (int j=0; j < column; j++) {
                int ind_col = matrice[i][j];
                DrawRectangle(i*cellsize +1, j*cellsize +1, cellsize-1, cellsize-1, GetCellColors()[ind_col]);
            }
        }
    }

    std::vector<int> find_not_null() {
        int i=0;
        int j=column-1;
        std::vector<int> not_null;
        bool found = true;
        for (; j >= 0; j--) {
            i=0;
            found = true;
            for (; i < line; i++) {
                if (matrice[i][j]==0) found = false;
            }
            if (found) not_null.push_back(j);
        }
        return not_null;
    }

    void destroyline(int indline) {
        for (int i=0; i < line; i++) {
            matrice[i][indline] = 0;
        }
        //printmatrice();


        object temp = object();
        temp = *this;
        for (int i=0; i < line; i++) {
            for (int j=indline; j < column; j++) {
                temp.matrice[i][j] = 0;
            }
        }
        column = indline;
        temp.translate_bas();
        set_zero();
        column = temp.column;

        this->add(temp);

    }

    int destroy() {
        std::vector<int> not_null = find_not_null();
        int n = not_null.size();
        if (n == 0) return 0;
        //for (int indline=0; indline < n; indline++) destroyline(not_null[indline]);
        for (int indline=n-1; indline >=0; indline--) destroyline(not_null[indline]);
        //std::cout << n << std::endl;
        return n;
    }

    bool checkintersection(object& other) {
        //retourne true s'il n'y a pas d'intersection
        std::vector<std::vector<int>> P = get_pos();
        for (int i=0; i < 4; i++) {
            int x = P[0][i];
            int y = P[1][i];
            if (other.matrice[x][y]!=0) return false;
        }
        return true;
    }


};

#endif //TETRISONLINE_OBJETS_H