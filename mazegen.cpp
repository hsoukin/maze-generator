#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <random>
#include <time.h>

// PGE applications must be compiled with the following settings (on linux):
// -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17
// on Visual Studio, using the Microsoft Compiler included you should be fine

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

// I use bitwise operators to work out which direction to pick
// as I feel it streamlines the process. Also wanted to be fancy
enum Cardinal : uint8_t
{
	N = 0b1000,
	E = 0b0100,
	W = 0b0010,
	S = 0b0001,

	NONE = 0b0000,
};

// bounds are effectively only used for the outmost cells in the grid
// v_list contains any neighbours to the current cell which have been visited
Cardinal pickDirection(const uint8_t &v_list = 0b0000)
{
	uint8_t mask = (~v_list) & 0x0F;
	if(mask != 0b0000) do
    {
        uint8_t gen = (rand() % 15) + 1; // random int 1-15
        gen &= mask; // unreachable locations are "masked out"

	    if (gen & Cardinal::N) return Cardinal::N;
	    if (gen & Cardinal::E) return Cardinal::E;
	    if (gen & Cardinal::W) return Cardinal::W;
	    if (gen & Cardinal::S) return Cardinal::S;
    } while(true); // we can't return none if at least one neighbour hasn't been visited...

    return Cardinal::NONE; // only if all neighbours have been visited
}

/////////////////////////////////////////////


struct Cell
{
    int x;
    int y;
    uint8_t bounds = Cardinal::NONE;
    bool visited = false;

    Cell() : x(0), y(0), bounds(0x0F) {} // array mumbo jumbo
    
    ~Cell() {}

    // direction is used to draw over the wall of the cell traveling *to*
    void DrawSelf(olc::PixelGameEngine* pge, uint8_t direction, olc::Pixel color = olc::GREEN)
    {
        {
            int x = this->x * (3+1); // pixels per square + border gap
            int y = this->y * (3+1); // = x*3 ( square ) + x*1 ( gap )

            pge->FillRect(x, y, 3, 3, color);

            // now clear the border where moving to
            if(direction & Cardinal::N)
                pge->DrawLine(x, y-1, x+2, y-1, color);
            if(direction & Cardinal::E)
                pge->DrawLine(x+3, y, x+3, y+2, color);
            if(direction & Cardinal::W)
                pge->DrawLine(x-1, y, x-1, y+2, color);
            if(direction & Cardinal::S)
                pge->DrawLine(x, y+3, x+2, y+3, color);
        }
    }

    friend std::ostream& operator<< (std::ostream &o, Cell const &e)
    {
        o << e.x << ' ' << e.y << ' ';
        if(e.bounds & Cardinal::N)
            o << "N ";
        if(e.bounds & Cardinal::E)
            o << "E ";
        if(e.bounds & Cardinal::W)
            o << "W "; 
        if(e.bounds & Cardinal::S)
            o << "S" ;
        return o;
    }

    friend std::istream& operator>> (std::istream &i, Cell &e)
    {
        std::string line;
        i >> e.x >> e.y;
        std::getline(i, line);

        for(auto& ch : line)
        {
            if(ch == 'N')
                e.bounds |= Cardinal::N;
            else if(ch == 'E')
                e.bounds |= Cardinal::E;
            else if(ch == 'W')
                e.bounds |= Cardinal::W;
            else if(ch == 'S')
                e.bounds |= Cardinal::S;
        }

        return i;
    }

};


inline void log(const Cell *array, int width, int height)
{
    int size = width * height;
    std::ofstream out;
    out.open("logfile.txt");
    out << width << ' ' << height << std::endl; 
    for(int i = 0; i < size; i++)
        out << array[i] << std::endl;
    out.close();
}


class MazeGen : public olc::PixelGameEngine
{
private:
    const int WIDTH;
    const int HEIGHT;

    bool read;

    // grid is an array;
    Cell *grid;                // i hate matrixes
    std::stack<Cell*> history; // contains a history of visited cells

public:
    MazeGen(const int &width, const int &height, Cell *_grid)
        : WIDTH(width), HEIGHT(height)
    {
        sAppName = "Maze Generator";

        if(_grid)
        {
            grid = _grid;
            read = true;
        }
        else
        {
            grid = new Cell[width * height] {};
            read = false;
        }
    }

