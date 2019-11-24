
#include <GL/glut.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>



/*Estrutura de dados que representa cada posição no labirinto 3D*/
struct position3d{
    int i;  /*Posição no labirinto*/
    int j;

    int k;  /*Numero do Labirinto, k varia de 0 a 2*/
    int w;  /*Labirinto está invertido ou não. Com k, essa estrutra representa qualquer posição nos 6 labirintos*/
};

/*Estrutura de dados utilizada na animação*/
struct path{
    float i;  /*Posição no labirinto*/
    float j;

    int k;  /*Numero representando se o labirinto está cruzando o labirinto inicial no eixo x, y ou z*/
    int w;  /*Diz se o labirinto está invertido ou não*/
};

/*Nó da Pilha*/
struct node3d{
    struct position3d data;
    struct node3d *next;
};

/*Matriz com posições já passadas durante a busca em profundiade*/
static bool** isLooping[3][2] ;

/*Tamanho do caminho*/
int path_size = 0;

/*Vetor de labirintos*/
static char** labirintos[3][2];

/*Espaçamento entre as posições de um labirinto*/
static double spacing = 1.5;

/*Espessura da parede*/
static float d = 1.0;

/*Altura da parede*/
static float height = 5;

float rotation = 0.0;

/*Dimensões do labirinto*/
int rows0, columns0;

/*Caminho do início até o final do labirinto*/
struct position3d* path_position3d = NULL;

/*Posição da animação, onde ela está e onde ela está indo*/
struct path now = {1, 1, 1, 1}, going = {1,2,1,1};

/*Velocidade em que o labirinto é percorrido*/
float speed = 1;

/*Variação de tempo*/
float delta = 0;

/*Tempo anterior*/
float timeOld = 0;

/*Posição atual no vetor de posições do labirinto que representa o caminho até o fim do labirinto*/
int index_path = 0;

int view = 0;

float lx=0.0f,lz=-1.0f;
// XZ position of the camera
float x=0.0f,z=10.0f;

// all variables initialized to 1.0, meaning
// the triangle will initially be white
// angle for rotating triangle
float angle = 0.0f;

double maze_spacing = 2*height+d+spacing;

bool end_run = FALSE;

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

/*Inicializa a posição inicial e final do labirinto garantindo que as posições não sejam paredes*/
void initBeginEnd3D(struct position3d *pI, struct position3d *pF){
    /*A posição inicial são inicial em linha opostas na matriz*/
    pI->i = 1;

    /*Escolha de uma coluna aleatória*/
    pI->j = rand()%columns0;

    /*Escolha de um labirinto aleatório*/
    pI->k = rand()%3;
    pI->w = rand()%2;

    /*Garante que a posição escolhida não seja uma parede*/
    while(labirintos[pI->k][pI->w][pI->i][pI->j] == '1')
        pI->j = (pI->j + 1)%columns0;

    /*Escolha da posição Final, utilizando o mesmo processo de escolha da posição inicial, só que colacando na linha mais distante do labirinto*/
    pF->i = rows0 - 2;
    pF->j = rand()%columns0;
    pF->k = rand()%3;
    pF->w = rand()%2;
    while(labirintos[pF->k][pF->w][pF->i][pF->j] == '1')
        pF->j = (pF->j + 1)%columns0;

}

/*Garante que todos os labirinto tenham passagens entre eles*/
void prepareMazes(){
    /*Abre buracos nas divisões entre os labirintos*/
    for(int k = 0; k < 3; k++)
        for(int w = 0; w < 2; w++){
            labirintos[k][w][1][(int)(columns0*0.5)] = '0';
            labirintos[k][w][1][(int)(columns0*0.5 + 1)] = '0';

            labirintos[k][w][(int)(rows0*0.5)][1] = '0';
            labirintos[k][w][(int)(rows0*0.5 + 1)][1] = '0';
    }
}

/*Métodos da pilha de posições*/

/*Empilha uma posição*/
void push(struct position3d item, struct node3d** top){
    struct node3d *nptr = (struct node3d*)malloc(sizeof(struct node3d));
    nptr->data = item;
    nptr->next = *top;
    *top = nptr;
}

/*Imprime a pilha*/
void printStack(struct node3d *top){
    struct node3d *temp;
    temp = top;
    while (temp != NULL)
    {
        printf("\n(%d,%d)", temp->data.i, temp->data.j);
        temp = temp->next;
    }
}

