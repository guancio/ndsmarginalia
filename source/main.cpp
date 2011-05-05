#include <nds.h>
#include <fat.h>
#include <stdio.h>

#include "page.h"


#define MY_BG_W (512)
#define MY_BG_H (512)

class AppState {
public:
  int lastPage;
  sImage lastImage;
  bool lastImageFromPcx;
  int center_x;
  int center_y;
  int scroll_x;
  int scroll_y;
  Page* currentPage;
  

  Point convertBufferToImage(Point src) {
    return Point(
		 (src.x+center_x-MY_BG_W/2),
		 (src.y+center_y-MY_BG_H/2)
		 );
  }
  Point convertImageToBuffer(Point src) {
    return Point(
		 (src.x-center_x+MY_BG_W/2),
		 (src.y-center_y+MY_BG_H/2)
		 );
  }
  Point convertBufferToScreen(Point src) {
    // 0 -> -scroll_x
    // MY_BG_W -> MY_BG_W - scroll_x
    return Point(
		 src.x - scroll_x,
		 src.y - scroll_y
		 );
  }
  Point convertScreenToBuffer(Point src) {
    return Point(
		 src.x + scroll_x,
		 src.y + scroll_y
		 );
  }
  Point convertScreenToImage(Point src) {
    Point tmp = convertScreenToBuffer(src);
    return convertBufferToImage(tmp);
  }
};


void drawLine(int x1, int y1, int x2, int y2, unsigned short color);


void fillEmptyPage(AppState & state) {
  for (int y=0 ; y < MY_BG_H; y++) {
    int offset = y * MY_BG_W / 2;
    for (int x=0; x<MY_BG_W; x+=2) {
      int offset2 = offset + x/2;
      bgGetGfxPtr(3)[offset2] = 0;
    }
  }
  state.lastImageFromPcx = false;
}

void fillDisplay(unsigned short color, unsigned int page, AppState & state) {
  int offset = 0;
  // for (int y=0; y<MY_BG_H; y++) {
  //   offset = y * MY_BG_W;
  //   for (int x=0; x<MY_BG_W; x++) {
  //     offset+=1;
  //     BG_GFX[offset] = color;
  //   }
  // }
  
  if (page != state.lastPage) {
    if (state.lastPage != -1 && state.lastImageFromPcx) {
      free(state.lastImage.palette);
      // free(state.lastImage.imasge.data8);
      free(state.lastImage.image.data16);
    }
    state.lastPage = page;

    int res = 0;
 
    char fileName[1024];
    sprintf(fileName, "image%02d.pcx", page);

    FILE * pFile = fopen ( fileName , "rb" );
    if (pFile==NULL) {
      printf("File File Not Found\n");
      fillEmptyPage(state);
      return;
    }

    // obtain file size:
    long lSize;
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    char * pcx_image = (char *)malloc (sizeof(char)*lSize);
    if (pcx_image == NULL) {
      printf ("Memory error\n");
      fillEmptyPage(state);
      return;
    }

    // copy the file into the buffer:
    res = fread (pcx_image,1,lSize,pFile);
    if (res != lSize) {
      printf("Reading image error\n");
      fillEmptyPage(state);
      return;
    }

    fclose (pFile);


    res = loadPCX((u8*)pcx_image, &state.lastImage); 
    free (pcx_image);

    printf("Loaded Background of size %d x %d\n", state.lastImage.width, state.lastImage.height);
    printf(" Bits per pixel %d\n", state.lastImage.bpp);
    printf(" res %d\n", res);

    // image8to16(&image);

    printf(" palette size %d\n", sizeof(state.lastImage.palette));

    for (int i=0; i<256; i++) {
      BG_PALETTE[i] = state.lastImage.palette[i];
    }
    BG_PALETTE[1] = RGB15(31,0,0) | BIT(15);
    state.lastImageFromPcx = true;
  }



  // dmaCopy(image.palette, BG_PALETTE, sizeof(image.palette));

  for (int y=0 ; y < MY_BG_H; y++) {
    offset = y * MY_BG_W / 2;
    for (int x=0; x<MY_BG_W; x+=2) {
      int offset2 = offset + x/2;

      Point imgPoint = state.convertBufferToImage(Point(x,y));

      if (imgPoint.y < 0 || imgPoint.y >= state.lastImage.height) {
	bgGetGfxPtr(3)[offset2] = 0;
	continue;
      }
      if (imgPoint.x < 0 || imgPoint.x >= state.lastImage.width) {
	bgGetGfxPtr(3)[offset2] = 0;
	continue;
      }

      //BG_GFX[offset] = RGB15(31,0,0) | BIT(15);
      // BG_GFX[offset] = image.image.data16[imgO];
      int imgO = imgPoint.y * state.lastImage.width/2 + imgPoint.x/2;
      bgGetGfxPtr(3)[offset2] = state.lastImage.image.data16[imgO];
      // bgGetGfxPtr(3)[offset2] = 0x0101;
    }
  }
  // dmaCopy(image.image.data8, bgGetGfxPtr(3), image.width*image.height);
}
void drawPage(Page * page, unsigned short color, AppState & state) {
  for (unsigned int iSeg = 0; iSeg < page->segments.size(); iSeg++) {
    Segment segment = page->segments[iSeg];
    for (unsigned int iPoint=0; iPoint<segment.points.size()-1; iPoint++) {
      Point p1img = segment.points[iPoint];
      Point p2img = segment.points[iPoint+1];
      Point p1buff = state.convertImageToBuffer(p1img);
      Point p2buff = state.convertImageToBuffer(p2img);

      // printf("--------------\n");
      // printf("%d %d\n", p1buff.x, p1buff.y);
      // printf("%d %d\n", p2buff.x, p2buff.y);

      drawLine(p1buff.x, p1buff.y, p2buff.x, p2buff.y, color);
    }
  }
}

