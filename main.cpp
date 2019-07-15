#include <windows.h>
#include "stdio.h"
#include "math.h"


// constant definitions

const char DOMINOPIECE_EMPTY=0;
const char DOMINOPIECE_FALLEN=1;

const char DOMINOPIECE_UPRIGHT_MINVALUE=8;
const char DOMINOPIECE_UPRIGHT_HORIZONTAL=8;
const char DOMINOPIECE_UPRIGHT_VERTICAL=9;
const char DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST=10;
const char DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST=11;
const char DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_NORTH=12;
const char DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_SOUTH=13;
const char DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_WEST=14;
const char DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_EAST=15;
const char DOMINOPIECE_UPRIGHT_MAXVALUE=15;

const char DOMINOPIECE_FALLING_MINVALUE=32;
const char DOMINOPIECE_FALLING_SOUTH=32;
const char DOMINOPIECE_FALLING_NORTH=33;
const char DOMINOPIECE_FALLING_EAST=34;
const char DOMINOPIECE_FALLING_WEST=35;
const char DOMINOPIECE_FALLING_NORTHW=36;
const char DOMINOPIECE_FALLING_NORTHE=37;
const char DOMINOPIECE_FALLING_SOUTHW=38;
const char DOMINOPIECE_FALLING_SOUTHE=39;
const char DOMINOPIECE_FALLING_MEETINGNORTH=43;
const char DOMINOPIECE_FALLING_MEETINGS=44;
const char DOMINOPIECE_FALLING_MEETINGO=45;
const char DOMINOPIECE_FALLING_MEETINGW=46;
const char DOMINOPIECE_FALLING_MAXVALUE=46;

const int MAXTUNNEL=1024;
const int MAXTIMER=MAXTUNNEL;
const int LEFTPOS=10;
const int TOPPOS=10;
const int MAXEnclosement=128;
const int MAX=1024;
const int MAXPATHLENGTHSPOT=1024;

enum {
	GATE_NONE=0,
	GATE_OR,GATE_AND,GATE_XOR,
	GATE_NOT,GATE_NAND,GATE_TUNNEL2,
	GATE_TUNNEL1,GATE_PATHLENSPOT,
	GATE_MOVE2,GATE_MOVE1,GATE_MOVENORTH,
	GATE_MOVESOUTH,GATE_MOVE,GATE_MOVERIGHT,
	GATE_MOVE0,GATE_DEL2,GATE_DEL1 
};

const int MAXX=64;
const int MAXY=40;
const int MAXPAGEX=4;
const int MAXPAGEY=8;
const int MAXXMP=MAXX*MAXPAGEX;
const int MAXYMP=MAXY*MAXPAGEY;


// object definitions

struct Enclosement {
	int lx,ly,rx,ry;
};

struct Tunnel {
	int falling_into_and_beyond; 
	// takes DOMINOPIECE_FALLING__XXX
	int length_inaddition_to_stairs; 
	// 1 means a a total of 3 grid squares are
	// used for the tunnel: entry-the tunnel-exit
};

struct TunnelTimer {
	int x,y,timer;
	int faellt; // DOMINOPIECE_FALLING_;
};

struct PathlengthSpot {
	int x,y,pathlen;
};


// global variables

char dominogrid[MAXXMP][MAXYMP];
char dominogridtmp[MAXXMP][MAXYMP];
char startingpiecedominogrid[MAXXMP][MAXYMP];
Tunnel tunneldominogrid[MAXXMP][MAXYMP];
Tunnel tunneldominogridtmp[MAXXMP][MAXYMP];
HBRUSH hbrred,hbrwhite,hbrgray,hbrblack,hbrgreen;
int gate;
int enclosementtotal;
Enclosement enclosements[MAXEnclosement];
Enclosement tunnel[MAXTUNNEL];
int tunneltotal,tunnelx0,tunnely0;
int movex0,movex1,movey0,movey1,delx0,dely0;
int tunneltimertotal;
TunnelTimer tunneltimer[MAXTIMER];
PathlengthSpot resultspot[MAXPATHLENGTHSPOT];
int PathlengthSpotanz;
int DOMINOPIECESIZE=16;
int DOMINOPIECESIZEHALF=DOMINOPIECESIZE >> 1;
HWND globalwnd;
HDC globaldc;
int offsetx=0,offsety=0;


// forward declaration

void drawdominogrid(void);
void drawdominogrid(const int,const int);
int searchPathlengthSpot(const int ax,const int ay);
void erase(const int lx,const int ly,const int rx,const int ry);
LRESULT CALLBACK wndproc(HWND,UINT,WPARAM,LPARAM);


// function definitions

void swap(Enclosement& a,Enclosement& b) {
	Enclosement c;
	c.lx=a.lx; a.lx=b.lx; b.lx=c.lx;
	c.ly=a.ly; a.ly=b.ly; b.ly=c.ly;
	c.rx=a.rx; a.rx=b.rx; b.rx=c.rx;
	c.ry=a.ry; a.ry=b.ry; b.ry=c.ry;
}

void swap(PathlengthSpot& a,PathlengthSpot& b) {
	PathlengthSpot c;
	c.x=a.x; a.x=b.x; b.x=c.x;
	c.y=a.y; a.y=b.y; b.y=c.y;
	c.pathlen=a.pathlen; a.pathlen=b.pathlen; b.pathlen=c.pathlen;
}

inline int max(const int a,const int b) {
	if (a > b) return a;
	return b;
}

inline int min(const int a,const int b) {
	if (a < b) return a;
	return b;
}

// initialize tunnels as non-existent
void initTunnel(void) {
	for(int x=0;x<MAXXMP;x++) for(int y=0;y<MAXYMP;y++) {
		tunneldominogrid[x][y].falling_into_and_beyond=DOMINOPIECE_EMPTY;
	}
	tunneltotal=0;
	tunneltimertotal=0;
}

// initialize grid with empty cells
void initdominogrid(void) {
	for(int x=0;x<MAXXMP;x++) for(int y=0;y<MAXYMP;y++) {
		dominogrid[x][y]=DOMINOPIECE_EMPTY;
		startingpiecedominogrid[x][y]=DOMINOPIECE_EMPTY;
	}
	
	startingpiecedominogrid[0][0]=DOMINOPIECE_FALLING_EAST;
}

// add tunnel to the array and set it to active
void addActiveTunnelAsTimer(const int fx,const int fy,const int atimer,const int af) {
	if (tunneltimertotal > (MAXTIMER-8)) {
		MessageBox(NULL,"Error. Too many active tunnels/timers. Resume but behavior is not checked.",MB_OK,NULL);
		return;
	}
	
	tunneltimer[tunneltimertotal].x=fx;
	tunneltimer[tunneltimertotal].y=fy;
	tunneltimer[tunneltimertotal].timer=atimer;
	tunneltimer[tunneltimertotal].faellt=af;
	tunneltimertotal++;
}

void flipPathLength(const int ax,const int ay) {
	int idx=searchPathlengthSpot(ax,ay);
	if (idx<0) return;
	if (resultspot[idx].pathlen < 0) 
		resultspot[idx].pathlen=-resultspot[idx].pathlen; 
	else resultspot[idx].pathlen=0;
}