/*Desempilha uma posição*/
struct position3d pop(struct node3d** top){
    if (*top == NULL)
    {
        printf("\n\nStack is empty ");
        return {-1,-1};
    }
    else
    {
        struct node3d *temp;
        struct position3d data;
        temp = *top;
        data = temp->data;
        *top = (*top)->next;
        free(temp);
        return data;
    }
}

/*Inicializa a matriz utilizada na poda da árvore de busca. Garante que um caminho que já foi percorrido antes não seja percorrido novamente*/
bool** initIsLooping(int h, int b){
    bool** maze = (bool**)malloc(h * sizeof(bool*));
    for(int i = 0; i < h; i++){
        maze[i] = (bool*)malloc(b * sizeof(bool));
        for(int j = 0; j < b; j++)
           maze[i][j] = FALSE;
    }
    return maze;

}

/*Retorna a posição no vetor line de um elemento i*/
int getIndex(int line[],int i, int lenght){
    for(int k = 0; k < lenght; k++)
        if(line[k] == i)
            return k;
}

/*Coloca o elemento i na ultima posição do vetor line*/
void last_on_the_line(int line[],int i,int length){
    i = getIndex(line,i,length);
    for(int k = i; k < length - 1; k++){
        int aux = line[k];
        line[k] = line[k+1];
        line[k+1] = aux;
    }
}