void drawLine(int x1, int y1, int x2, int y2, unsigned short color)
{
  color = 1;
    int yStep = MY_BG_W;
    int xStep = 1;      
    int xDiff = x2 - x1;
    int yDiff = y2 - y1;
 
    int errorTerm = 0;
    int offset = y1 * MY_BG_W + x1; 
    int i;

    int dx = 1;
    int dy = 1;

    //need to adjust if y1 > y2
    if (yDiff < 0)       
    {                  
       yDiff = -yDiff;   //absolute value
       yStep = -yStep;   //step up instead of down   
       dy = -1;
    }
    
    //same for x
    if (xDiff < 0) 
    {           
       xDiff = -xDiff;            
       xStep = -xStep;            
       dx = -1;
    }        
 
    //case for changes more in X than in Y	 
    if (xDiff > yDiff) 
    {                            
       for (i = 0; i < xDiff + 1; i++)
       {                           
	 if (x1 >= 0 && x1 < MY_BG_W && y1 >=0 && y1 < MY_BG_H) {
	   // VRAM_A[offset] = color;  
	   // BG_GFX[offset] = color; 
	   int offset2 = offset/2;
	   if (offset%2 == 0) {
	     BG_GFX[offset2] = (BG_GFX[offset2] & 0xff00) | color;
	   }
	   else {
	     BG_GFX[offset2] = (BG_GFX[offset2] & 0x00ff) | (color<<8);
	   }
	 }

          offset += xStep;           
 
          errorTerm += yDiff;     
 
	   x1+=dx;
          if (errorTerm > xDiff) 
          {  
             errorTerm -= xDiff;     
             offset += yStep;
	     y1 += dy;
          }
       }
    }//end if xdiff > ydiff
    //case for changes more in Y than in X
    else 
    {                       
       for (i = 0; i < yDiff + 1; i++) 
       {  
          // VRAM_A[offset] = color;  
	 // BG_GFX[offset] = color; 
	 if (x1 >= 0 && x1 < MY_BG_W && y1 >=0 && y1 < MY_BG_H) {
	     int offset2 = offset/2;
	     if (offset%2 == 0) {
	       BG_GFX[offset2] = (BG_GFX[offset2] & 0xff00) | color;
	     }
	     else {
	       BG_GFX[offset2] = (BG_GFX[offset2] & 0x00ff) | (color<<8);
	     }
	   }
 
          offset += yStep;           
 
          errorTerm += xDiff;    

	   y1+=dy;

          if (errorTerm > yDiff) 
          {     
             errorTerm -= yDiff;  
             offset += xStep;     
	     x1+=dx;
          }
       }
    }
 
}

int moveEndOfLine(FILE * file) {
  while (fgetc(file) != '\n') {
  }
  return 0;
}