void startDomino(void) {
	if (PathlengthSpotanz>0) {
		for(int i=0;i<PathlengthSpotanz;i++) resultspot[i].pathlen=0; 
	}
	
	for(int x=0;x<MAXXMP;x++) 
	for(int y=0;y<MAXYMP;y++) 
	if ( (dominogrid[x][y] >= DOMINOPIECE_UPRIGHT_MINVALUE) && (dominogrid[x][y] <= DOMINOPIECE_UPRIGHT_MAXVALUE) ) 
	{
		if ( (startingpiecedominogrid[x][y] >= DOMINOPIECE_FALLING_MINVALUE) && (startingpiecedominogrid[x][y] <= DOMINOPIECE_FALLING_MAXVALUE) ) 
		{
			switch (startingpiecedominogrid[x][y]) {
				case DOMINOPIECE_FALLING_NORTH: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTH; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTHE; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTHW; break;
						case DOMINOPIECE_UPRIGHT_VERTICAL: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_SOUTH: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTH; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTHE; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTHW; break;
						case DOMINOPIECE_UPRIGHT_VERTICAL: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_EAST: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_VERTICAL: dominogrid[x][y]=DOMINOPIECE_FALLING_EAST; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTHE; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTHE; break;
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_WEST: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_VERTICAL: dominogrid[x][y]=DOMINOPIECE_FALLING_WEST; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTHW; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTHW; break;
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_NORTHW: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_VERTICAL: dominogrid[x][y]=DOMINOPIECE_FALLING_WEST; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTHW; break;
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTH; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_NORTHE: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_VERTICAL: dominogrid[x][y]=DOMINOPIECE_FALLING_EAST; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTHE; break;
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogrid[x][y]=DOMINOPIECE_FALLING_NORTH; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_SOUTHW: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_VERTICAL: dominogrid[x][y]=DOMINOPIECE_FALLING_WEST; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTHW; break;
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTH; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: break;
					}
					break;
				}
				case DOMINOPIECE_FALLING_SOUTHE: {
					switch (dominogrid[x][y]) {
						case DOMINOPIECE_UPRIGHT_VERTICAL: dominogrid[x][y]=DOMINOPIECE_FALLING_EAST; break;
						case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTHE; break;
						case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogrid[x][y]=DOMINOPIECE_FALLING_SOUTH; break;
						case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: break;
					}
					break;
				}
			} // switch
			
			// mark as done
			startingpiecedominogrid[x][y]=DOMINOPIECE_EMPTY;
		}
	}
	
	int changed=1;
	char tmp[1024];
	int counttimesteps=0;
	while (changed) {
		changed=0;
		counttimesteps++;
		
		for(int x=0;x<MAXXMP;x++) 
		for(int y=0;y<MAXYMP;y++) dominogridtmp[x][y]=DOMINOPIECE_EMPTY;

		// set resultspots to current timesteps
		for(int i=0;i<PathlengthSpotanz;i++) if (resultspot[i].pathlen <= 0) resultspot[i].pathlen=-counttimesteps;
		
		// propagate falling domino pieces within
		// the tunbnels virtually
		if (tunneltimertotal>0) {
			int activetimer=0;
			for(int i=0;i<tunneltimertotal;i++) {
				if (tunneltimer[i].timer==0) {
					activetimer=1;
					tunneltimer[i].timer=-1;
					dominogridtmp[tunneltimer[i].x][tunneltimer[i].y]=tunneltimer[i].faellt;
				} else if (tunneltimer[i].timer > 0) {
					tunneltimer[i].timer--;
					activetimer=1;
				}
			}
			if (activetimer) {
				changed=1;
			} else { 
				tunneltimertotal=0; 
			}
		} // if

		// propagate falling domino pieces in their
		// respective direction
		for(int x=0;x<MAXXMP;x++) for(int y=0;y<MAXYMP;y++) if (dominogrid[x][y] != DOMINOPIECE_FALLEN) {
			switch (dominogrid[x][y]) {
				case DOMINOPIECE_FALLING_MEETINGNORTH: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					if (y>0) {
						if (x>0) {
							switch (dominogrid[x-1][y-1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_NORTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_WEST; break;
								case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_NORTHW; break;
							}
						}
						if (x<(MAXXMP-1)) {
							switch (dominogrid[x+1][y-1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_NORTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_EAST; break;
								case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_NORTHE; break;
							}
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_MEETINGS: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					if (y<(MAXYMP-1)) {
						if (x>0) {
							switch (dominogrid[x-1][y+1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_WEST; break;
								case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_SOUTHW; break;
							}
						}
						if (x<(MAXXMP-1)) {
							switch (dominogrid[x+1][y+1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_EAST; break;
								case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_SOUTHE; break;
							}
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_MEETINGW: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					if (x>0) {
						if (y>0) {
							switch (dominogrid[x-1][y-1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_NORTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_WEST; break;
								case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_NORTHW; break;
							}
						}
						if (y<(MAXYMP-1)) {
							switch (dominogrid[x-1][y+1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_WEST; break;
								case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_SOUTHW; break;
							}
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_MEETINGO: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					if (x<(MAXXMP-1)) {
						if (y>0) {
							switch (dominogrid[x+1][y-1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_NORTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_EAST; break;
								case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_NORTHE; break;
							}
						}
						if (y<(MAXYMP-1)) {
							switch (dominogrid[x+1][y+1]) {
								case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
								case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_EAST; break;
								case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_SOUTHE; break;
							}
						}
					}

					break;
				}
				case DOMINOPIECE_FALLING_SOUTHW: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);
						
					if ( (x>0) && (y>0) ) {
						switch (dominogrid[x-1][y+1]) {
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_WEST; break;
							case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x-1][y+1]=DOMINOPIECE_FALLING_SOUTHW; break;
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_SOUTHE: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);
						
					if ( (x<(MAXXMP-1)) && (y<(MAXYMP-1)) ) {
						switch (dominogrid[x+1][y+1]) {
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_EAST; break;
							case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x+1][y+1]=DOMINOPIECE_FALLING_SOUTHE; break;
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_NORTHW: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);
						
					if ( (x>0) && (y>0) ) {
						switch (dominogrid[x-1][y-1]) {
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_NORTH; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_WEST; break;
							case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x-1][y-1]=DOMINOPIECE_FALLING_NORTHW; break;
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_NORTHE: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);
						
					if ( (x<(MAXXMP-1)) && (y>0) ) {
						switch (dominogrid[x+1][y-1]) {
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_NORTH; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_EAST; break;
							case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x+1][y-1]=DOMINOPIECE_FALLING_NORTHE; break;
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_NORTH: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					// beginnt hier ein Tunnel ?
					if (tunneldominogrid[x][y].falling_into_and_beyond==DOMINOPIECE_FALLING_NORTH) {
						addActiveTunnelAsTimer(x,y-(tunneldominogrid[x][y].length_inaddition_to_stairs+1),tunneldominogrid[x][y].length_inaddition_to_stairs-1,DOMINOPIECE_FALLING_NORTH);
						break;
					}
					
					if (y>0) {
						switch (dominogrid[x][y-1]) {
							case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_NORTH: dominogridtmp[x][y-1]=DOMINOPIECE_FALLING_MEETINGNORTH; break;
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x][y-1]=DOMINOPIECE_FALLING_NORTH; break;
							case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x][y-1]=DOMINOPIECE_FALLING_NORTHE; break;
							case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x][y-1]=DOMINOPIECE_FALLING_NORTHW; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: {
								// Stein wird nur umgeworfen und damit ein Pfad entfernt
								dominogridtmp[x][y-1]=DOMINOPIECE_FALLEN; 
								flipPathLength(x,y-1);
								break;
							}
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_WEST: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					// does the falling domino reach a tunnel entry ?
					if (tunneldominogrid[x][y].falling_into_and_beyond==DOMINOPIECE_FALLING_WEST) {
						addActiveTunnelAsTimer(x-(tunneldominogrid[x][y].length_inaddition_to_stairs+1),y,tunneldominogrid[x][y].length_inaddition_to_stairs-1,DOMINOPIECE_FALLING_WEST);
						break;
					}
						
					if (x>0) {
						switch (dominogrid[x-1][y]) {
							case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_WEST: dominogridtmp[x-1][y]=DOMINOPIECE_FALLING_MEETINGW; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x-1][y]=DOMINOPIECE_FALLING_WEST; break;
							case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x-1][y]=DOMINOPIECE_FALLING_SOUTHW; break;
							case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x-1][y]=DOMINOPIECE_FALLING_NORTHW; break;
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: {
								// Bewegungspfad entfernen
								dominogridtmp[x-1][y]=DOMINOPIECE_FALLEN;
								flipPathLength(x-1,y);
								break;
							}
						}
					}
					break;
				}
				case DOMINOPIECE_FALLING_SOUTH: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					// tunnel entry reached ?
					if (tunneldominogrid[x][y].falling_into_and_beyond==DOMINOPIECE_FALLING_SOUTH) {
						addActiveTunnelAsTimer(x,y+(tunneldominogrid[x][y].length_inaddition_to_stairs+1),tunneldominogrid[x][y].length_inaddition_to_stairs-1,DOMINOPIECE_FALLING_SOUTH);
						break;
					}
						
					if (y<(MAXYMP-1)) {
						switch (dominogrid[x][y+1]) {
							case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_SOUTH: dominogridtmp[x][y+1]=DOMINOPIECE_FALLING_MEETINGS;	break;
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: dominogridtmp[x][y+1]=DOMINOPIECE_FALLING_SOUTH; break;
							case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x][y+1]=DOMINOPIECE_FALLING_SOUTHW; break;
							case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x][y+1]=DOMINOPIECE_FALLING_SOUTHE; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: {
								// Bewegungspfad entfernen
								dominogridtmp[x][y+1]=DOMINOPIECE_FALLEN;
								flipPathLength(x,y+1);
								break;
							}
						}
					}
					break;
				}

				case DOMINOPIECE_FALLING_EAST: {
					changed=1;
					dominogridtmp[x][y]=DOMINOPIECE_FALLEN;
					flipPathLength(x,y);

					if (tunneldominogrid[x][y].falling_into_and_beyond==DOMINOPIECE_FALLING_EAST) {
						addActiveTunnelAsTimer(x+(tunneldominogrid[x][y].length_inaddition_to_stairs+1),y,tunneldominogrid[x][y].length_inaddition_to_stairs-1,DOMINOPIECE_FALLING_EAST);
						break;
					}
						
					if (x<(MAXXMP-1)) {
						switch (dominogrid[x+1][y]) {
							case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_EAST: dominogridtmp[x+1][y]=DOMINOPIECE_FALLING_MEETINGO; break;
							case DOMINOPIECE_UPRIGHT_VERTICAL: dominogridtmp[x+1][y]=DOMINOPIECE_FALLING_EAST; break;
							case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: dominogridtmp[x+1][y]=DOMINOPIECE_FALLING_NORTHE; break;
							case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: dominogridtmp[x+1][y]=DOMINOPIECE_FALLING_SOUTHE; break;
							case DOMINOPIECE_UPRIGHT_HORIZONTAL: {
								// Bewegungspfad entfernen
								dominogridtmp[x+1][y]=DOMINOPIECE_FALLEN;
								flipPathLength(x+1,y);
								break;
							}
						}
					}
					break;
				}
			} // switch
		} // for x,y
		
		if (changed) {
			// copy the temporary domino grid onto
			// the actual one
			for(int x=0;x<MAXXMP;x++) {
				for(int y=0;y<MAXYMP;y++) {
					if (dominogridtmp[x][y] != DOMINOPIECE_EMPTY) {
						dominogrid[x][y]=dominogridtmp[x][y];
					}
				}
			}
			drawdominogrid();
		}
	}
}

void drawdominogrid(const int a,const int b) {
	RECT rect,r2,rges;

	rect.left=LEFTPOS+1;
	rect.right=LEFTPOS+DOMINOPIECESIZE;
	rect.top=TOPPOS+1;
	rect.bottom=TOPPOS+DOMINOPIECESIZE;
	
	int x2,y2;
	POINT poi;
	
	int sx0,sx1,sy0,sy1;
	if (a<0) { 
		sx0=offsetx; 
		sy0=offsety; 
		sx1=sx0+MAXX-1; 
		sy1=sy0+MAXY-1; 
	} else { 
		sx0=sx1=a; 
		sy0=sy1=b; 
	}
	
	x2=LEFTPOS+(sx0-offsetx)*DOMINOPIECESIZE;
	
	int changed=1;
	r2.left=LEFTPOS-4;
	r2.right=r2.left+4;
	r2.top=TOPPOS;
	r2.bottom=r2.top+DOMINOPIECESIZE;
	
	int xm,ym;
	
	for(int x=sx0;x<=sx1;x++) {
		y2=TOPPOS+(sy0-offsety)*DOMINOPIECESIZE;
		rect.left=x2+4;
		rect.right=x2+DOMINOPIECESIZE-4 + 1;
		rges.left=x2+1;
		rges.right=x2+DOMINOPIECESIZE;
		for(int y=sy0;y<=sy1;y++) {
			rect.top=y2+4;
			rect.bottom=y2+DOMINOPIECESIZE-4 + 1;
			rges.top=y2+1;
			rges.bottom=y2+DOMINOPIECESIZE-1;
			xm=(rges.right+rges.left) >> 1;
			ym=(rges.bottom+rges.top) >> 1;
			FillRect(globaldc,&rges,hbrwhite);
			
			switch (dominogrid[x][y]) {
				case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_NORTH: {
					HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
					SetDCPenColor(globaldc, RGB(0,0,0));
					MoveToEx(globaldc,rges.left,ym,&poi);
					LineTo(globaldc,xm,rges.top);
					MoveToEx(globaldc,xm,rges.top,&poi);
					LineTo(globaldc,rges.right,ym);
					MoveToEx(globaldc,rges.left+1,ym,&poi);
					LineTo(globaldc,xm,rges.top+1);
					MoveToEx(globaldc,xm,rges.top+1,&poi);
					LineTo(globaldc,rges.right-1,ym);
					MoveToEx(globaldc,rges.left+2,ym,&poi);
					LineTo(globaldc,xm,rges.top+2);
					MoveToEx(globaldc,xm,rges.top+2,&poi);
					LineTo(globaldc,rges.right-2,ym);
					SelectObject(globaldc,o);  
					break;
				}
				case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_SOUTH: {
					HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
					SetDCPenColor(globaldc, RGB(0,0,0));
					MoveToEx(globaldc,rges.left,ym,&poi);
					LineTo(globaldc,xm,rges.bottom);
					MoveToEx(globaldc,xm,rges.bottom,&poi);
					LineTo(globaldc,rges.right,ym);
					MoveToEx(globaldc,rges.left+1,ym,&poi);
					LineTo(globaldc,xm,rges.bottom-1);
					MoveToEx(globaldc,xm,rges.bottom-1,&poi);
					LineTo(globaldc,rges.right-1,ym);
					MoveToEx(globaldc,rges.left+2,ym,&poi);
					LineTo(globaldc,xm,rges.bottom-2);
					MoveToEx(globaldc,xm,rges.bottom-2,&poi);
					LineTo(globaldc,rges.right-2,ym);
					SelectObject(globaldc,o);  
					break;
				}
				case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_WEST: {
					HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
					SetDCPenColor(globaldc, RGB(0,0,0));
					MoveToEx(globaldc,rges.left,ym,&poi);
					LineTo(globaldc,xm,rges.bottom);
					MoveToEx(globaldc,rges.left,ym,&poi);
					LineTo(globaldc,xm,rges.top);
					MoveToEx(globaldc,rges.left+1,ym,&poi);
					LineTo(globaldc,xm,rges.bottom-1);
					MoveToEx(globaldc,rges.left+1,ym,&poi);
					LineTo(globaldc,xm,rges.top+1);
					MoveToEx(globaldc,rges.left+2,ym,&poi);
					LineTo(globaldc,xm,rges.bottom-2);
					MoveToEx(globaldc,rges.left+2,ym,&poi);
					LineTo(globaldc,xm,rges.top+2);
					break;
				}
				case DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_EAST: {
					HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
					SetDCPenColor(globaldc, RGB(0,0,0));
					MoveToEx(globaldc,rges.right,ym,&poi);
					LineTo(globaldc,xm,rges.bottom);
					MoveToEx(globaldc,rges.right,ym,&poi);
					LineTo(globaldc,xm,rges.top);
					MoveToEx(globaldc,rges.right-1,ym,&poi);
					LineTo(globaldc,xm,rges.bottom-1);
					MoveToEx(globaldc,rges.right-1,ym,&poi);
					LineTo(globaldc,xm,rges.top+1);
					MoveToEx(globaldc,rges.right-2,ym,&poi);
					LineTo(globaldc,xm,rges.bottom-2);
					MoveToEx(globaldc,rges.right-2,ym,&poi);
					LineTo(globaldc,xm,rges.top+2);
					break;
				}
				case DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST: {
					MoveToEx(globaldc,rect.left,rect.top,&poi);
					LineTo(globaldc,rect.right,rect.bottom);
					MoveToEx(globaldc,rect.left+1,rect.top,&poi);
					LineTo(globaldc,rect.right+1,rect.bottom);
					MoveToEx(globaldc,rect.left-1,rect.top,&poi);
					LineTo(globaldc,rect.right-1,rect.bottom);
					MoveToEx(globaldc,rect.left+2,rect.top,&poi);
					LineTo(globaldc,rect.right+2,rect.bottom);
					MoveToEx(globaldc,rect.left-2,rect.top,&poi);
					LineTo(globaldc,rect.right-2,rect.bottom);
					break;
				}
				case DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST: {
					MoveToEx(globaldc,rect.right,rect.top,&poi);
					LineTo(globaldc,rect.left,rect.bottom);
					MoveToEx(globaldc,rect.right+1,rect.top,&poi);
					LineTo(globaldc,rect.left+1,rect.bottom);
					MoveToEx(globaldc,rect.right-1,rect.top,&poi);
					LineTo(globaldc,rect.left-1,rect.bottom);
					MoveToEx(globaldc,rect.right+2,rect.top,&poi);
					LineTo(globaldc,rect.left+2,rect.bottom);
					MoveToEx(globaldc,rect.right-2,rect.top,&poi);
					LineTo(globaldc,rect.left-2,rect.bottom);
					break;
				}
				case DOMINOPIECE_UPRIGHT_HORIZONTAL: {
					r2.left=rect.left-2;
					r2.right=rect.right+2;
					r2.top=rect.top+2;
					r2.bottom=rect.bottom-2;
					FillRect(globaldc,&r2,hbrblack);
					break;
				}
				case DOMINOPIECE_UPRIGHT_VERTICAL: {
					r2.left=rect.left+2;
					r2.right=rect.right-2;
					r2.top=rect.top-2;
					r2.bottom=rect.bottom+2;
					FillRect(globaldc,&r2,hbrblack);
					break;
				}
				case DOMINOPIECE_FALLEN: {
					MoveToEx(globaldc,rect.right,rect.bottom,&poi);
					LineTo(globaldc,rect.left,rect.top);
					MoveToEx(globaldc,rect.left,rect.bottom,&poi);
					LineTo(globaldc,rect.right,rect.top);
					break;
				}
			}; // switch
				
			if (startingpiecedominogrid[x][y] != DOMINOPIECE_EMPTY) {
				// a small red line over any starting piece
				switch (startingpiecedominogrid[x][y]) {
					case DOMINOPIECE_FALLING_NORTHW: {
						HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
						SetDCPenColor(globaldc, RGB(255,0,0));
						MoveToEx(globaldc,rges.right-4,rges.bottom-1,&poi);
						LineTo(globaldc,rges.right-1,rges.bottom-4);
						MoveToEx(globaldc,rges.right-5,rges.bottom-1,&poi);
						LineTo(globaldc,rges.right-1,rges.bottom-5);
						SelectObject(globaldc,o);
						break;
					}
					case DOMINOPIECE_FALLING_SOUTHW: {
						HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
						SetDCPenColor(globaldc, RGB(255,0,0));
						MoveToEx(globaldc,rges.right-4,rges.top+1,&poi);
						LineTo(globaldc,rges.right-1,rges.top+4);
						MoveToEx(globaldc,rges.right-5,rges.top+1,&poi);
						LineTo(globaldc,rges.right-1,rges.top+5);
						SelectObject(globaldc,o);
						break;
					}
					case DOMINOPIECE_FALLING_NORTHE: {
						HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
						SetDCPenColor(globaldc, RGB(255,0,0));
						MoveToEx(globaldc,rges.left+4,rges.bottom-1,&poi);
						LineTo(globaldc,rges.left+1,rges.bottom-4);
						MoveToEx(globaldc,rges.left+5,rges.bottom-1,&poi);
						LineTo(globaldc,rges.left+1,rges.bottom-5);
						SelectObject(globaldc,o);
						break;
					}
					case DOMINOPIECE_FALLING_SOUTHE: {
						HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
						SetDCPenColor(globaldc, RGB(255,0,0));
						MoveToEx(globaldc,rges.left+4,rges.top+1,&poi);
						LineTo(globaldc,rges.left+1,rges.top+4);
						MoveToEx(globaldc,rges.left+5,rges.top+1,&poi);
						LineTo(globaldc,rges.left+1,rges.top+5);
						SelectObject(globaldc,o);
						break;
					}
					case DOMINOPIECE_FALLING_NORTH: {
						r2.left=rges.left+2;
						r2.right=rges.right-1;
						r2.top=rges.bottom-3;
						r2.bottom=rges.bottom-1;
						FillRect(globaldc,&r2,hbrred);
						break;
					}
					case DOMINOPIECE_FALLING_SOUTH: {
						r2.left=rges.left+2;
						r2.right=rges.right-1;
						r2.top=rges.top+3;
						r2.bottom=rges.top+1;
						FillRect(globaldc,&r2,hbrred);
						break;
					}
					case DOMINOPIECE_FALLING_WEST: {
						r2.left=rges.right-3;
						r2.right=rges.right-1;
						r2.top=rges.top+2;
						r2.bottom=rges.bottom-1;
						FillRect(globaldc,&r2,hbrred);
						break;
					}
					case DOMINOPIECE_FALLING_EAST: {
						r2.left=rges.left+1;
						r2.right=rges.left+3;
						r2.top=rges.top+2;
						r2.bottom=rges.bottom-1;
						FillRect(globaldc,&r2,hbrred);
						break;
					}
				} // switch
			}

			y2 += DOMINOPIECESIZE;
		} // for y
		x2 += DOMINOPIECESIZE;
	} // for x
	
	// output enclosement regions
	if (enclosementtotal > 0) {
		RECT r;
		HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
		SetDCPenColor(globaldc, RGB(255,255,255));
		for(int i=0;i<enclosementtotal;i++) {
			if (
					((enclosements[i].lx - offsetx) >= 0) &&
					((enclosements[i].lx - offsetx) < MAXX) &&
					((enclosements[i].ly - offsety) >= 0) &&
					((enclosements[i].ly - offsety) < MAXY) 
			) {
				int xl=LEFTPOS+(enclosements[i].lx-offsetx)*DOMINOPIECESIZE;
				int yl=TOPPOS+(enclosements[i].ly-offsety)*DOMINOPIECESIZE;
				int xr=LEFTPOS+(enclosements[i].rx-offsetx)*DOMINOPIECESIZE;
				int yr=TOPPOS+(enclosements[i].ry-offsety)*DOMINOPIECESIZE;

				int xx=xl;
				for(int x=enclosements[i].lx;x<=enclosements[i].rx;x++) {
					MoveToEx(globaldc,xx,yl,&poi);
					LineTo(globaldc,xx,yr);
					xx += DOMINOPIECESIZE;
				}

				int yy=yl;
				for(int y=enclosements[i].ly;y<=enclosements[i].ry;y++) {
					MoveToEx(globaldc,xl,yy,&poi);
					LineTo(globaldc,xr,yy);
					yy += DOMINOPIECESIZE;
				}
			}
		}

		SetDCPenColor(globaldc, RGB(0,0,255));
		for(int i=0;i<enclosementtotal;i++) {
			if (
					((enclosements[i].lx - offsetx) >= 0) &&
					((enclosements[i].lx - offsetx) < MAXX) &&
					((enclosements[i].ly - offsety) >= 0) &&
					((enclosements[i].ly - offsety) < MAXY) 
			) {
				int xl=LEFTPOS+(enclosements[i].lx-offsetx)*DOMINOPIECESIZE;
				int yl=TOPPOS+(enclosements[i].ly-offsety)*DOMINOPIECESIZE;
				int xr=LEFTPOS+(enclosements[i].rx-offsetx)*DOMINOPIECESIZE;
				int yr=TOPPOS+(enclosements[i].ry-offsety)*DOMINOPIECESIZE;
			
				POINT poi;
				MoveToEx(globaldc,xl,yl,&poi);
				LineTo(globaldc,xr,yl);
				LineTo(globaldc,xr,yr);
				LineTo(globaldc,xl,yr);
				LineTo(globaldc,xl,yl);
				MoveToEx(globaldc,xl-1,yl-1,&poi);
				LineTo(globaldc,xr+1,yl-1);
				LineTo(globaldc,xr+1,yr+1);
				LineTo(globaldc,xl-1,yr+1);
				LineTo(globaldc,xl-1,yl-1);
			}
		}
		SelectObject(globaldc,o);
	}
	
	// draw tunnels
	if (tunneltotal>0) {
		HGDIOBJ o=SelectObject(globaldc,GetStockObject(DC_PEN));
		SetDCPenColor(globaldc, RGB(127,127,127));

		POINT poi;
		for(int i=(tunneltotal-1);i>=0;i--) {
			if (
					((tunnel[i].lx - offsetx) >= 0) &&
					((tunnel[i].lx - offsetx) < MAXX) &&
					((tunnel[i].ly - offsety) >= 0) &&
					((tunnel[i].ly - offsety) < MAXY) 
			) {
				int x0=LEFTPOS+(tunnel[i].lx-offsetx)*DOMINOPIECESIZE + DOMINOPIECESIZEHALF;
				int y0=TOPPOS+(tunnel[i].ly-offsety)*DOMINOPIECESIZE + DOMINOPIECESIZEHALF;
				int x1=LEFTPOS+(tunnel[i].rx-offsetx)*DOMINOPIECESIZE + DOMINOPIECESIZEHALF;
				int y1=TOPPOS+(tunnel[i].ry-offsety)*DOMINOPIECESIZE + DOMINOPIECESIZEHALF;
				MoveToEx(globaldc,x0,y0,&poi);
				LineTo(globaldc,x1,y1);
				if (x0==x1) {
					// vertical tunnel
					MoveToEx(globaldc,x0+2,y0,&poi);
					LineTo(globaldc,x1+2,y1);
				} else {
					// horizontal
					MoveToEx(globaldc,x0,y0+2,&poi);
					LineTo(globaldc,x1,y1+2);
				}
			}
		}
		SelectObject(globaldc,o);  
	} // if
	
	// draw result spots to visualize running time
	if (PathlengthSpotanz>0) {
		RECT r;		
		for(int i=0;i<PathlengthSpotanz;i++) {
			if (
					((resultspot[i].x - offsetx) >= 0) &&
					((resultspot[i].x - offsetx) < MAXX) &&
					((resultspot[i].y - offsety) >= 0) &&
					((resultspot[i].y - offsety) < MAXY) 
			) {
				r.top=TOPPOS+(resultspot[i].y-offsety)*DOMINOPIECESIZE+1;
				r.bottom=r.top+DOMINOPIECESIZE-1;
				r.left=LEFTPOS+(resultspot[i].x-offsetx)*DOMINOPIECESIZE+1;
				r.right=r.left+DOMINOPIECESIZE-1;
				if (resultspot[i].pathlen>0) {
					char tmp[1024];
					sprintf(tmp,"%i ",resultspot[i].pathlen);
					TextOut(globaldc,r.left,r.top-1,tmp,strlen(tmp));
				} else {
					FillRect(globaldc,&r,hbrgreen);
				}
			}
		}
	}
}

void drawdominogrid(void) {
	drawdominogrid(-1,-1);
}

void drawStart(void) {
	// draw the underlying grid nad the starting pieces
	RECT rect;
	rect.left=LEFTPOS;
	rect.right=LEFTPOS+DOMINOPIECESIZE*MAXX;
	rect.top=TOPPOS;
	rect.bottom=TOPPOS+DOMINOPIECESIZE*MAXY;
	FillRect(globaldc,&rect,hbrwhite);

	POINT poi;
	Rectangle(globaldc,LEFTPOS,TOPPOS,LEFTPOS+MAXX*DOMINOPIECESIZE,TOPPOS+MAXY*DOMINOPIECESIZE);
	int bottom=TOPPOS+MAXY*DOMINOPIECESIZE;
	int right=LEFTPOS+MAXX*DOMINOPIECESIZE;
	int x2,y2;
	for(int x=0;x<MAXX;x++) {
		MoveToEx(globaldc,x2=LEFTPOS+x*DOMINOPIECESIZE,TOPPOS,&poi);
		LineTo(globaldc,x2,bottom);
	}
	for(int y=0;y<MAXY;y++) {
		MoveToEx(globaldc,LEFTPOS,y2=TOPPOS+y*DOMINOPIECESIZE,&poi);
		LineTo(globaldc,right,y2);
	}
	
	drawdominogrid();
}

void translateScr_to_ArrayIdx(const int scrx,const int scry,int& fx,int& fy) {
	double d=scrx-LEFTPOS;
	d /= DOMINOPIECESIZE;
	fx=(int)floor(d) + offsetx;
	d=scry-TOPPOS;
	d /= DOMINOPIECESIZE;
	fy=(int)floor(d) + offsety;
}


void saveDominoGrid(const char* fn) {
	FILE *f=fopen(fn,"wb");
	if (!f) return;
	
	int w;
	w=MAXX; fwrite(&w,sizeof(w),1,f);
	w=MAXPAGEX; fwrite(&w,sizeof(w),1,f);
	w=MAXY; fwrite(&w,sizeof(w),1,f);
	w=MAXPAGEY; fwrite(&w,sizeof(w),1,f);

	fwrite(dominogrid,sizeof(char),MAXX*MAXY*MAXPAGEX*MAXPAGEY,f);
	fwrite(startingpiecedominogrid,sizeof(char),MAXX*MAXY*MAXPAGEX*MAXPAGEY,f);
	fwrite(&tunneltotal,sizeof(tunneltotal),1,f);
	fwrite(tunneldominogrid,sizeof(Tunnel),MAXX*MAXY*MAXPAGEX*MAXPAGEY,f);
	fwrite(tunnel,sizeof(Enclosement),tunneltotal,f);
	fwrite(&PathlengthSpotanz,sizeof(PathlengthSpotanz),1,f);
	fwrite(resultspot,sizeof(PathlengthSpot),PathlengthSpotanz,f);
	fwrite(&enclosementtotal,sizeof(enclosementtotal),1,f);
	fwrite(enclosements,sizeof(Enclosement),enclosementtotal,f);
	fclose(f);
}

// setting domino pieces by char* to facilitate
// gate definitions
void setLine(const int ax,const int ay,const char* s) {
	const int len=strlen(s);
	for(int i=0;i<len;i++) {
		if (s[i] != ' ') {
			switch (s[i]) {
				case '-': dominogrid[ax+i][ay]=DOMINOPIECE_UPRIGHT_HORIZONTAL; break;
				case '|': dominogrid[ax+i][ay]=DOMINOPIECE_UPRIGHT_VERTICAL; break;
				case '\\': dominogrid[ax+i][ay]=DOMINOPIECE_UPRICHT_NORTHWEST_SOUTHEAST; break;
				case '/': dominogrid[ax+i][ay]=DOMINOPIECE_UPRIGHT_NORTHEAST_SOUTHWEST; break;
				case 'Y': dominogrid[ax+i][ay]=DOMINOPIECE_UPRIGHT_DOUBLEPIECE_TOGETHER_AT_SOUTH; break;
				default: MessageBox(NULL,"Fehler setLine",MB_OK,NULL); break;
			} // switch
		}
	} // for
}

void addgateEnclosement(const int alx,const int aly,const int arx,const int ary) {
	if (enclosementtotal > (MAXEnclosement-8)) return;
	
	enclosements[enclosementtotal].lx=alx;
	enclosements[enclosementtotal].ly=aly;
	enclosements[enclosementtotal].rx=arx;
	enclosements[enclosementtotal].ry=ary;
	
	enclosementtotal++;
}

int searchTunnel(const int x0,const int y0,const int x1,const int y1) {
	int minx=min(x0,x1);
	int maxx=max(x0,x1);
	int miny=min(y0,y1);
	int maxy=max(y0,y1);
	
	for(int i=0;i<tunneltotal;i++) {
		if (
			(tunnel[i].lx==minx) && (tunnel[i].rx==maxx) &&
			(tunnel[i].ly==miny) && (tunnel[i].ry==maxy)
		) return i;
	}
	
	return -1;
}

int searchPathlengthSpot(const int ax,const int ay) {
	for(int i=0;i<PathlengthSpotanz;i++) {
		if (
			(resultspot[i].x==ax) &&
			(resultspot[i].y==ay) 
		) return i;
	}
	
	return -1;
}

void copyEraseTunnel(const int x0,const int y0,const int x1,const int y1) {
	if (tunneltotal > (MAXTUNNEL-8)) return;
	
	// every tunnel must either be vertical or horizontal
	if ((x0 != x1) && (y0 != y1)) return; 
	
	// if a tunnel between those point exist
	// delete it
	int idx=searchTunnel(x0,y0,x1,y1);
	
	if (idx >= 0) {
		if (tunneltotal==1) tunneltotal=0;
		else {
			swap(tunnel[idx],tunnel[tunneltotal-1]);
			tunneltotal--;
		}
	} else {
		tunnel[tunneltotal].lx=min(x0,x1);
		tunnel[tunneltotal].ly=min(y0,y1);
		tunnel[tunneltotal].rx=max(x0,x1);
		tunnel[tunneltotal].ry=max(y0,y1);
		tunneltotal++;
	
		if (x0==x1) {
			// vertical
			tunneldominogrid[x0][y0].length_inaddition_to_stairs = tunneldominogrid[x1][y1].length_inaddition_to_stairs = max(y1-y0,y0-y1) - 1;
			if (y0 < y1) {
				tunneldominogrid[x0][y0].falling_into_and_beyond=DOMINOPIECE_FALLING_SOUTH;
				tunneldominogrid[x1][y1].falling_into_and_beyond=DOMINOPIECE_FALLING_NORTH;
			} else {
				tunneldominogrid[x1][y0].falling_into_and_beyond=DOMINOPIECE_FALLING_NORTH;
				tunneldominogrid[x0][y1].falling_into_and_beyond=DOMINOPIECE_FALLING_SOUTH;
			}	
		} else if (y0==y1) {
			// horizontal
			tunneldominogrid[x0][y0].length_inaddition_to_stairs = tunneldominogrid[x1][y1].length_inaddition_to_stairs = max(x1-x0,x0-x1) - 1;
			if (x0 < x1) {
				tunneldominogrid[x0][y0].falling_into_and_beyond=DOMINOPIECE_FALLING_EAST;
				tunneldominogrid[x1][y1].falling_into_and_beyond=DOMINOPIECE_FALLING_WEST;
			} else {
				tunneldominogrid[x0][y1].falling_into_and_beyond=DOMINOPIECE_FALLING_WEST;
				tunneldominogrid[x1][y0].falling_into_and_beyond=DOMINOPIECE_FALLING_EAST;
			}
		}
	} 
}

void copyEraseResultSpot(const int ax,const int ay) {
	if (PathlengthSpotanz > (MAXPATHLENGTHSPOT-8)) return;
	
	int idx=searchPathlengthSpot(ax,ay);
	
	if (idx >= 0) {
		// if spot is already marked => delete mark
		if (PathlengthSpotanz==1) PathlengthSpotanz=0;
		else {
			swap(resultspot[idx],resultspot[PathlengthSpotanz-1]);
			PathlengthSpotanz--;
		}
	} else {
		resultspot[PathlengthSpotanz].x=ax;
		resultspot[PathlengthSpotanz].y=ay;
		resultspot[PathlengthSpotanz].pathlen=0;
		PathlengthSpotanz++;
	} 
}

void copygateNOT(const int ax,const int ay) {
	// input: constant 1 / in bit / constant 1
	// output: constant 1 / result but / constant 1
	
	// not enough space
	if (
		((ay+4) >= MAXYMP) || ((ax+15) >= MAXXMP)
	) return;
	
	// empty gird places
	erase(ax,ay,ax+15,ay+4); 
	
	// NOT-gate
	setLine(ax,ay  ,"    - -   -");
	setLine(ax,ay+1,"    Y -   /");
	setLine(ax,ay+2," \\|| ||/   ||/");
	setLine(ax,ay+3,"-       -     -");
	setLine(ax,ay+4,"-       -     -");

	// set constant 1 as starting spot
	startingpiecedominogrid[ax+4][ay]=DOMINOPIECE_FALLING_SOUTH;
	startingpiecedominogrid[ax+10][ay]=DOMINOPIECE_FALLING_SOUTH;
	
	addgateEnclosement(ax,ay+1,ax+15,ay+4);
}

void copygateAND(const int ax,const int ay) {
	// input: constant 1 / in bit #1 / in bit #2 / constant 1
	// output: constant 1 / result bit / constant 1

	if ( 
		((ay+22) >= MAXYMP) ||	((ax+28) >= MAXXMP)
	) return;

	erase(ax,ay,ax+27,ay+22); 

	setLine(ax,ay  ," -    -          -       - ");
	setLine(ax,ay+1," -    -          -       - ");
	setLine(ax,ay+2," -    -          -       - ");
	setLine(ax,ay+3," Y    -          -       - ");
	setLine(ax,ay+4,"- ||||||/        -       - ");
	copyEraseTunnel(ax+6,ay+3,ax+6,ay+5);
	setLine(ax,ay+5,"-     -  -       -       - ");
	setLine(ax,ay+6,"-     -  -       -       Y ");
	setLine(ax,ay+7,"-     -  Y       -      \\ -");
	setLine(ax,ay+8,"-    \\||| ||||/  -     \\  -");
	setLine(ax,ay+9,"-   -          - -    \\   -");
	setLine(ax,ay+10,"-   /          - -   \\    -");
	setLine(ax,ay+11,"-    /         - -  \\     -");
	setLine(ax,ay+12,"-     /        -|||| -    -");
	setLine(ax,ay+13,"-   |/ /       \\     \\|   -");
	setLine(ax,ay+14,"-  \\  - /    \\|     -  /  -");
	setLine(ax,ay+15,"/  -  -  /  -       -  -  -");
	setLine(ax,ay+16," |\\   -   / /       -  -  \\");
	setLine(ax,ay+17,"      \\    / |/     /   /| ");
	setLine(ax,ay+18," \\||||      /  -     / -   ");
	setLine(ax,ay+19,"-            / -      /    ");
	setLine(ax,ay+20,"-             |-       ||/ ");
	setLine(ax,ay+21,"-              -          -");
	setLine(ax,ay+22,"-              -          -");
	// mark result as spot to display running time
	copyEraseResultSpot(ax,ay+22);
	copyEraseResultSpot(ax+15,ay+22);
	copyEraseResultSpot(ax+26,ay+22);

	startingpiecedominogrid[ax+1][ay]=DOMINOPIECE_FALLING_SOUTH;
	startingpiecedominogrid[ax+25][ay]=DOMINOPIECE_FALLING_SOUTH;
	
	addgateEnclosement(ax,ay+1,ax+27,ay+22);
}

void copygateXOR(const int ax,const int ay) {
	// input: constnat 1 / in bit #1 / in bit #2 / constant 1
	// output: constant 1 / result bit / constant 1

	if (
		((ay+15) >= MAXYMP) || ((ax+25) >= MAXXMP)
	) return;

	erase(ax,ay,ax+25,ay+15); 

	setLine(ax,ay   ,"-      -         -      -");
	setLine(ax,ay+1 ,"-      -         -      -");
	setLine(ax,ay+2 ,"/      Y         Y      \\");
	setLine(ax,ay+3 ," ||/  - |||/ \\||| -  \\|| ");
	setLine(ax,ay+4 ,"    - -     -     - -    ");
	setLine(ax,ay+5 ,"    \\ -     Y     - /    ");
	setLine(ax,ay+6 ," \\||  -   \\| |/   -  ||/ ");
	setLine(ax,ay+7 ,"-     -  \\     /  -     -");
	setLine(ax,ay+8 ,"/     - -       - -     \\");
	setLine(ax,ay+9 ," |/   / -       - \\   \\| ");
	setLine(ax,ay+10,"   -   |-       -|   -   ");
	setLine(ax,ay+11,"   \\    -       -    /   ");
	setLine(ax,ay+12," \\|     /       \\     |/ ");
	setLine(ax,ay+13,"-        ||/ \\||        -");
	setLine(ax,ay+14,"-           -           -");
	setLine(ax,ay+15,"-           -           -");

	startingpiecedominogrid[ax][ay]=DOMINOPIECE_FALLING_SOUTH;
	startingpiecedominogrid[ax+24][ay]=DOMINOPIECE_FALLING_SOUTH;
	
	addgateEnclosement(ax,ay+1,ax+25,ay+15);
}

void copygateODER(const int ax,const int ay) {
	// input: constant 1 / in bit #1 / in bit #2 / constant 1
	// output: constant 1 / result bit / constant 1
	
	if (
		((ay+2) >= MAXYMP) || ((ax+9) >= MAXXMP)
	) return;

	erase(ax,ay,ax+9,ay+2); 

	setLine(ax,ay  ,"-  - -  -");
	setLine(ax,ay+1,"-  / \\  -");
	setLine(ax,ay+2,"-   -   -");

	startingpiecedominogrid[ax][ay]=DOMINOPIECE_FALLING_SOUTH;
	startingpiecedominogrid[ax+8][ay]=DOMINOPIECE_FALLING_SOUTH;
	
	addgateEnclosement(ax,ay+1,ax+9,ay+2);
}

void copygateNAND(const int ax,const int ay) {
	// input: constant 1 / in bit #1 / in bit #2 / constant 1
	// output: constant 1 / result bit / constant 1
	
	if (
		((ay+4) >= MAXYMP) || ((ax+17) >= MAXXMP)
	) return;

	erase(ax,ay,ax+17,ay+4); 

	setLine(ax,ay  ,"    - -   - -");
	setLine(ax,ay+1,"    Y -   - Y");
	setLine(ax,ay+2," \\|| ||/ \\|| ||/");
	setLine(ax,ay+3,"-       -       -");
	setLine(ax,ay+4,"-       -       -");

	startingpiecedominogrid[ax+4][ay]=DOMINOPIECE_FALLING_SOUTH;
	startingpiecedominogrid[ax+12][ay]=DOMINOPIECE_FALLING_SOUTH;
	
	addgateEnclosement(ax,ay+1,ax+17,ay+4);
}

// move mouse marked rectangle
void move(const int lx,const int ly,const int rx,const int ry,const int dx,const int dy) {
	int x0=min(lx,rx);
	int x1=max(lx,rx);
	int y0=min(ly,ry);
	int y1=max(ly,ry);
	
	// move the domino pieces
	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		dominogridtmp[x][y]=dominogrid[x][y];
		dominogrid[x][y]=DOMINOPIECE_EMPTY;
	}

	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		dominogrid[x+dx][y+dy]=dominogridtmp[x][y];
		dominogridtmp[x][y]=DOMINOPIECE_EMPTY;
	}
	
	// move the starting grid positions
	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		dominogridtmp[x][y]=startingpiecedominogrid[x][y];
		startingpiecedominogrid[x][y]=DOMINOPIECE_EMPTY;
	}

	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		startingpiecedominogrid[x+dx][y+dy]=dominogridtmp[x][y];
		dominogridtmp[x][y]=DOMINOPIECE_EMPTY;
	}
	
	// move any tunnel in the grid
	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		tunneldominogridtmp[x][y].falling_into_and_beyond=tunneldominogrid[x][y].falling_into_and_beyond;
		tunneldominogridtmp[x][y].length_inaddition_to_stairs=tunneldominogrid[x][y].length_inaddition_to_stairs;
	}

	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		tunneldominogrid[x+dx][y+dy].falling_into_and_beyond=tunneldominogridtmp[x][y].falling_into_and_beyond;
		tunneldominogrid[x+dx][y+dy].length_inaddition_to_stairs=tunneldominogridtmp[x][y].length_inaddition_to_stairs;
	}
	
	// adjust tunnel coordinates
	for(int i=0;i<tunneltotal;i++) {
		if ( 
				(tunnel[i].lx >= lx) &&
				(tunnel[i].rx <= rx) &&
				(tunnel[i].ly >= ly) &&
				(tunnel[i].ry <= ry)
		) {
			tunnel[i].lx += dx;
			tunnel[i].rx += dx;
			tunnel[i].ly += dy;
			tunnel[i].ry += dy;
		}
	}
	
	// move enclosements
	for(int i=0;i<enclosementtotal;i++) {
		if ( 
				(enclosements[i].lx >= lx) &&
				(enclosements[i].rx <= rx) &&
				(enclosements[i].ly >= ly) &&
				(enclosements[i].ry <= ry)
		) {
			enclosements[i].lx += dx;
			enclosements[i].rx += dx;
			enclosements[i].ly += dy;
			enclosements[i].ry += dy;
		}
	}
	
	// move result spots
	for(int i=0;i<PathlengthSpotanz;i++) {
		if ( 
				(resultspot[i].x >= lx) &&
				(resultspot[i].x <= rx) &&
				(resultspot[i].y >= ly) &&
				(resultspot[i].y <= ry)
		) {
			resultspot[i].x += dx;
			resultspot[i].y += dy;
		}
	}
}