/*Realiza a divisão de uma camara definida por dois pontos  na diagonal da camara*/
void gen(char **maze, int i0, int j0, int i1, int j1){
    /*Posição das linhas em relação a i e j*/
    int horizontal_line = -1, vertical_line = -1;

    /*Segmentos de linha sem buraco*/
    int non_break_points[4] = {0,1,2,3};

    /*Quantidade de buracos*/
    int break_points = 0;

    /*Garante a ordem dos pontos de forma que eles fiquem na diagonal*/
    if(i0 > i1){
        int aux = i0;
        i0 = i1;
        i1 = aux;
    }
    if(j0 > j1){
        int aux = j0;
        j0 = j1;
        j1 = aux;
    }

    /*Finaliza as dvisões por falta de espaço*/
    if(i1 - i0 < 1 || j1 - j0 < 1)
        return;

    /*Tenta desenhar a linha horizontal*/
    if(i1-i0 > 1){
        horizontal_line = i0 + 1 + rand()%(i1 - i0 - 1);

        /*Tenta criar uma linha horizontal que não começa em um buraco*/
        for(int i = 0; (i < i1 - i0 - 1) && (maze[horizontal_line][j0 - 1] == '0' || maze[horizontal_line][j1 + 1] == '0'); i++)
            horizontal_line = i0 + 1 + rand()%(i1 - i0 - 1);

        /*Cria a parede na matriz de caracteres*/
        for(int j = j0; j <= j1; j++)
            maze[horizontal_line][j] = '1';

        /*Caso ela começe ou termine em um buraco é feito um buraco adjacente a esse buaco*/
        if(maze[horizontal_line][j0 - 1] == '0'){
            maze[horizontal_line][j0] = '0';
            last_on_the_line(non_break_points,2,4);
            break_points++;
        }
        if(maze[horizontal_line][j1 + 1] == '0'){
            maze[horizontal_line][j1] = '0';
            last_on_the_line(non_break_points,0,4);
            break_points++;
        }
    }
    /*Faz com que seja possivel criar apenas mais um buraco já que deve ter apenas um segmento no máximo nesse desvio*/
    else{
        last_on_the_line(non_break_points,2,4);
        last_on_the_line(non_break_points,0,4);
        break_points = 2;
    }

    /*Tenta a criar uma linha vertical*/
    if(j1-j0 > 1){
        vertical_line = j0 + 1 + rand()%(j1 - j0 - 1);

        /*Tenta criar uma linha vertical que não começa em um buraco*/
        for(int j = 0; (j < j1 - j0 - 1) && (maze[i0 - 1][vertical_line] == '0' || maze[i1 + 1][vertical_line] == '0'); j++)
            vertical_line = j0 + 1 + rand()%(j1 - j0 - 1);

        /*Cria a linha vertical na matriz de caracteres*/
        for(int i = i0; i <= i1; i++)
            maze[i][vertical_line] = '1';

        /*Caso ela começe ou termine em um buraco é feito um buraco adjacente a esse buaco*/
        if(maze[i0 - 1][vertical_line] == '0'){
            maze[i0][vertical_line] = '0';
            last_on_the_line(non_break_points,1,4);
            break_points++;
        }

        if(maze[i1 + 1][vertical_line] == '0'){
            maze[i1][vertical_line] = '0';
            last_on_the_line(non_break_points,3,4);
            break_points++;
        }

    }
    /*Faz com que seja possivel criar apenas mais um buraco já que deve ter apenas um segmento no máximo nesse desvio*/
    else{
        last_on_the_line(non_break_points,3,4);
        last_on_the_line(non_break_points,1,4);
        break_points += 2;
    }

    /*Cria buracos aleatório de acordo com a quantidade de buracos que ainda podem ser criados*/
    while(break_points < 3 && (horizontal_line > -1 || vertical_line > -1)){
        /*Escolhe um segmento aleatório sem buraco*/
        int break_point = rand()%(4 - break_points);

        /*posição do buraco na linha ou coluna*/
        int point;

        /*Cria um buraco de acordo com a posição do segmento: do centro até a parede horizontal direita, do centro até a parede vertical superior...*/
        switch (non_break_points[break_point]) {
            case 0:
                point = j1 - rand() % (vertical_line == -1 ? j1 - j0 : j1 - vertical_line);
                maze[horizontal_line][point] = '0';
                break;
            case 1:
                point = i0 + rand() % (horizontal_line == -1 ? i1 - i0 : horizontal_line - i0);
                maze[point][vertical_line] = '0';
                break;
            case 2:
                point = j0 + rand() % (vertical_line == -1 ? j1 - j0 : vertical_line - j0);
                maze[horizontal_line][point] = '0';
                break;
            case 3:
                point = i1 - rand() % (horizontal_line == -1 ? i1 - i0 : i1 - horizontal_line);
                maze[point][vertical_line] = '0';
        }

        /*Atualiza a quantidade de buracos e retira o segmento do possiveis candidatos a buracos*/
        break_points++;
        last_on_the_line(non_break_points,non_break_points[break_point],4);
    }

    /*Passa 4 camaras novas para serem subdivididas*/
    if(horizontal_line > -1 && vertical_line > -1){
        gen(maze,i0,j0,horizontal_line - 1, vertical_line - 1);
        gen(maze,horizontal_line - 1, vertical_line + 1, i0, j1);
        gen(maze,horizontal_line + 1, vertical_line + 1, i1, j1);
        gen(maze,horizontal_line + 1, vertical_line - 1, i1, j0);
    }
    /*Passa apenas duas camaras para serem subdivididas de acordo com a linha criada*/
    else if(horizontal_line > - 1){
        gen(maze,i0,j0,horizontal_line-1,j1);
        gen(maze,i1,j1,horizontal_line+1,j0);
    }
    else if(vertical_line > -1){
        gen(maze,i0,j0,i1,vertical_line - 1);
        gen(maze,i1,j1,i0,vertical_line + 1);
    }
}

/*Cria um labirinto na memória de dimensões h por b*/
char** mazeGen(int h, int b){
    char** maze = (char**)malloc(h * sizeof(char*));
    for(int i = 0; i < h; i++){
        maze[i] = (char*)malloc(b * sizeof(char));
        for(int j = 0; j < b; j++)
            maze[i][j] = '0';
    }

    for(int i = 0; i < h; i++)
        maze[i][0] = maze[i][b-1] = '1';

    for(int j = 0; j < b; j++)
        maze[0][j] = maze[h-1][j] = '1';

    gen(maze,1,1,h-2,b-2);

    return maze;
}

/*Atuliza o caminho pelo qual a camera está se movimentando em relação ao vetor de posições*/
void updateGoing(){

    /*Termina o movimento*/
    if(index_path >= path_size - 1){
        end_run = TRUE;
        return;
    }

    /*Move o now e o going em um item do vetor que possui o caminho até o final do labirinto*/
    now.i = path_position3d[index_path].i;
    now.j = path_position3d[index_path].j;
    now.k = path_position3d[index_path].k;
    now.w = path_position3d[index_path].w;

    going.i = path_position3d[index_path+1].i;
    going.j = path_position3d[index_path+1].j;
    going.k = path_position3d[index_path+1].k;
    going.w = path_position3d[index_path+1].w;

    index_path++;
    printf("\nNow = (%f,%f) Going = (%f,%f) k = %d",now.i,now.j,going.i,going.j, now.k);
}