Notebook loadFile(const char * fileName) {
  Notebook notebook;
  unsigned int pageNumber;

  FILE* save = fopen (fileName, "r");
  if (save == NULL) {
    printf("Failed to load file %s\n", fileName);
    notebook.pages.push_back(Page());
    return notebook;
  }
  else
    printf("File %s Opened\n", fileName);

  fscanf(save, "%d", &pageNumber);
  moveEndOfLine(save);
  printf("File contains %d pages\n", pageNumber);

  for (unsigned int iPage=0; iPage<pageNumber; iPage++) {
    unsigned int segmentNumber;
    Page page = Page();
    fscanf(save, "%d", &segmentNumber);
    moveEndOfLine(save);
    printf(" Page %d contains %d segments \n", iPage, segmentNumber);
    for (unsigned int iSegment=0; iSegment<segmentNumber; iSegment++) {
      unsigned int pointNumber;
      Segment segment = Segment();

      fscanf(save, "%d", &pointNumber);
      moveEndOfLine(save);
      printf("  Segment %d contains %d points \n", iSegment, pointNumber);

      for (unsigned int iPoint=0; iPoint<pointNumber; iPoint++) {
	unsigned int x,y;
	fscanf(save, "%d;%d", &x, &y);
	moveEndOfLine(save);

	segment.points.push_back(Point(x,y));
      }

      page.segments.push_back(segment);
    }
    notebook.pages.push_back(page);
  }


  fclose(save);

  if (notebook.pages.size() == 0)
    notebook.pages.push_back(Page());

  return notebook;
}

int saveFile(const char * fileName, Notebook notebook) {
  FILE* save = fopen (fileName, "w");
  if (save == NULL)
    printf("Failed to open file %s\n", fileName);
  else
    printf("File %s Opened\n", fileName);

  fprintf(save, "%d\t# number of page\n", notebook.pages.size());

  for (unsigned int iPage=0; iPage<notebook.pages.size(); iPage++) {
    Page page = notebook.pages[iPage];
    fprintf(save, "%d\t# segments for page %d\n", page.segments.size(), iPage);
    for (unsigned int iSegment=0; iSegment<page.segments.size(); iSegment++) {
      Segment segment = page.segments[iSegment];
      fprintf(save, "%d\t# points for segment %d-%d\n", segment.points.size(), iPage, iSegment);
      for (unsigned int iPoint=0; iPoint<segment.points.size(); iPoint++) {
	Point point = segment.points[iPoint];
	fprintf(save, "%d;%d\n", point.x, point.y);
      }
    }
  }
  fclose(save);
  printf("File saves\n");
  return 0;
}