void loadDominoGrid(const char* fn) {
	FILE *f=fopen(fn,"rb");
	if (!f) return;
	
	int rmaxx,rmaxy,rmaxpagex,rmaxpagey;
	fread(&rmaxx,sizeof(rmaxx),1,f);
	fread(&rmaxpagex,sizeof(rmaxpagex),1,f);
	fread(&rmaxy,sizeof(rmaxy),1,f);
	fread(&rmaxpagey,sizeof(rmaxpagey),1,f);
	
	// requirement:
	// rmaxx <= MAXX, rmaxy <= MAXY
	// rmaxpagex <= MAXPAGEX, rmaxpagey <= MAXPAGEY
	if ( !(
		(rmaxx <= MAXX) &&
		(rmaxy <= MAXY) &&
		(rmaxpagex <= MAXPAGEX) &&
		(rmaxpagey <= MAXPAGEY) 
	)) {
		MessageBox(NULL,"Error. Cannot load %s due to incompatibility. Behavior is undefined.",MB_OK,NULL);
		return;
	}
	
	// read dominogrid in column by column and - if read row
	// has fewer entries than column in memory here,
	// fill up with DOMINOPIECE_EMPTY
	for(int x=0;x<(rmaxx*rmaxpagex);x++) {
		fread(&dominogrid[x][0],rmaxy*rmaxpagey,sizeof(char),f);
		for(int y=(rmaxy*rmaxpagey);y<(MAXY*MAXPAGEY);y++) dominogrid[x][y]=DOMINOPIECE_EMPTY;
	} // x
	// same with startingpiecedominogrid
	for(int x=0;x<(rmaxx*rmaxpagex);x++) {
		fread(&startingpiecedominogrid[x][0],rmaxy*rmaxpagey,sizeof(char),f);
		for(int y=(rmaxy*rmaxpagey);y<(MAXY*MAXPAGEY);y++) startingpiecedominogrid[x][y]=DOMINOPIECE_EMPTY;
	} // x
	
	fread(&tunneltotal,sizeof(tunneltotal),1,f);
	for(int x=0;x<(rmaxx*rmaxpagex);x++) {
		fread(&tunneldominogrid[x][0],rmaxy*rmaxpagey,sizeof(Tunnel),f);
		for(int y=(rmaxy*rmaxpagey);y<(MAXY*MAXPAGEY);y++) {
			tunneldominogrid[x][y].falling_into_and_beyond=DOMINOPIECE_EMPTY;
		}
	} // x

	fread(tunnel,sizeof(Enclosement),tunneltotal,f);
	fread(&PathlengthSpotanz,sizeof(PathlengthSpotanz),1,f);
	fread(resultspot,sizeof(PathlengthSpot),PathlengthSpotanz,f);
	fread(&enclosementtotal,sizeof(enclosementtotal),1,f);
	fread(enclosements,sizeof(Enclosement),enclosementtotal,f);
	fclose(f);
}