/*Atualiza o movimento da camera*/
void update(float delta){
    /*Faz o movimento apenas em duas direções ortogonais*/

    if(end_run)
        return;

    /*Movimento nas linhas do labirinto*/
    if(now.i != going.i){
        /*Verifica a camera está indo para trás ou para frente*/
        if(now.i < going.i){
            now.i += delta*speed;
            /*Verifica se a camera já chegou no destino atual*/
            if (now.i >= going.i)
                updateGoing();
        }
        else{
            now.i -= delta*speed;
            /*Verifica se a camera já chegou no destino atual*/
            if(now.i <= going.i)
                updateGoing();
        }
    }
    /*Movimento nas colunas*/
    else if(now.j != going.j){
        /*Verifica a camera está indo para trás ou para frente*/
        if(now.j < going.j){
            now.j += delta*speed;
            /*Verifica se a camera já chegou no destino atual*/
            if (now.j >= going.j)
                updateGoing();
        }
        else{
            now.j -= delta*speed;
            /*Verifica se a camera já chegou no destino atual*/
            if(now.j <= going.j)
                updateGoing();
        }
    }
}

/*Atualiza as variaveis temporais e renderiza novamente o labirinto*/
void idle(){
    float timeNow = glutGet(GLUT_ELAPSED_TIME);
    delta = timeNow - timeOld;
    rotation = timeNow*0.1;
    timeOld = timeNow;
    update(delta*0.01);
    glutPostRedisplay();
}

/*Cria o chão do labirinto*/
void bottom(int i0, int j0, int i1, int j1){
    double zPosition = 0.01d;
    glBegin(GL_QUADS);
    if(now.k == 1)
        glColor3f(0.4f, 0.2f, 0.2f);
    else if (now.k == 2)
        glColor3f(0.2f, 0.4f, 0.2f);
    else
        glColor3f(0.2f, 0.2f, 0.4f);
    glVertex3f(j1*spacing, -i1*spacing,  zPosition);
    glVertex3f(j1*spacing, -i0*spacing,  zPosition);
    glVertex3f(j0*spacing, -i0*spacing, zPosition);
    glVertex3f(j0*spacing, -i1*spacing, zPosition);
    glEnd();
}

/*Cria uma parede do labirinto*/
void wall(float x1, float y1, float x2, float y2, bool inverted){
    float wallHeight = inverted ? -height : height;

    /*Garante que os pontos estejam em ordem*/
    if(x1 == x2){
        x1 += d*0.5;
        x2 -= d*0.5;
    }
    else{
        y1 += d*0.5;
        y2 -= d*0.5;
    }

    /*Cria as varias faces de uma parede*/
    glBegin(GL_QUADS);

      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex3f( x1, y1, 0.0);
      glVertex3f(x1, y1, wallHeight);
      glVertex3f(x2, y1,  wallHeight);
      glVertex3f(x2, y1,  0.0);


      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex3f( x1, y2, 0.0);
      glVertex3f(x1, y2, wallHeight);
      glVertex3f(x2, y2,  wallHeight);
      glVertex3f(x2, y2,  0.0);

      glColor3f(0.0f, 0.0f, 1.0f);
      glVertex3f( x1, y1, 0.0);
      glVertex3f(x1, y1, wallHeight);
      glVertex3f(x1, y2,  wallHeight);
      glVertex3f(x1, y2,  0.0);


      glColor3f(0.0f, 0.0f, 1.0f);
      glVertex3f( x2, y1, 0.0);
      glVertex3f(x2, y1, wallHeight);
      glVertex3f(x2, y2,  wallHeight);
      glVertex3f(x2, y2,  0.0);



      glColor3f(0.5f, 0.5f, 0.0f);
      glVertex3f( x1, y1, 0.0);
      glVertex3f(x1, y2, 0.0);
      glVertex3f(x2, y2,  0.0);
      glVertex3f(x2, y1,  0.0);


      glColor3f(0.5f, 0.5f, 1.0f);
      glVertex3f( x1, y1, wallHeight);
      glVertex3f(x1, y2, wallHeight);
      glVertex3f(x2, y2,  wallHeight);
      glVertex3f(x2, y1,  wallHeight);

   glEnd();
}

