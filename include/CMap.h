/////////////////////////////////////////
//
//                  OpenLieroX
//
// code under LGPL, based on JasonBs work,
// enhanced by Dark Charlie and Albert Zeyer
//
//
/////////////////////////////////////////


// Map class
// Created 21/1/02
// Jason Boettcher


#ifndef __CMAP_H__
#define __CMAP_H__

#include <SDL.h>
#include <string>
#include "ReadWriteLock.h"
#include "SmartPointer.h"
#include "LieroX.h" // for maprandom_t

class CViewport;



#define		MAP_VERSION	0

// Map types
#define		MPT_PIXMAP	0
#define		MPT_IMAGE	1

// Pixel flags
#define		PX_EMPTY	0x01
#define		PX_DIRT		0x02
#define		PX_ROCK		0x04
#define		PX_SHADOW	0x08
#define		PX_WORM		0x10

// Object types
#define		OBJ_HOLE	0
#define		OBJ_STONE	1
#define		OBJ_MISC	2

#define		MAX_OBJECTS	8192

// Antialiasing blur
#define		MINIMAP_BLUR	10.0f

// Shadow drop
#define		SHADOW_DROP	3



class CWorm;



// Theme structure
class theme_t { public:
	std::string	name;
	Uint32		iDefaultColour;
	SmartPointer<SDL_Surface> bmpFronttile;
	SmartPointer<SDL_Surface> bmpBacktile;

	int			NumStones;
	SmartPointer<SDL_Surface> bmpStones[16];
	SmartPointer<SDL_Surface> bmpHoles[16];
	int			NumMisc;
	SmartPointer<SDL_Surface> bmpMisc[32];

};


class CPlayer;

class CMap {
private:
	// just don't do that
	CMap(const CMap&) { assert(false); }
	CMap& operator=(const CMap&) { assert(false); return *this; }
	
public:
	// Constructor
	CMap() {
		Width = 800;
		Height = 700;
		MinimapWidth = 128;
		MinimapHeight = 96;
		Type = MPT_PIXMAP;
        nTotalDirtCount = 0;

		Created = false;
		FileName = "";

		bmpImage = NULL;
#ifdef _AI_DEBUG
		bmpDebugImage = NULL;
#endif
		bmpBackImage = NULL;
		bmpMiniMap = NULL;
		PixelFlags = NULL;
        bmpGreenMask = NULL;
        bmpShadowMap = NULL;
        GridFlags = NULL;
		AbsoluteGridFlags = NULL;
		
		NumObjects = 0;
		Objects = NULL;

		bMiniMapDirty = true;
        sRandomLayout.bUsed = false;

		FlagSpawnX = -1;
		FlagSpawnY = -1;
		BaseStartX = -1;
		BaseStartY = -1;
		BaseEndX   = -1;
		BaseEndY   = -1;
		
		bMapSavingToMemory = false;
		bmpSavedImage = NULL;
		savedPixelFlags = NULL;
		savedMapCoords.clear();
   	}

	~CMap() {
		Shutdown();
	}

private:
	// Attributes

	std::string	Name;
	std::string FileName;
	int			Type;
	uint		Width;
	uint		Height;
	uint		MinimapWidth;
	uint		MinimapHeight;
	theme_t		Theme;
    uint         nTotalDirtCount;

	bool		Created;
	
	SmartPointer<SDL_Surface> bmpImage;
	SmartPointer<SDL_Surface> bmpDrawImage;
	SmartPointer<SDL_Surface> bmpBackImage;    
	SmartPointer<SDL_Surface> bmpMiniMap;
    SmartPointer<SDL_Surface> bmpGreenMask;
	uchar		*PixelFlags;  
    SmartPointer<SDL_Surface> bmpShadowMap;
#ifdef _AI_DEBUG
	SmartPointer<SDL_Surface> bmpDebugImage;
#endif

    // AI Grid
    int         nGridWidth, nGridHeight;
    int         nGridCols, nGridRows;
	uchar		*GridFlags;
	uchar		*AbsoluteGridFlags;

    maprandom_t sRandomLayout;

	bool		bMiniMapDirty;

	ReadWriteLock	flagsLock;

	// Objects
	int			NumObjects;
	object_t	*Objects;

	// CTF
	short		FlagSpawnX;
	short		FlagSpawnY;
	short		BaseStartX;
	short		BaseStartY;
	short		BaseEndX;
	short		BaseEndY;