void updateCenter(AppState & state) {
  Point bottomLeft = state.convertBufferToScreen(Point(0,0));
  Point topRight = state.convertBufferToScreen(Point(MY_BG_W,MY_BG_H));

  Point newCenter = state.convertScreenToImage(Point(SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
  printf("--------------\n");
  printf("%d %d\n", bottomLeft.x, bottomLeft.y);
  printf("%d %d\n", topRight.x, topRight.y);
  printf("%d %d\n", newCenter.x, newCenter.y);

  if (bottomLeft.x > 0 || bottomLeft.y > 0 || topRight.x < SCREEN_WIDTH || topRight.y < SCREEN_HEIGHT) {
    //Point newCenter = state.convertScreenToImage(Point(SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
      state.scroll_x = MY_BG_W/2-SCREEN_WIDTH/2;
      state.scroll_y = MY_BG_H/2-SCREEN_HEIGHT/2;
      state.center_x = newCenter.x;
      state.center_y = newCenter.y;

      printf("%d %d\n", state.center_x, state.center_y);
      bgHide(3);
      fillDisplay(RGB15(0,0,0) | BIT(15), state.lastPage, state);
      drawPage(state.currentPage, RGB15(31,0,0) | BIT(15), state);
  }
  bgSetScroll(3, state.scroll_x, state.scroll_y);
  bgUpdate();
  bgShow(3);
}

int main(void) {

  consoleDemoInit();
  //videoSetMode(MODE_FB0);
  videoSetMode( MODE_5_2D );
  vramSetBankA(VRAM_A_MAIN_BG);
  vramSetBankB(VRAM_B_MAIN_BG);
  // vramSetBankC(VRAM_C_MAIN_BG);
  // vramSetBankD(VRAM_D_MAIN_BG_0x06040000);
  int bg = bgInit(3, BgType_Bmp8, BgSize_B8_512x512, 0,0);  //vramSetBankA(VRAM_A_LCD);

  lcdMainOnBottom();
  // lcdMainOnTop();

  printf("Video modes configured\n");
  printf("BG ID %d\n", bg);

  if (fatInitDefault())
    printf("FAT initialized\n");
  else
    printf("FAT initialization failed\n");

  const char * fileName = "note.txt";
  Notebook notebook = loadFile(fileName);

  AppState state;
  state.lastPage = -1;
  state.currentPage = &(notebook.pages[0]);
  Segment * currentSegment = NULL;

  state.scroll_x = 0;
  state.scroll_y = 0;

  state.center_x = MY_BG_W/2;
  state.center_y = MY_BG_H/2;


  fillDisplay(RGB15(0,0,0) | BIT(15), 0, state);
  drawPage(state.currentPage, RGB15(31,31,31) | BIT(15), state);

  touchPosition touch;


  // Keyboard *kbd =  keyboardDemoInit();
  // char myName[256];
  // iprintf("What is your name?\n");
  // scanf("%s", myName);

  while (1) {
    scanKeys();
    
    if(keysHeld() & KEY_TOUCH)
      {
	// write the touchscreen coordinates in the touch variable
	touchRead(&touch);
	
	Point screenPoint = Point(touch.px, touch.py);
	Point imagePoint = state.convertScreenToImage(screenPoint);

	if (currentSegment == NULL) {
	  state.currentPage->segments.push_back(Segment());
	  currentSegment = &(state.currentPage->segments[state.currentPage->segments.size()-1]);
	  currentSegment->points.push_back(imagePoint);
	  continue;
	}

	Point lastImagePoint = currentSegment->points[currentSegment->points.size()-1];
	currentSegment->points.push_back(imagePoint);

	Point bufferPoint = state.convertScreenToBuffer(screenPoint);
	Point lastBufferPoint = state.convertImageToBuffer(lastImagePoint);
	drawLine(lastBufferPoint.x, lastBufferPoint.y,  bufferPoint.x, bufferPoint.y, RGB15(31,0,0) | BIT(15));
      }    
    else {
      currentSegment = NULL;
    }
    if (keysDown() & KEY_X) {
      int page = state.lastPage+1;
      while (notebook.pages.size() < page+1) {
	notebook.pages.push_back(Page());
	printf("New Page created\n");
      }
      state.currentPage = &(notebook.pages[page]);
      currentSegment = NULL;

      printf("Display page %d\n", page);

      fillDisplay(RGB15(0,0,0) | BIT(15), page, state);
      drawPage(state.currentPage, RGB15(31,0,0) | BIT(15), state);
    }
    if (keysDown() & KEY_Y) {
      int page = 0;
      if (state.lastPage > 0) {
	page = state.lastPage-1;
      }
      state.currentPage = &(notebook.pages[page]);
      currentSegment = NULL;

      printf("Display page %d\n", page);

      fillDisplay(RGB15(0,0,0) | BIT(15), page, state);
      drawPage(state.currentPage, RGB15(31,0,0) | BIT(15), state);
    }
    if (keysDown() & KEY_A) {
      int page = 0;
      notebook = loadFile(fileName);

      state.currentPage = &(notebook.pages[page]);
      currentSegment = NULL;

      printf("Display page %d\n", page);

      fillDisplay(RGB15(0,0,0) | BIT(15), page, state);
      drawPage(state.currentPage, RGB15(31,0,0) | BIT(15), state);
    }
    if (keysDown() & KEY_B) {
      saveFile(fileName, notebook);
    }
    if (keysDown() & KEY_LEFT) {
      state.scroll_x-=50;
      updateCenter(state);
    }
    if (keysDown() & KEY_RIGHT) {
      state.scroll_x+=50;
      updateCenter(state);
    }
    if (keysDown() & KEY_UP) {
      state.scroll_y-=50;
      updateCenter(state);
    }
    if (keysDown() & KEY_DOWN) {
      state.scroll_y+=50;
      updateCenter(state);
    }

    swiWaitForVBlank();
  }
}