/*Desenha o labirinto realizando uma divisão em quatro dele e um afastamento das quatro partes*/
static void drawMaze(bool inverted, int rows, int columns, char **maze){
    /*Desenha as paredes do labirinto*/
    for(int i = 0; i < rows*0.5; i++)
        for(int j = 0; j < columns*0.5; j++){
            if( maze[i][j] == '1'){
                if(i + 1 < rows && maze[i+1][j] == '1'){
                    wall(j*spacing , -i*spacing, j*spacing , -(i+1)*spacing,inverted);
                }
                if(j + 1 < columns && maze[i][j+1] == '1'){
                    wall(j*spacing , -i*spacing, (j+1)*spacing, -i*spacing,inverted);

                }
            }
        }
    /*Desenha o chão*/
    bottom(0,0,rows*0.5 - 1, columns*0.5 - 1);
    glPushMatrix();

    /*Realiza o afastamento dessa parte do labirinto*/
    glTranslated(maze_spacing,0,0);

    /*Desenha as paredes do labirinto*/
    for(int i = 0; i < rows*0.5; i++)
        for(int j = columns*0.5 ; j < columns; j++){
            if( maze[i][j] == '1'){
                if(i + 1 < rows && maze[i+1][j] == '1'){
                    wall(j*spacing , -i*spacing, j*spacing , -(i+1)*spacing,inverted);
                }
                if(j + 1 < columns && maze[i][j+1] == '1'){
                    wall(j*spacing , -i*spacing, (j+1)*spacing, -i*spacing,inverted);

                }
            }
        }
    /*Desenha o chão*/
    bottom(0,columns*0.5,rows*0.5 - 1, columns - 1);
    glPopMatrix();
    glPushMatrix();

    /*Realiza o afastamento dessa parte do labirinto*/
    glTranslated(0,-(maze_spacing),0);

    /*Desenha as paredes do labirinto*/
    for(int i = rows*0.5 ; i < rows; i++)
        for(int j = 0; j < columns*0.5; j++){
            if( maze[i][j] == '1'){
                if(i + 1 < rows && maze[i+1][j] == '1'){
                    wall(j*spacing , -i*spacing, j*spacing , -(i+1)*spacing,inverted);
                }
                if(j + 1 < columns && maze[i][j+1] == '1'){
                    wall(j*spacing , -i*spacing, (j+1)*spacing, -i*spacing,inverted);

                }
            }
        }
    /*Desenha o chão*/
    bottom(rows*0.5,0,rows - 1, columns*0.5 - 1);
    glPopMatrix();
    glPushMatrix();

    /*Realiza o afastamento dessa parte do labirinto*/
    glTranslated(maze_spacing,-(maze_spacing),0);

    /*Desenha as paredes do labirinto*/
    for(int i = rows*0.5 ; i < rows; i++)
        for(int j = columns*0.5 ; j < columns; j++){
            if( maze[i][j] == '1'){
                if(i + 1 < rows && maze[i+1][j] == '1'){
                    wall(j*spacing , -i*spacing, j*spacing , -(i+1)*spacing,inverted);
                }
                if(j + 1 < columns && maze[i][j+1] == '1'){
                    wall(j*spacing , -i*spacing, (j+1)*spacing, -i*spacing,inverted);

                }
            }
        }
    /*Desenha o chão*/
    bottom(rows*0.5,columns*0.5,rows - 1, columns - 1);
    glPopMatrix();

}


