#include <nds.h>
#include <fat.h>
#include <stdio.h>

#include "page.h"


#define MY_BG_W (512)
#define MY_BG_H (512)

void drawLine(int x1, int y1, int x2, int y2, unsigned short color);

int lastPage = -1;
sImage image;

void fillDisplay(unsigned short color, unsigned int page, unsigned int center_x, unsigned int center_y) {
  int offset = 0;
  // for (int y=0; y<MY_BG_H; y++) {
  //   offset = y * MY_BG_W;
  //   for (int x=0; x<MY_BG_W; x++) {
  //     offset+=1;
  //     BG_GFX[offset] = color;
  //   }
  // }
  
  if (page != lastPage) {
    if (lastPage != -1) {
      free(image.palette);
      // free(image.image.data8);
      free(image.image.data16);
    }

    //load our ball pcx file into an image
    int res = 0;
 
    char fileName[1024];
    sprintf(fileName, "image%02d.pcx", page);

    FILE * pFile = fopen ( fileName , "rb" );
    if (pFile==NULL) printf("File error\n");

    // obtain file size:
    long lSize;
    size_t result;
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    char * pcx_image = (char *)malloc (sizeof(char)*lSize);
    if (pcx_image == NULL) printf ("Memory error\n");

    // copy the file into the buffer:
    res = fread (pcx_image,1,lSize,pFile);
    if (res != lSize) {printf("Reading error\n");}

    // terminate
    fclose (pFile);


    res = loadPCX((u8*)pcx_image, &image); 
    free (pcx_image);

    printf("Loaded Background of size %d x %d\n", image.width, image.height);
    printf(" Bits per pixel %d\n", image.bpp);
    printf(" res %d\n", res);

    // image8to16(&image);

    printf(" palette size %d\n", sizeof(image.palette));

    for (int i=0; i<256; i++) {
      BG_PALETTE[i] = image.palette[i];
    }
    BG_PALETTE[1] = RGB15(31,0,0) | BIT(15);


    lastPage = page;
  }



  // dmaCopy(image.palette, BG_PALETTE, sizeof(image.palette));

  int imgO = 0;
  for (int y=0 ; y < MY_BG_H; y++) {
    int yImage = (y+center_y-MY_BG_H/2);
    offset = y * MY_BG_W / 2;
    imgO = yImage * image.width/2;
    for (int x=0; x<MY_BG_W; x+=2) {
      int xImage = (x+center_x-MY_BG_W/2);

      int offset2 = offset + x/2;
      int imgO2 = imgO + xImage/2;
      //BG_GFX[offset] = RGB15(31,0,0) | BIT(15);
      // BG_GFX[offset] = image.image.data16[imgO];
      if (yImage < 0 || yImage >= image.height)
	bgGetGfxPtr(3)[offset2] = 0;
      else if (xImage < 0 || xImage >= image.width)
	bgGetGfxPtr(3)[offset2] = 0;
      else
	bgGetGfxPtr(3)[offset2] = image.image.data16[imgO2];
      // bgGetGfxPtr(3)[offset2] = 0x0101;
    }
  }
  // dmaCopy(image.image.data8, bgGetGfxPtr(3), image.width*image.height);
}
void drawPage(Page * page, unsigned short color) {
  for (unsigned int iSeg = 0; iSeg < page->segments.size(); iSeg++) {
    Segment segment = page->segments[iSeg];
    for (unsigned int iPoint=0; iPoint<segment.points.size()-1; iPoint++) {
      Point p1 = segment.points[iPoint];
      Point p2 = segment.points[iPoint+1];
      drawLine(p1.x, p1.y, p2.x, p2.y, color);
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

    //need to adjust if y1 > y2
    if (yDiff < 0)       
    {                  
       yDiff = -yDiff;   //absolute value
       yStep = -yStep;   //step up instead of down   
    }
    
    //same for x
    if (xDiff < 0) 
    {           
       xDiff = -xDiff;            
       xStep = -xStep;            
    }        
 
    //case for changes more in X than in Y	 
    if (xDiff > yDiff) 
    {                            
       for (i = 0; i < xDiff + 1; i++)
       {                           
          // VRAM_A[offset] = color;  
	 // BG_GFX[offset] = color; 
	 int offset2 = offset/2;
	 if (offset%2 == 0) {
	   BG_GFX[offset2] = (BG_GFX[offset2] & 0xff00) | color;
	 }
	 else {
	   BG_GFX[offset2] = (BG_GFX[offset2] & 0x00ff) | (color<<8);
	 }

          offset += xStep;           
 
          errorTerm += yDiff;     
 
          if (errorTerm > xDiff) 
          {  
             errorTerm -= xDiff;     
             offset += yStep;        
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
	 int offset2 = offset/2;
	 if (offset%2 == 0) {
	   BG_GFX[offset2] = (BG_GFX[offset2] & 0xff00) | color;
	 }
	 else {
	   BG_GFX[offset2] = (BG_GFX[offset2] & 0x00ff) | (color<<8);
	 }
 
          offset += yStep;           
 
          errorTerm += xDiff;    
 
          if (errorTerm > yDiff) 
          {     
             errorTerm -= yDiff;  
             offset += xStep;     
 
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

void updateCenter(int & center_x, int & center_y, int & scroll_x, int & scroll_y, Page* currentPage, unsigned int currentPageIdx) {
  int x_remain = MY_BG_W-SCREEN_WIDTH-scroll_x;
  int tmp_cx = center_x-MY_BG_W/2+scroll_x+SCREEN_WIDTH/2;
  int y_remain = MY_BG_H-SCREEN_HEIGHT-scroll_y;
  int tmp_cy = center_y-MY_BG_H/2+scroll_y+SCREEN_HEIGHT/2;
  printf("Scroll X %d Remain %d Center %d\n", scroll_x, x_remain, tmp_cx);
  printf("Scroll Y %d Remain %d Center %d\n", scroll_y, y_remain, tmp_cy);
  if (x_remain < 0 || y_remain < 0 || scroll_x<0 || scroll_y<0) {
    scroll_x = MY_BG_W/2-SCREEN_WIDTH/2;
    scroll_y = MY_BG_H/2-SCREEN_HEIGHT/2;
    center_x = tmp_cx;
    center_y = tmp_cy;
    bgHide(3);
    fillDisplay(RGB15(0,0,0) | BIT(15), currentPageIdx, center_x, center_y);
    drawPage(currentPage, RGB15(31,0,0) | BIT(15));
  }
  bgSetScroll(3, scroll_x, scroll_y);
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



  unsigned int currentPageIdx = 0;
  Page * currentPage = &(notebook.pages[currentPageIdx]);
  Segment * currentSegment = NULL;

  int scroll_x = 0;
  int scroll_y = 0;

  int center_x = MY_BG_W/2;
  int center_y = MY_BG_H/2;


  fillDisplay(RGB15(0,0,0) | BIT(15), currentPageIdx, center_x, center_y);
  drawPage(currentPage, RGB15(31,31,31) | BIT(15));

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
	
	Point touchPoint = Point(center_x-MY_BG_W/2+scroll_x+touch.px,
				 center_y-MY_BG_H/2+scroll_y+touch.py);

	if (currentSegment == NULL) {
	  currentPage->segments.push_back(Segment());
	  currentSegment = &(currentPage->segments[currentPage->segments.size()-1]);
	  currentSegment->points.push_back(touchPoint);
	  continue;
	}

	Point lastPoint = currentSegment->points[currentSegment->points.size()-1];
	currentSegment->points.push_back(touchPoint);

	drawLine(lastPoint.x, lastPoint.y, touchPoint.x,touchPoint.y, RGB15(31,0,0) | BIT(15));
      }    
    else {
      currentSegment = NULL;
    }
    if (keysDown() & KEY_X) {
      currentPageIdx+=1;
      while (notebook.pages.size() < currentPageIdx+1) {
	notebook.pages.push_back(Page());
	printf("New Page created\n");
      }
      currentPage = &(notebook.pages[currentPageIdx]);
      currentSegment = NULL;

      printf("Display page %d\n", currentPageIdx);

      fillDisplay(RGB15(0,0,0) | BIT(15), currentPageIdx, center_x, center_y);
      drawPage(currentPage, RGB15(31,0,0) | BIT(15));
    }
    if (keysDown() & KEY_Y) {
      if (currentPageIdx > 0) {
	currentPageIdx-=1;
      }
      currentPage = &(notebook.pages[currentPageIdx]);
      currentSegment = NULL;

      printf("Display page %d\n", currentPageIdx);

      fillDisplay(RGB15(0,0,0) | BIT(15), currentPageIdx, center_x, center_y);
      drawPage(currentPage, RGB15(31,0,0) | BIT(15));
    }
    if (keysDown() & KEY_A) {
      currentPageIdx=0;
      notebook = loadFile(fileName);

      currentPage = &(notebook.pages[currentPageIdx]);
      currentSegment = NULL;

      printf("Display page %d\n", currentPageIdx);

      fillDisplay(RGB15(0,0,0) | BIT(15), currentPageIdx, center_x, center_y);
      drawPage(currentPage, RGB15(31,0,0) | BIT(15));
    }
    if (keysDown() & KEY_B) {
      saveFile(fileName, notebook);
    }
    if (keysDown() & KEY_LEFT) {
      scroll_x+=20;
      updateCenter(center_x, center_y, scroll_x, scroll_y, currentPage, currentPageIdx);
    }
    if (keysDown() & KEY_RIGHT) {
      scroll_x-=20;
      updateCenter(center_x, center_y, scroll_x, scroll_y, currentPage, currentPageIdx);
    }
    if (keysDown() & KEY_UP) {
      scroll_y+=20;
      updateCenter(center_x, center_y, scroll_x, scroll_y, currentPage, currentPageIdx);
    }
    if (keysDown() & KEY_DOWN) {
      scroll_y-=20;
      updateCenter(center_x, center_y, scroll_x, scroll_y, currentPage, currentPageIdx);
    }

    swiWaitForVBlank();
  }
}
