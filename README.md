# triangles
Triangles strategy game

Must install Python 3 with TKinter library
Drag mouse between two points to connect. Does not display as you drag.
To get the AI to move, click on the AI button, it will continue to move as that player for the rest of the game.
You can set the maxdepth (to 12 for example) in the first few lines to speed it up but not recommended. 
The AI will search ahead until two moves have been made without making a triangle or to maxdepth.

For C version: 
g++ `pkg-config --cflags gtk+-3.0` -o h AI.cpp Tboard.cpp triangles.cpp `pkg-config --libs gtk+-3.0`