/*Desenha o caminho do labirinto*/
void draw_path3D(){
        for (int i = 0; i < path_size - 1; i++){
            /*Pega 2 pontos de cada vez do vetor de posições e cria uma reta entre eles*/
            struct position3d p0 = path_position3d[i];
            struct position3d p1 = path_position3d[i+1];

            /*Altura em que a linha será desenhada*/
            float heightP = height;
            /*Inverte o tamanho caso o labirinto esteja de ponta cabeça*/
            if(p0.w != now.w)
                heightP = -height;

            glPushMatrix();

            /*Afasta a linha de acordo com a posição no labirinto e desenha a linha de acordo com o labirinto na frente da camera*/
            if(p0.k == (now.k + 1)%3 ){
                glRotated(90,1,0,0);
                glTranslated(0,(maze_spacing)*0.5 + (float)rows0*0.5*spacing,(maze_spacing)*0.5 + (float)columns0*0.5*spacing);
            }
            else if (p0.k == (now.k + 2)%3){
                glRotated(90,0,1,0);
                glTranslated(-((maze_spacing)*0.5 + (float)rows0*0.5*spacing ),0,((maze_spacing)*0.5 + (float)columns0*0.5*spacing));
            }

            if(p0.i < rows0*0.5){
                if(p0.j >= (int)(columns0*0.5))
                    glTranslated(maze_spacing,0,0);
            }else{
                if(p0.j < rows0*0.5)
                    glTranslated(0,-(maze_spacing),0);
                else
                    glTranslated(maze_spacing,-(maze_spacing),0);
            }

            /*Desemha o caminho*/
            glBegin(GL_LINES);
                glVertex3f(p0.j*spacing , -p0.i*spacing, heightP);
                glVertex3f(p1.j*spacing , -p1.i*spacing, heightP);
            glEnd();
            glPopMatrix();
        }
}



/*Realiza a busca em profundidade com poda no labirinto dado uma posição inicial e uma final*/
bool AIPath3D(struct position3d pI, struct position3d pF, struct node3d** path){
    int i = pI.i, j = pI.j, k = pI.k, w = pI.w;

    /*Proxima posição*/
    struct position3d next = {i,j,k,w};

    /*Realiza a poda nos ciclos da árvore*/
    if(isLooping[k][w][i][j])
        return FALSE;
    else
        isLooping[k][w][i][j] = TRUE;

    /*Caso atinja a posição final, a posição é empilhada e é retornado true para empilhar a posição do nó pai em cascata até que atinja o fim*/
    if(pI.i == pF.i && pI.j == pF.j && pI.k == pF.k && pI.w == pF.w){
        push(pF, path);
        path_size++;
        return TRUE;
    }

    /*Possiveis nós da arvore de busca*/
    next.i = i - 1;
    next.j = j;
    if(labirintos[k][w][i - 1][j] == '0' && AIPath3D(next,pF,path)){
        push(pI,path);
        path_size++;
        return TRUE;
    }

    next.i = i;
    next.j = j - 1;
    if(labirintos[k][w][i][j - 1] == '0' && AIPath3D(next,pF,path)){
        push(pI,path);
        path_size++;
        return TRUE;
    }

    next.i = i + 1;
    next.j = j;
    if(labirintos[k][w][i + 1][j] == '0' && AIPath3D(next,pF,path)){
        push(pI,path);
        path_size++;
        return TRUE;
    }

    next.i = i;
    next.j = j + 1;
    if(labirintos[k][w][i][j + 1] == '0' && AIPath3D(next,pF,path)){
        push(pI,path);
        path_size++;
        return TRUE;
    }

    /*Nós que representam a passagem de um labirinto para outro*/
    if(i == (int)(rows0*0.5 - 1)){
        next.i = i+1;
        next.j = j;
        next.k = (k == 0 ? 1 : k == 1 ? 0 : 1);
        next.w = 1;
        if(labirintos[next.k][next.w][next.i][next.j] == '0' && AIPath3D(next,pF,path)){
            push(pI,path);
            path_size++;
            return TRUE;
        }
    }
    else if(i == (int)rows0*0.5){
        next.i = i+1;
        next.j = j;
        next.k = (k == 0 ? 1 : k == 1 ? 0 : 1);
        next.w = 0;
        if(labirintos[next.k][next.w][next.i][next.j] == '0' && AIPath3D(next,pF,path)){
            push(pI,path);
            path_size++;
            return TRUE;
        }
    }
    if(j == (int)(columns0*0.5 - 1)){
        next.i = i;
        next.j = j + 1;
        next.k = (k == 0 || k == 1 ? 2 : 0);
        next.w = 1;
        if(labirintos[next.k][next.w][next.i][next.j] == '0' && AIPath3D(next,pF,path)){
            push(pI,path);
            path_size++;
            return TRUE;
        }
    }
    else if(j == (int)(columns0*0.5)){
        next.i = i;
        next.j = j + 1;
        next.k = (k == 0 || k == 1 ? 2 : 0);
        next.w = 0;
        if(labirintos[next.k][next.w][next.i][next.j] == '0' && AIPath3D(next,pF,path)){
            push(pI,path);
            path_size++;
            return TRUE;
        }
    }

    /*Informa que não existe um caminho entre as posições passadas*/
    return FALSE;
};




