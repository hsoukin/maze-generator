#include <iostream>
#include <stack>
#include <random>
#include <time.h>

// PGE applications must be compiled with the following settings (on linux):
// -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17
// on Visual Studio, using the Microsoft Compiler included you should be fine

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

const int DIM = 60; // maze dimensions for your nightmare fuel


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
Cardinal pickDirection(const uint8_t &bounds = 0b0000, const uint8_t &v_list = 0b0000)
{
	uint8_t mask = ((~bounds) & (~v_list)) & 0x0F;
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

    Cell() : x(0), y(0), bounds(0) {} // array mumbo jumbo

    Cell(int _x, int _y, uint8_t _bounds = Cardinal::NONE)
        : x(_x), y(_y), bounds(_bounds) {} // i don't think i ever use this


    // direction is used to draw over the wall of the cell traveling *to*
    void DrawSelf(olc::PixelGameEngine* pge, Cardinal direction)
    {
        {
            int x = this->x * (3+1); // pixels per square + border gap
            int y = this->y * (3+1); // = x*3 ( square ) + x*1 ( gap )

            pge->FillRect(x, y, 3, 3, olc::GREEN);

            // now clear the border where moving to
            switch(direction)
            {
                case Cardinal::N:
                    pge->DrawLine(x, y-1, x+2, y-1, olc::GREEN);
                    break;
                case Cardinal::E:
                    pge->DrawLine(x+3, y, x+3, y+2, olc::GREEN);
                    break;
                case Cardinal::W:
                    pge->DrawLine(x-1, y, x-1, y+2, olc::GREEN);
                    break;
                case Cardinal::S:
                    pge->DrawLine(x, y+3, x+2, y+3, olc::GREEN);
                    break;
                default:
                    break;
            }
        }
    }
};


class MazeGen : public olc::PixelGameEngine
{
private:
    Cell grid[DIM*DIM];        // i hate matrixes
    std::stack<Cell*> history; // contains a history of visited cells

public:
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
        for(int y = 0; y < DIM; y++)
            for(int x = 0; x < DIM; x++)
            {
                Cell& cell = grid[y * DIM + x];
                cell.x = x;
                cell.y = y;

                if(x == 0)
                    cell.bounds |= Cardinal::W;
                else if(x == DIM-1)
                    cell.bounds |= Cardinal::E;
                if(y == 0)
                    cell.bounds |= Cardinal::N;
                else if(y == DIM-1)
                    cell.bounds |= Cardinal::S;
            }

        // we then set cell (0, 0), aka cell 0, as the first visited cell.
        history.push(&grid[0]);
        srand(time(NULL));

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
            int pos = y * DIM + x;

            // checks all neighbours to determine whether they've been visited before
            uint8_t visited_list = 0b0000;
            if(y != 0 && grid[pos - DIM].visited)
                visited_list |= Cardinal::N;
            if(x != 0 && grid[pos-1].visited)
                visited_list |= Cardinal::W;
            if(x != DIM-1 && grid[pos+1].visited)
                visited_list |= Cardinal::E;
            if(y != DIM-1 && grid[pos + DIM].visited)
                visited_list |= Cardinal::S;

            // out of the pool of available directions to go to, picks one at random (sort of)
            Cardinal toGo = pickDirection(current.bounds, visited_list);

            // it's worth noting that after every path calculation, the *previous* location
            // is drawn instead of the new one. I just felt this was more convenient
            switch(toGo)
            {
                case Cardinal::NONE:
                    // when you hit a dead end
                    grid[pos].DrawSelf(this, toGo);
                    history.pop();
                    break;

                case Cardinal::N:
                    grid[pos].bounds &= ~Cardinal::N;
                    grid[pos - DIM].bounds &= ~Cardinal::S; 

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos - DIM]);
                    break;
                
                case Cardinal::E:
                    grid[pos].bounds &= ~Cardinal::E;
                    grid[pos + 1].bounds &= ~Cardinal::W;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos + 1]);
                    break;
                
                case Cardinal::W:
                    grid[pos].bounds &= ~Cardinal::W;
                    grid[pos - 1].bounds &= ~Cardinal::E;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos - 1]);
                    break;
                
                case Cardinal::S:
                    grid[pos].bounds &= ~Cardinal::S;
                    grid[pos + DIM].bounds &= ~Cardinal::N;

                    grid[pos].DrawSelf(this, toGo);

                    history.push(&grid[pos + DIM]);
                    break;
                
                // i remember not being able to compile on code::blocks
                // without specifying a default case. hm. modern c++ might not need this
                default:
                    break;
            }
        }
        
        // quit if the user presses Enter at any point during the process
        if(GetKey(olc::ENTER).bPressed)
            return false;
        return true;
    }   
    
    // no need for this but i kept it in because you never know
    bool OnUserDestroy() override
    { 
        return true; 
    }
};

int main()
{
    MazeGen window; // each one cell is three by three, with a one by three border
    if(window.Construct(DIM*3 + DIM - 1, DIM*3 + DIM - 1, 3, 3))
        window.Start();

    return 0;
}