// erase mouse marked rectangle
void erase(const int lx,const int ly,const int rx,const int ry) {
	int x0=min(lx,rx);
	int x1=max(lx,rx);
	int y0=min(ly,ry);
	int y1=max(ly,ry);
	
	// dominogrid, tunnelgrid, startinggrid
	for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) {
		dominogrid[x][y]=DOMINOPIECE_EMPTY;
		startingpiecedominogrid[x][y]=DOMINOPIECE_EMPTY;
		tunneldominogrid[x][y].falling_into_and_beyond=DOMINOPIECE_EMPTY;
	}
	
	// Tunnel
	int idx=0;
	while (idx < tunneltotal) {
		if (
				(tunnel[idx].lx >= lx) &&
				(tunnel[idx].rx <= rx) &&
				(tunnel[idx].ly >= ly) &&
				(tunnel[idx].ry <= ry)
		) {
			// remove tunnel
			if (tunneltotal==1) { tunneltotal=0; break; }
			swap(tunnel[tunneltotal-1],tunnel[idx]);
			tunneltotal--;
		}
		idx++;
	}

	// Enclosement
	idx=0;
	while (idx < enclosementtotal) {
		if (
				(enclosements[idx].lx >= lx) &&
				(enclosements[idx].rx <= rx) &&
				(enclosements[idx].ly >= ly) &&
				(enclosements[idx].ry <= ry)
		) {
			if (enclosementtotal==1) { enclosementtotal=0; break; }
			swap(enclosements[enclosementtotal-1],enclosements[idx]);
			enclosementtotal--;
		}
		idx++;
	}

	// PathlengthSpot
	idx=0;
	while (idx < PathlengthSpotanz) {
		if (
				(resultspot[idx].x >= lx) &&
				(resultspot[idx].x <= rx) &&
				(resultspot[idx].y >= ly) &&
				(resultspot[idx].y <= ry)
		) {
			if (PathlengthSpotanz==1) { PathlengthSpotanz=0; break; }
			swap(resultspot[PathlengthSpotanz-1],resultspot[idx]);
			PathlengthSpotanz--;
		}
		idx++;
	}
}