void initGL() {
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClearDepth(1.0f);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
   glShadeModel(GL_SMOOTH);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

/*Desenha os labirintos*/
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /*Configura a camera de acordo com a posição do caminho sendo percorrido*/
    double eX = 0, eY = 0, eZ = 0, cX = 0, cY = 0, cZ = 0;
    if(view == 0 || view == 1){
        if(view == 1){
            eX = columns0*spacing*0.5 - (-now.j + columns0 - 1)*spacing + maze_spacing - 1;
            eY = -rows0*spacing*0.5 + (-now.i + rows0 - 1)*spacing - maze_spacing + 1 + 1.5;
            eZ = z -24;
            cX = eX;
            cY = eY;
            cZ = z+lz -27;
            glRotated(90*view,1,0,0);
        }
        else if (view == 0){
            eX = columns0*spacing*0.5 - (-now.j + columns0 - 1)*spacing + maze_spacing - 1;
            eY = -rows0*spacing*0.5 + (-now.i + rows0 - 1)*spacing - maze_spacing + 1;
            eZ = z -20;
            cX = eX;
            cY = eY;
            cZ = z+lz -20;
        }
        /*Realiza o espaçamento entre partes do labirinto*/
        if(now.j < columns0*0.5){
            glTranslated(maze_spacing,0,0);
        }
        if(now.i < rows0*0.5)
            glTranslated(0,-(maze_spacing),0);

        /*Configura a camera*/
        gluLookAt(eX, eY, eZ,
                cX, cY, cZ,
                0.0f, 1.0f, 0.0f);

    }else{
        glRotated(rotation,0,1,1);
        glTranslatef(0,0, -100.0f);

    }



    /*Posicionamento do labirinto como um todo*/
   glTranslatef(-columns0*spacing*0.5, rows0*spacing*0.5, -20.0f);

   /*Desenho do caminho sendo percorrido*/
   glColor3d(1,1,1);
   draw_path3D();

   /*Desenha o labirinto que a camera está atualmente*/
   drawMaze(FALSE,rows0,columns0,labirintos[now.k][now.w]);
   drawMaze(TRUE,rows0,columns0,labirintos[now.k][(now.w + 1)%2]);


   /*Desenha os labirintos adajcentes fazendo as suas respectivas rotações e posiionamento no centro*/
   glPushMatrix();

   glRotated(90,1,0,0);
   glTranslated(0,(maze_spacing)*0.5 + (float)rows0*0.5*spacing,(maze_spacing)*0.5 + (float)columns0*0.5*spacing);

   drawMaze(FALSE,rows0,columns0,labirintos[(now.k + 1)%3][now.w]);
   drawMaze(TRUE,rows0,columns0,labirintos[(now.k + 1)%3][(now.w + 1)%2]);

   glPopMatrix();

   glRotated(90,0,1,0);
   glTranslated(-((maze_spacing)*0.5 + (float)rows0*0.5*spacing ),0,((maze_spacing)*0.5 + (float)columns0*0.5*spacing));

   drawMaze(FALSE,rows0,columns0,labirintos[(now.k + 2)%3][now.w]);
   drawMaze(TRUE,rows0,columns0,labirintos[(now.k + 2)%3][(now.w + 1)%2]);

   glutSwapBuffers();
}


void reshape(int w, int h){

if (h == 0)
h = 1;
float ratio = w * 1.0 / h;

glMatrixMode(GL_PROJECTION);

glLoadIdentity();

glViewport(0, 0, w, h);

gluPerspective(-100.0f, ratio, 0.1f, 100.0f);

glMatrixMode(GL_MODELVIEW);
}

/*Trata eventos do teclado*/
void processSpecialKeys(int key, int xx, int yy){

    float fraction = 0.1f;

    switch (key) {
        case GLUT_KEY_LEFT :
            angle -= 0.01f;
            lx = sin(angle);
            lz = -cos(angle);
            break;
        case GLUT_KEY_RIGHT :
            angle += 0.01f;
            lx = sin(angle);
            lz = -cos(angle);
            break;

        /*Afasta e aproxima o labirinto*/
        case GLUT_KEY_UP :
            x += lx * fraction;
            z += lz * fraction;
            break;
        case GLUT_KEY_DOWN :
            x -= lx * fraction;
            z -= lz * fraction;
            break;
    }
}