	// Save/restore from memory, for commit/rollback net mechanism
	bool		bMapSavingToMemory;
	SmartPointer<SDL_Surface> bmpSavedImage;
	uchar *		savedPixelFlags;
	enum { MAP_SAVE_CHUNK = 16 };
	struct SavedMapCoord_t {
		SavedMapCoord_t( int _X=0, int _Y=0 ): X(_X), Y(_Y) {};
		int X;
		int Y;
		bool operator < (const SavedMapCoord_t & m) const
		{
			if( Y < m.Y ) return true;
			else if( Y > m.Y ) return false;
			else if( X < m.X ) return true;
			else return false;
		};
	};
	std::set< SavedMapCoord_t > savedMapCoords;

private:
	// Update functions
	void		UpdateMiniMap(bool force = false);
	void		UpdateMiniMapRect(int x, int y, int w, int h);
	void		UpdateArea(int x, int y, int w, int h, bool update_image = false);

	friend void CCache::SaveMap(const std::string& file, CMap *map);

	bool		NewFrom(CMap *map);
	void		SaveToCache();
	bool		LoadFromCache();

public:
	// Methods
	bool		Create(uint _width, uint _height, const std::string& _theme, uint _minimap_w, uint _minimap_h);
	bool		New(uint _width, uint _height, const std::string& _theme, uint _minimap_w = 128, uint _minimap_h = 96);
	bool		Load(const std::string& filename);
	bool		LoadOriginal(FILE *fp);
	bool		Save(const std::string& name, const std::string& filename);
	bool		SaveImageFormat(FILE *fp);
	bool		LoadImageFormat(FILE *fp, bool ctf);	
	void		Clear(void);

    void		ApplyRandom(void);
    void        ApplyRandomLayout(maprandom_t *psRandom);

	void		UpdateDrawImage(int x, int y, int w, int h);

	void		Shutdown(void);

	bool		LoadTheme(const std::string& _theme);
	bool		CreateSurface(void);
	bool		CreatePixelFlags(void);
    bool        createGrid(void);
    void        calculateGrid(void);
private:
	// not thread-safe    
    void        calculateGridCell(int x, int y, bool bSkipEmpty);
public:	
	void		TileMap(void);
    
    void        CalculateDirtCount();
    void        CalculateShadowMap();

	inline void	lockFlags(bool writeAccess = true) {
		if(writeAccess)
			flagsLock.startWriteAccess();
		else
			flagsLock.startReadAccess();
	}
	inline void unlockFlags(bool writeAccess = true) {
		if(writeAccess)
			flagsLock.endWriteAccess();
		else
			flagsLock.endReadAccess(); 
	}

    static std::string findRandomTheme();
    static bool validateTheme(const std::string& name);

    void        PutImagePixel(uint x, uint y, Uint32 colour);

	void		Draw(SDL_Surface *bmpDest, CViewport *view);
	void		Draw(SDL_Surface *bmpDest, const SDL_Rect& rect, int worldX, int worldY);	// For CMapEditor
	
	void        DrawObjectShadow(SDL_Surface * bmpDest, SDL_Surface * bmpObj, int sx, int sy, int w, int h, CViewport *view, int wx, int wy);
	void        DrawPixelShadow(SDL_Surface * bmpDest, CViewport *view, int wx, int wy);
	void		DrawMiniMap(SDL_Surface * bmpDest, uint x, uint y, float dt, CWorm *worms, int gametype);
	
private:
	// not thread-safe, therefore private	
	inline void	SetPixelFlag(uint x, uint y, uchar flag)	
	{
		// Check edges
		if(x >= Width || y >= Height)
			return;
	
		PixelFlags[y * Width + x] = flag;
	}
	
	// Saves region of map to savebuffer for RestoreFromMemory() - called from CarveHole()/PlaceDirt()/PlaceGreenDirt()
	void SaveToMemoryInternal(int x, int y, int w, int h);


public:	

	size_t GetMemorySize();

	inline uchar GetPixelFlag(uint x, uint y)
	{
		// Checking edges
		if(x >= Width || y >= Height)
			return PX_ROCK;
	
		return PixelFlags[y * Width + x];
	}

	uchar	*GetPixelFlags() const	{ return PixelFlags; }

	SmartPointer<SDL_Surface> GetDrawImage()		{ return bmpDrawImage; }
	SmartPointer<SDL_Surface> GetImage()			{ return bmpImage; }
	SmartPointer<SDL_Surface> GetBackImage()		{ return bmpBackImage; }
	SmartPointer<SDL_Surface> GetMiniMap()		{ return bmpMiniMap; }
#ifdef _AI_DEBUG
	// TODO: the debug image is also usefull for other debugging things, not for AI
	// so make it also available if DEBUG is defined
	SmartPointer<SDL_Surface> GetDebugImage()	{ return bmpDebugImage; }

	void		ClearDebugImage();
#endif

	int 		CarveHole(int size, CVec pos);
	int 		PlaceDirt(int size, CVec pos);
	void		PlaceStone(int size, CVec pos);
	void		PlaceMisc(int id, CVec pos);
    int         PlaceGreenDirt(CVec pos);
	void		ApplyShadow(int sx, int sy, int w, int h);