LRESULT CALLBACK wndproc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char tmp[1024];
	switch(Message) {
		
		case WM_KEYDOWN: {
			switch (wParam) {
				case 'O': 
					// set an entire OR-gate at the
					// beforehand mouseclicked grid positions
					if (GetKeyState(VK_CONTROL)) gate=GATE_OR;
					break;
				case 'B': 
					// set NAND-gate
					if (GetKeyState(VK_CONTROL)) gate=GATE_NAND;
					break;
				case 'A': 
					// set AND-gate
					if (GetKeyState(VK_CONTROL)) gate=GATE_AND;
					break;
				case 'X': 
					// set XOR-gate
					if (GetKeyState(VK_CONTROL)) gate=GATE_XOR;
					break;
				case 'N': 
					// set NOT-gate
					if (GetKeyState(VK_CONTROL)) gate=GATE_NOT;
					break;
				case 'T': 
					// set or delete a tunnel
					// start and end points will be set afterwards
					if (GetKeyState(VK_CONTROL)) gate=GATE_TUNNEL2;
					break;
				case 'M': 
					// move a rectangle, marked beforehand by
					// two mouse clicks one grid square in
					// the following arrow keys indiciated direction
					if (GetKeyState(VK_CONTROL)) gate=GATE_MOVE2;
					break;
				case 'E': 
					// set or delete a result spot
					if (GetKeyState(VK_CONTROL)) gate=GATE_PATHLENSPOT;
					break;
				case VK_DELETE: 
					// delete one grid position marked by a
					// afterwards mouse click
					gate=GATE_DEL2; 
					break;
				case VK_F9: startDomino(); break;
				case VK_F5: drawStart(); drawdominogrid(); break;
				case VK_F2: saveDominoGrid("domino.feld"); break;
				case VK_F3: 
					loadDominoGrid("domino.feld"); 
					drawStart(); 
					break;
				case VK_LEFT: 
					if (gate==GATE_MOVE0) {
						// move marked rectangle
						gate=GATE_NONE;
						move(movex0,movey0,movex1,movey1,-1,0); 
					} else {
						// change to next desktop tile
						if (offsetx > 0) offsetx -= MAXX; else offsetx=(MAXPAGEX-1)*MAXX;
					}
					drawStart();
					drawdominogrid();
					break;
				case VK_RIGHT: 
					if (gate==GATE_MOVE0) {
						gate=GATE_NONE;
						move(movex0,movey0,movex1,movey1,+1,0); 
					} else {
						if (offsetx < ((MAXPAGEX-1)*MAXX)) offsetx += MAXX; else offsetx=0;
					}
					drawStart();
					drawdominogrid();
					break;
				case VK_UP: 
					if (gate==GATE_MOVE0) {
						gate=GATE_NONE;
						move(movex0,movey0,movex1,movey1,0,-1); 
					} else {
						if (offsety > 0) offsety -= MAXY; else offsety=(MAXPAGEY-1)*MAXY;
					}
					drawStart();
					drawdominogrid();
					break;
				case VK_DOWN: 
					if (gate==GATE_MOVE0) {
						gate=GATE_NONE;
						move(movex0,movey0,movex1,movey1,0,+1); 
					} else {
						if (offsety < ((MAXPAGEY-1)*MAXY)) offsety += MAXY; else offsety=0;
					}
					drawStart();
					drawdominogrid();
					break;
			}
			
        	break;
        }
        
		case WM_LBUTTONDOWN: {
			// mouse click sets coordinates into a
			// buffer. translated into array indices
			// some commands need two marked points
			// hence _DEL1 and _DEL2 constants
			POINT poi;
			GetCursorPos(&poi);
			ScreenToClient(globalwnd,&poi);
			char t[1024];
			int fx,fy;
			translateScr_to_ArrayIdx(poi.x,poi.y,fx,fy);
			
			if (gate != GATE_NONE) {
				switch (gate) {
					case GATE_OR: copygateODER(fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_NOT: copygateNOT(fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_NAND: copygateNAND(fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_AND: copygateAND(fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_XOR: copygateXOR(fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_TUNNEL2: tunnelx0=fx; tunnely0=fy; gate=GATE_TUNNEL1; break;
					case GATE_TUNNEL1: copyEraseTunnel(tunnelx0,tunnely0,fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_PATHLENSPOT: copyEraseResultSpot(fx,fy); drawdominogrid(); gate=GATE_NONE; break;
					case GATE_MOVE2: movex0=fx; movey0=fy; gate=GATE_MOVE1; break;
					case GATE_MOVE1: movex1=fx; movey1=fy; gate=GATE_MOVE0; break;
					case GATE_DEL2: delx0=fx; dely0=fy; gate=GATE_DEL1; break;
					case GATE_DEL1: erase(delx0,dely0,fx,fy); drawStart(); drawdominogrid(); gate=GATE_NONE; break;
				}
								
				break;
			}

			if ((fx>=offsetx)&&(fx<(offsetx+MAXX))&&(fy>=offsety)&&(fy<(offsety+MAXY))) {
				// in the current desktop tile visible

				// rotate the current domino grid content
				if (dominogrid[fx][fy]==DOMINOPIECE_EMPTY) {
					dominogrid[fx][fy]=DOMINOPIECE_UPRIGHT_MINVALUE;
					drawdominogrid(fx,fy);
				} else
				if ( (dominogrid[fx][fy]>=DOMINOPIECE_UPRIGHT_MINVALUE)&& (dominogrid[fx][fy] < DOMINOPIECE_UPRIGHT_MAXVALUE)) {
					dominogrid[fx][fy]++;
					drawdominogrid(fx,fy);
				} else
				if (dominogrid[fx][fy]==DOMINOPIECE_UPRIGHT_MAXVALUE) {
					dominogrid[fx][fy]=DOMINOPIECE_EMPTY;
					drawdominogrid(fx,fy);
				}
			}
			
			break;
		}
		
		case WM_RBUTTONDOWN: {
			POINT poi;
			GetCursorPos(&poi);
			ScreenToClient(globalwnd,&poi);
			int fx,fy;
			translateScr_to_ArrayIdx(poi.x,poi.y,fx,fy);
			if ((fx>=offsetx)&&(fx<(offsetx+MAXX))&&(fy>=offsety)&&(fy<(offsety+MAXY))) {
				// change the startinggrid entry
				if (startingpiecedominogrid[fx][fy]==DOMINOPIECE_EMPTY) {
					startingpiecedominogrid[fx][fy]=DOMINOPIECE_FALLING_MINVALUE;
					drawdominogrid(fx,fy);
				} else
				if ( (startingpiecedominogrid[fx][fy]>=DOMINOPIECE_FALLING_MINVALUE)&& (startingpiecedominogrid[fx][fy] < DOMINOPIECE_FALLING_MAXVALUE)) {
					startingpiecedominogrid[fx][fy]++;
					drawdominogrid(fx,fy);
				} else
				if (startingpiecedominogrid[fx][fy]==DOMINOPIECE_FALLING_MAXVALUE) {
					startingpiecedominogrid[fx][fy]=DOMINOPIECE_EMPTY;
					drawdominogrid(fx,fy);
				}
			}
			
			return 0;
		}

		case WM_CLOSE: {
			DestroyWindow(hwnd);
			break;
		}
		
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE instance,HINSTANCE p,LPSTR arg,int cmd) {
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wincl;

    wincl.hInstance=instance;
    wincl.lpszClassName="dominoclass";
    wincl.lpfnWndProc=wndproc;
    wincl.style=CS_DBLCLKS;
    wincl.cbSize=sizeof(WNDCLASSEX);

    wincl.hIcon=LoadIcon(NULL,IDI_APPLICATION);
    wincl.hIconSm=LoadIcon(NULL,IDI_APPLICATION);
    wincl.hCursor=LoadCursor(NULL,IDC_ARROW);
    wincl.lpszMenuName=NULL;
    wincl.cbClsExtra=0;
    wincl.cbWndExtra=0;
    wincl.hbrBackground=(HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx(&wincl)) return 0;

	globalwnd=hwnd=CreateWindowEx (
		0,"dominoclass","Domino",
		WS_OVERLAPPEDWINDOW,
        0,0,       
		32+LEFTPOS+MAXX*DOMINOPIECESIZE+32,
		32+TOPPOS+MAXY*DOMINOPIECESIZE+32,
        HWND_DESKTOP,NULL,instance,NULL                 
    );
		
    ShowWindow(hwnd,cmd);

	offsetx=offsety=0;
	
	hbrred=CreateSolidBrush(RGB(255,0,0));
	hbrgreen=CreateSolidBrush(RGB(0,210,0));
	hbrwhite=(HBRUSH)GetStockObject(WHITE_BRUSH);
	hbrgray=(HBRUSH)GetStockObject(LTGRAY_BRUSH);
	hbrblack=(HBRUSH)GetStockObject(BLACK_BRUSH);
	enclosementtotal=0;
	PathlengthSpotanz=0;

	globaldc=GetDC(globalwnd);
	gate=GATE_NONE;
	
	initdominogrid();
	initTunnel();
	drawStart();

    while (GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	ReleaseDC(globalwnd,globaldc);

    return msg.wParam;
}