void save(){
    for(int k = 0; k < 3; k++)
        for(int w = 0; w < 2; w++){
            char number[7] = {k + '0', w + '0', '.', 't', 'x', 't', '\0'};
            char file_name[30] = "labirinto";
            strcat(file_name,number);


            FILE *f = fopen(file_name, "w");
            if (f == NULL){
                printf("Error opening file!\n");
                exit(1);
            }

            /* print some text */
            for(int i = 0; i < rows0; i++){
                const char *text = labirintos[k][w][i];
                fprintf(f, "%s\n", text);
            }

            fclose(f);
        }
}
static void load(){


    for(int k = 0; k < 3; k++)
        for(int w = 0; w < 2; w++){
            char number[7] = {k + '0', w + '0', '.', 't', 'x', 't', '\0'};
            char file_name[30] = "labirinto";
            strcat(file_name,number);


            FILE *f = fopen(file_name, "r");
            if (f == NULL){
                printf("Error opening file!\n");
                exit(1);
            }
            int i = 0;
            int j = 0;
            while(i*j < rows0*columns0){
                    char aux;
                    fread(&aux,sizeof(char),1,f);
                    if(aux == '0' || aux == '1'){
                        labirintos[k][w][i][j] = aux;
                        j++;
                        i = j == columns0 ? i + 1: i;
                        j = 0;
                    }

            }
        }




}

void processMenuEvents(int option) {
    if(option < 3){
        view = option;
    }
    else if(option == 3){
        save();
    }
    else if(option == 4){
        load();
    }
    else if(option == 5);

}

void createGLUTMenus() {

	int menu;

	// create the menu and
	// tell glut that "processMenuEvents" will
	// handle the events
	menu = glutCreateMenu(processMenuEvents);

	//add entries to our menu
	glutAddMenuEntry("Vista panorâmica", 0);
	glutAddMenuEntry("Vista inclinada", 1);
	glutAddMenuEntry("Vista Labirinto inteiro", 2);
	glutAddMenuEntry("Save", 3);
	glutAddMenuEntry("Load", 4);

	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}


int main(int argc, char** argv) {

    /*Posição inicial*/
    struct position3d pI3, pF3;

    /*Pilha*/
    struct node3d* path3 = NULL;


    /*Inicializa as dimensões dos labirintos*/
    printf("Insira o lado L do labirinto(de preferencia maior ou igual a 20): ");
    scanf("%d",&rows0);
    columns0 = rows0 ;

    /*Garante que os labirintos sejam diferentes em cada execução*/
    srand(time(NULL));

    for(int i = 0; i < 3; i ++){
        labirintos[i][0] = mazeGen(rows0,columns0);
        labirintos[i][1] = mazeGen(rows0,columns0);
    }
    save();

    /*Inicializa as matrizes de poda*/
    for(int i = 0; i<3; i++){
        isLooping[i][0] = initIsLooping(rows0,columns0);
        isLooping[i][1] = initIsLooping(rows0,columns0);
    }

    /*Inicializa a Posição inicial e final*/
    initBeginEnd3D(&pI3,&pF3);

    /*Realiza a busca em profundidade*/
    AIPath3D(pI3,pF3,&path3);

    /*Cria o vetor com o caminho até o final*/
    path_position3d = (struct position3d*) malloc(path_size*sizeof(struct position3d));
    for(int i = 0; i < path_size; i++){
        path_position3d[i] = pop(&path3);
    }


    glutInit(&argc, argv);            // Initialize GLUT
    glutInitDisplayMode(GLUT_DOUBLE); // Enable double buffered mode
    glutInitWindowSize(600, 600);   // Set the window's initial width & height
    glutInitWindowPosition(100, 100); // Position the window's initial top-left corner
    glutCreateWindow("Labirinto");          // Create window with the given title
    glutDisplayFunc(display);       // Register callback handler for window re-paint event
    glutReshapeFunc(reshape);       // Register callback handler for window re-size event
    initGL();
    createGLUTMenus();
    glutIdleFunc(idle);

    glutSpecialFunc(processSpecialKeys);

    glClearColor(1,1,1,1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
                         // Our own OpenGL initialization
   glutMainLoop();                 // Enter the infinite event-processing loop
   return 0;
}