    bool OnUserCreate() override 
    {
        Clear(olc::VERY_DARK_BLUE);

        // this draws the "grid" structure to the screen.
        // cells are 3 by 3, then followed by a 1 by 3 separator (maze walls)
        for(int y = 0; y < ScreenHeight(); y++)
        {
            if(y != 0 && (y+1) % 4 == 0)
                DrawLine(0, y, ScreenWidth(), y, olc::DARK_CYAN);
            else for(int x = 3; x < ScreenWidth(); x += 4)
                Draw(x, y, olc::DARK_CYAN);
        }

        // i miss python list comprehensions. anyways,
        // this sets the boundaries for the outmost cells of the grid
        if(read)
        {
            for(int i = 0; i < WIDTH*HEIGHT; i++)
                grid[i].DrawSelf(this, ~(grid[i].bounds) & 0x0F);
        }
        else
        {
            for(int y = 0; y < HEIGHT; y++)
                   for(int x = 0; x < WIDTH; x++)
                   {
                       Cell& cell = grid[y * WIDTH + x];
                       cell.x = x;
                       cell.y = y;
                   }

               // we then pick a random starting cell.
               srand(time(NULL));
               history.push(&grid[ rand() % WIDTH*HEIGHT ]);
        }
        
        return true;
    }
    
    bool OnUserUpdate(float fElapsedTime) override
    {
        // if we haven't run out of places to visit
        if(!history.empty())
        {
            // i'm using references to trim down the code and indexing
            Cell &current = *history.top();

            int x = current.x;
            int y = current.y;

            current.visited = true;
            int pos = y * WIDTH + x;

            // checks all neighbours to determine whether they've been visited before
            uint8_t visited_list = 0b0000;
            if(y == 0 || grid[pos - WIDTH].visited)
                visited_list |= Cardinal::N;
            if(x == 0 || grid[pos-1].visited)
                visited_list |= Cardinal::W;
            if(x == WIDTH-1  || grid[pos+1].visited)
                visited_list |= Cardinal::E;
            if(y == HEIGHT-1 || grid[pos + WIDTH].visited)
                visited_list |= Cardinal::S;

            // out of the pool of available directions to go to, picks one at random (sort of)
            Cardinal toGo = pickDirection(visited_list);

            // it's worth noting that after every path calculation, the *previous* location
            // is drawn instead of the new one. I just felt this was more convenient
            switch(toGo)
            {
                case Cardinal::NONE:
                    // when you hit a dead end
                    grid[pos].bounds &= ~(toGo) & 0x0F;

                    grid[pos].DrawSelf(this, toGo);
                    history.pop();
                    break;

                case Cardinal::N:
                    grid[pos].bounds &= ~(toGo) & 0x0F;
                    grid[pos - WIDTH].bounds &= ~(Cardinal::S) & 0x0F;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos - WIDTH]);
                    break;
                
                case Cardinal::E:
                    grid[pos].bounds &= ~(toGo) & 0x0F;
                    grid[pos + 1].bounds &= ~(Cardinal::W) & 0x0F;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos + 1]);
                    break;
                
                case Cardinal::W:
                    grid[pos].bounds &= ~(toGo) & 0x0F;
                    grid[pos - 1].bounds &= ~(Cardinal::E) & 0x0F;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos - 1]);
                    break;
                
                case Cardinal::S:
                    grid[pos].bounds &= ~(toGo) & 0x0F;
                    grid[pos + WIDTH].bounds &= ~(Cardinal::N) & 0x0F;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos + WIDTH]);
                    break;
            }

            if(!history.empty())
                history.top()->DrawSelf(this, Cardinal::NONE, olc::RED);
        }
        
        // quit if the user presses Enter at any point during the process
        if(GetKey(olc::ENTER).bPressed)
            return false;
        
        else if(GetKey(olc::S).bPressed)
            log(grid, WIDTH, HEIGHT);

        return true;
    }   
    
    // no need for this but i kept it in because you never know
    bool OnUserDestroy() override
    {
        delete[] grid;
        return true; 
    }
};

int main(int argc, char **argv)
{
    int width, height;
    Cell *grid = nullptr;
    std::ifstream input;
    bool isRead = false;

    if(argc == 3)
    {
        std::string str = argv[1];
        if(str == "-i")
        {
            isRead = true;
            input.open(argv[2]);
            input >> width >> height;
            grid = new Cell[width * height] {};

            for(int i = 0; i < width*height; i++)
            {    
                grid[i].bounds = 0x00;
                input >> grid[i];
            }
            input.close();
        }
        else
        {
            width  = std::stoi(argv[1]);
            height = std::stoi(argv[2]);
        }
    }
    else do {
        std::cout << "Please insert the width and height of the generated maze,\n"
                  << "separated by spaces, then hit Enter: ";
        std::cin  >> width >> height;

        if(width <= 0 || height <= 0)
            std::cout << "Please insert valid dimensions for the maze.\n\n";
    } while(width <= 1 || height <= 1);

    MazeGen window(width, height, grid);

    // each one cell is three by three, with a one by three border
    if(window.Construct(width*3 + width-1, height*3 + height-1, 3, 3))
        window.Start();
    return 0;
}
