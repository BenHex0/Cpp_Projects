#include <windows.h>
#include <iostream>
#include <vector>
#include <limits>
using namespace std;

const int ROW = 25;
const int COLUM = 50;
vector<COORD> snake_position(ROW * COLUM , {15 , 0});
COORD last_position;
int snake_length = 1;

void drawMap(HANDLE output)
{
    for(int i = 0; i < COLUM; i++)
    {
        SetConsoleCursorPosition(output , {(short)i , 1});
        cout << '#';
    }
    for(int i = 0; i < COLUM; i++)
    {
        SetConsoleCursorPosition(output , {(short)i , ROW});
        cout << '#';
    }
    for(int i = 1; i <= ROW; i++)
    {
        SetConsoleCursorPosition(output , {0 , (short)i});
        cout << '#';
    }
    for(int i = 1; i <= ROW; i++)
    {
        SetConsoleCursorPosition(output , {COLUM , (short)i});
        cout << '#';
    }
}

void drawPlayer(HANDLE output)
{
    int i = 0;
    for(; i < snake_length; i++)
    {
        SetConsoleCursorPosition(output , snake_position[i]);
        if(i == 0)
        std::cout << "O";
        else
        std::cout << "o";
    }
    SetConsoleCursorPosition(output , last_position);
    cout << ' ';
}

bool collision(COORD position)
{
    if(position.X == COLUM - 1 || position.X == 0)
    return true;
    if(position.Y == ROW || position.Y == 1)
    return true;

    for(int i = 1; i < snake_length; i++)
    if(position.X == snake_position[i].X && position.Y == snake_position[i].Y)
    return true;

    return false;
}

void control(char& move)
{
    if((GetAsyncKeyState('A') & 0b1) && move != 'd')
    move = 'a';
    else if((GetAsyncKeyState('D') & 0b1) && move != 'a')
    move = 'd';
    else if((GetAsyncKeyState('W') & 0b1 && move != 's'))
    move = 'w';
    else if((GetAsyncKeyState('S') & 0b1) && move != 'w')
    move = 's';
}

void moveDirection(char control , short& x , short& y)
{
    if(snake_length == 1)
    {
        last_position.X = x;
        last_position.Y = y;
    }
    else
    {
        last_position = snake_position[snake_length - 1];
    }


    switch (control)
    {
    case 'a':
        x--;
        break;
    case 'd':
        x++;
        break;
    case 'w':
        y--;
        break;
    case 's':
        y++;
        break;
    default:
        break;
    }

    for(int i = snake_length - 1; i >= 0; i--)
    {
        snake_position[i] = snake_position[i - 1]; 
    }
    snake_position[0] = {x , y};
}

bool eatFruit(COORD snake_pos , COORD fruit_pos)
{
    if(snake_pos.X == fruit_pos.X && snake_pos.Y == fruit_pos.Y)
    return true;
    return false;
}

void UpdateFruit(int& score , COORD& fruit_pos , HANDLE output)
{
    short x_min = 4;
    short y_min = 3;
    short x_max = 48;
    short y_max = 24;
    
    if(eatFruit(snake_position[0] , fruit_pos))
    {
        score++;
        snake_length++;       
        short x = x_min + (rand() % (x_max - x_min + 1));
        short y = y_min + (rand() % (y_max - y_min + 1));
        fruit_pos = {x , y};
    }
    
    SetConsoleCursorPosition(output , fruit_pos);
    cout << '*';
}

void dead(bool& again)
{
    system("cls");
    cout << "Dead\n";
    cout << "\nPlay Again ? Y/N : ";
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    char choise;
    cin.get(choise);
    
    switch(choise)
    {
        case 'y':
        again = true;
        break;
        case 'n' :
        again = false;
        default:
        break;
    }
    system("cls");
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main()
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD position = {3 , 2};
    
    srand(time(NULL));
    snake_position[0] = position;
    int score = 0;
    char move = 'd';
    bool again = true;
    COORD fruit_pos = {5 , 7};
    while(again)
    {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(output, &cursorInfo); 
        cursorInfo.bVisible = FALSE;                
        SetConsoleCursorInfo(output, &cursorInfo);
        SetConsoleCursorPosition(output , {0 , 0});
        cout << "Score : " << score << '\n';
        control(move);
        drawMap(output);
        UpdateFruit(score , fruit_pos , output);
        moveDirection(move , position.X , position.Y);
        drawPlayer(output);
        Sleep(150);

        if(collision(position))
        {
            dead(again);
            position = {3 , 2};
            snake_position[0] = position;
            fruit_pos = {5 , 7};
            score = 0;
            move = 'd';
            snake_length = 1;
        }
    }
}