	// Save/restore from memory, for commit/rollback net mechanism
	void		SaveToMemory();
	void		RestoreFromMemory();

	inline theme_t		*GetTheme(void)		{ return &Theme; }

	void		DEBUG_DrawPixelFlags(int x, int y, int w, int h);

    inline maprandom_t *getRandomLayout(void)  { return &sRandomLayout; }


	inline uint			GetWidth(void) const	{ return Width; }
	inline uint			GetHeight(void)	const	{ return Height; }
	inline uint			GetMinimapWidth() const { return MinimapWidth; }
	inline uint			GetMinimapHeight() const { return MinimapHeight; }
	void				SetMinimapDimensions(uint _w, uint _h);
    inline uint         GetDirtCount(void) const { return nTotalDirtCount; }

    inline int         getGridCols(void) const  { return nGridCols; }
    inline int         getGridRows(void) const  { return nGridRows; }
    inline int         getGridWidth(void) const { return nGridWidth; }
    inline int         getGridHeight(void) const { return nGridHeight; }
    inline const uchar *getGridFlags(void) const { 
		return GridFlags; 
	}
	inline const uchar	*getAbsoluteGridFlags() const { return AbsoluteGridFlags; }
	inline bool			getCreated(void)	{ return Created; }
	inline std::string getName(void)		{ return Name; }

	// TODO: why is this converted to CVec? either use directly CVec or don't use it at all! every conversion take performance away
	inline CVec		getFlagSpawn(void)		{ return CVec(FlagSpawnX, FlagSpawnY); }
	inline CVec		getBaseStart(void)		{ return CVec(BaseStartX, BaseStartY); }
	inline CVec		getBaseEnd(void)		{ return CVec(BaseEndX, BaseEndY); }

	// TODO: this needs to be made much more general to be as fast as the current routines
	
	// _F has to be a functor with provides compatible functions to:
	//   bool operator()(int x, int y, int adr_offset); // handle one point; if returns false, break
	template<typename _T, typename _F>
	inline void walkPixels(Rect<_T> r, _F walker) {
		if(!r.clipWith(SDLRect(0, 0, Width, Height)))
			return;
		
		for(int y = r.y(); y < r.y2(); y++)
			for(int x = r.x(); x < r.x2(); x++)
				if(!walker(x, y)) return;
	}
};



// TODO: make this a member function of CMap
/*
	this traces the given line
	_action is a functor, used for the checkflag_action
	it will be called, if we have a flag fitting to checkflag, with (int x, int y) as param; it also will be called on clipping with the edges
	if the returned value is false, the loop will break
*/
template<class _action>
void fastTraceLine(CVec target, CVec start, CMap *pcMap, uchar checkflag, _action& checkflag_action) {
	enum { X_DOM=-1, Y_DOM=1 } dom; // which is dominating?
	CVec dir = target-start;
	if(dir.x == 0 && dir.y == 0)
		return;
		
	float quot;
	int s_x = (dir.x >= 0) ? 1 : -1;
	int s_y = (dir.y >= 0) ? 1 : -1;	
	// ensure, that |quot| <= 1 (we swap the whole map virtuelly for this, this is, what dom saves)
	if(s_x * dir.x >= s_y * dir.y) {
		dom = X_DOM;
		quot = dir.y / dir.x;
	} else {
		dom = Y_DOM;
		quot = dir.x / dir.y;
	}
	
#ifdef _AI_DEBUG
	//SmartPointer<SDL_Surface> bmpDest = pcMap->GetDebugImage();
#endif
	
	const uchar* pxflags = pcMap->GetPixelFlags();
	const uchar* gridflags = pcMap->getAbsoluteGridFlags();
	if (!pxflags || !gridflags)  // map has been probably shut down in the meantime
		return;

	int map_w = pcMap->GetWidth();
	int map_h = pcMap->GetHeight();	
    int grid_w = pcMap->getGridWidth();
    int grid_h = pcMap->getGridHeight();
	int grid_cols = pcMap->getGridCols();
	
	int start_x = (int)start.x;
	int start_y = (int)start.y;
	int last_gridflag_i = -1;
	int gridflag_i;
	int pos_x, pos_y;
	int grid_x, grid_y;
	register int x = 0;
	register int y = 0;
	while(true) {
		if(dom != Y_DOM) { // X_DOM
			y = (int)(quot*(float)x);
		} else { // Y_DOM
			x = (int)(quot*(float)y);
		}
		
		// is all done?
		if(s_x*x > s_x*(int)dir.x
		|| s_y*y > s_y*(int)dir.y) {
			break;
		}
		
		// this is my current pos
		pos_x = start_x + x;
		pos_y = start_y + y;
			
		// clipping?
		if(pos_x < 0 || pos_x >= map_w
		|| pos_y < 0 || pos_y >= map_h) {
			if(!checkflag_action(pos_x, pos_y))
				break;
			continue;
		}
		
#ifdef _AI_DEBUG
		//PutPixel(bmpDest,pos_x*2,pos_y*2,MakeColour(255,255,0));
#endif
			
		// inside the grid
		grid_x = pos_x / grid_w;
		grid_y = pos_y / grid_h;

		// got we some usefull info from our grid?
		gridflag_i = grid_y*grid_cols + grid_x;
		if(last_gridflag_i != gridflag_i) {
			last_gridflag_i = gridflag_i;
			if(!(gridflags[gridflag_i] & checkflag)) {				
				// yes, we did, no checkflag inside
#ifdef _AI_DEBUG
				//DrawRectFill(bmpDest,grid_x*grid_w*2,grid_y*grid_h*2,(grid_x+1)*grid_w*2+4,(grid_y+1)*grid_h*2,MakeColour(150,150,0));	
#endif
				// go behind this grid-cell
				// the following checks works, because |quot| <= 1
				// make some pictures, then you will belive me :)
				if(dom != Y_DOM) { // X_DOM
					if(s_x > 0) {
						if(s_y > 0) {
							if( pos_x != (grid_x+1)*grid_w &&
							(float(pos_y - (grid_y+1)*grid_h))/float(pos_x - (grid_x+1)*grid_w) <= quot )
								x += int(float((grid_y+1)*grid_h - pos_y)/quot) + 1; // down
							else
								x = (grid_x+1)*grid_w - start_x; // right
						} else { // s_y < 0
							if( pos_x != (grid_x+1)*grid_w &&
							(float(pos_y - grid_y*grid_h))/float(pos_x - (grid_x+1)*grid_w) >= quot )
								x += int(float(grid_y*grid_h - pos_y)/quot) + 1; // up
							else
								x = (grid_x+1)*grid_w - start_x; // right
						}
					} else { // s_x < 0
						if(s_y > 0) {
							if( pos_x != grid_x*grid_w &&
							(float(pos_y - (grid_y+1)*grid_h))/float(pos_x - grid_x*grid_w) >= quot )
								x += int(float((grid_y+1)*grid_h - pos_y)/quot) - 1; // down
							else
								x = grid_x*grid_w - start_x - 1; // left
						} else { // s_y < 0
							if( pos_x != grid_x*grid_w &&
							(float(pos_y - grid_y*grid_h))/float(pos_x - grid_x*grid_w) <= quot )
								x += int(float(grid_y*grid_h - pos_y)/quot) - 1; // up
							else
								x = grid_x*grid_w - start_x - 1; // left
						}
					}
				} else { // Y_DOM
					if(s_y > 0) {
						if(s_x > 0) {
							if( pos_y != (grid_y+1)*grid_h &&
							(float(pos_x - (grid_x+1)*grid_w))/float(pos_y - (grid_y+1)*grid_h) <= quot )
								y += int(float((grid_x+1)*grid_w - pos_x)/quot) + 1; // right
							else
								y = (grid_y+1)*grid_h - start_y; // down
						} else { // s_y < 0
							if( pos_y != (grid_y+1)*grid_h &&
							(float(pos_x - grid_x*grid_w))/float(pos_y - (grid_y+1)*grid_h) >= quot )
								y += int(float(grid_x*grid_w - pos_x)/quot) + 1; // left
							else
								y = (grid_y+1)*grid_h - start_y; // down
						}
					} else { // s_y < 0
						if(s_x > 0) {
							if( pos_y != grid_y*grid_h &&
							(float(pos_x - (grid_x+1)*grid_w))/float(pos_y - grid_y*grid_h) >= quot )
								y += int(float((grid_x+1)*grid_w - pos_x)/quot) - 1; // right
							else
								y = grid_y*grid_h - start_y - 1; // up
						} else { // s_y < 0
							if( pos_y != grid_y*grid_h &&
							(float(pos_x - grid_x*grid_w))/float(pos_y - grid_y*grid_h) <= quot )
								y += int(float(grid_x*grid_w - pos_x)/quot) - 1; // left
							else
								y = grid_y*grid_h - start_y - 1; // up
						}
					}
				}	
			}
			continue;		
		} 
		
		// is the checkflag fitting to our current flag?
		if(pxflags[pos_y*map_w + pos_x] & checkflag)
			// do the given action; break if false
			if(!checkflag_action(pos_x, pos_y))
				break;
		
		// go ahead
		if(dom != Y_DOM) { // X_DOM
			x += s_x;
		} else { // Y_DOM
			y += s_y;
		}
	}
}

int		CheckCollision(CVec trg, CVec pos, uchar checkflags, CMap *map);
int 	CarveHole(CMap *cMap, CVec pos);


#endif  //  __CMAP_H__
