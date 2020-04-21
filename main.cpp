#include <iostream>
#include <string>
#include <malloc.h>
#include <string.h>
#include <vector>
#include <numeric>
#include <bitset>

#include "CImg.h"

//Settings
#define FILE_INPUT "entrada.bmp"
#define FILE_OUTPUT "saida.bmp"
#define N_COLORS 8

using namespace cimg_library;
using namespace std;        //delete
class Color
{
public:
    unsigned char c;
    int X,Y;
    bool operator >(Color d) const
    {
        return c>d.c;
    }
    bool operator <(Color d) const
    {
        return c<d.c;
    }

};

#define BLUE_GREATER 0
#define GREEN_GREATER 1
#define RED_GREATER 2
class RGB
{
public:
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned int  X;
    unsigned int  Y;
    unsigned int  *sX;
    unsigned int  *sY;
    unsigned int  sizeselect;
    void LOG(bool pause)
    {
        std::cout<<(int)B<<" "<<(int)R<<" "<<(int)G<<" POS:("<<X<<","<<Y<<")"<<endl;
        if(pause)
        {
            cin.get();
        }
    }
    unsigned char get_greater_color()
    {
        return (B>=G&&B>=R?B:G>=R&&G>=B?G:R);
    }
    int get_greater_index()
    {
        return (B>=G&&B>=R?BLUE_GREATER:G>=R&&G>B?GREEN_GREATER:RED_GREATER);
    }
    bool operator <(const RGB &r) const
    {
        return B<r.B;
    }
};
RGB _return_to_rgb(CImg <unsigned char> &img,unsigned int x,unsigned int y)
{
    return {img.atXY(x,y,0,0),img.atXY(x,y,0,1),img.atXY(x,y,0,2),x,y};
}
typedef struct pixelsort
{
    vector <Color> red;  //Red sorted
    vector <Color> blue; //Blue sorted
    vector <Color> green;//Green sorted
} PixelSort;
bool range_colors(RGB rgb,PixelSort &s)
{
    Color c;
    c.c = rgb.R;
    c.X=rgb.X;
    c.Y=rgb.Y;
    s.red.push_back(c);
    c.c = rgb.G;
    s.green.push_back(c);
    c.c = rgb.B;
    s.blue.push_back(c);
}
RGB assign_color(CImg <unsigned char> &img,Color s)
{
    return _return_to_rgb(img,s.X,s.Y);
}
int sort_color(PixelSort &s,unsigned int start,unsigned int endof)
{
    vector <Color>::iterator itRed = s.red.begin();
    vector <Color>::iterator itGreen = s.green.begin();
    vector <Color>::iterator itBlue = s.blue.begin();

    sort((itRed+start),(itRed+endof),greater<Color>());
    sort((itBlue+start),(itBlue+endof),greater<Color>());
    sort((itGreen+start),(itGreen+endof),greater<Color>());

    RGB r;
    r.R = (*(itRed+start)).c-(*(itRed+endof)).c;
    r.G = (*(itGreen+start)).c-(*(itGreen+endof)).c;
    r.B = (*(itBlue+start)).c-(*(itBlue+endof)).c;
    //r.LOG(true);
    return r.get_greater_index();
}

RGB *create_bucket(CImg <unsigned char> &img,int nColors,PixelSort &s)
{
    int linesize = img.height()*img.width();
    int start,endof;
    int layers = log2(nColors);
    int mapb = 0;
    if(nColors%2 == 1|s.blue.empty()||s.green.empty()||s.red.empty())
    {
        return NULL;
    }

    RGB *rgb = new RGB[nColors];
    for(int t =0 ; t<layers; t++)
    {
        mapb |= 1<<t;
    }
    #ifdef DEBUG
        cout<<"Layers:"<<layers<<endl;
    #endif // DEBUG


    PixelSort toIn = s;
    for(mapb; mapb>=0; mapb--)
    {
        start =0;
        endof = linesize;
        #ifdef DEBUG
            cout<<"Contador da arvore:"<<bitset<8>(mapb)<<" dec("<<mapb<<")"<<endl;
        #endif // DEBUG
        for(int e=layers-1,k,c=mapb; e>=0; e--)
        {
            k = c>>e;
            int rc = sort_color(toIn,start,endof);
            if(k==1)
            {
                start+=(endof - start)/2;
            }
            if(k==0)         //upper half
            {
                endof-= (endof - start)/2;
            }
            if(rc == GREEN_GREATER)
            {
                //Assign other colors according to green order
                for(int i=start; i<endof; i++)
                {
                    int xPos = toIn.green[i].X;
                    int yPos = toIn.green[i].Y;
                    toIn.blue[i].c = img.atXY(xPos,yPos,0,0);
                    toIn.red[i].c =  img.atXY(xPos,yPos,0,2);
                    toIn.blue[i].X = toIn.red[i].X = xPos;
                    toIn.blue[i].Y = toIn.red[i].Y = yPos;
                }
            }
            if(rc == BLUE_GREATER)
            {
                //Assign other colors according to blue order
                for(int i=start; i<endof; i++)
                {
                    int xPos = toIn.blue[i].X;
                    int yPos = toIn.blue[i].Y;
                    toIn.green[i].c = img.atXY(xPos,yPos,0,1);
                    toIn.red[i].c =  img.atXY(xPos,yPos,0,2);
                    toIn.green[i].X = toIn.red[i].X = xPos;
                    toIn.green[i].Y = toIn.red[i].Y = yPos;
                }
            }
            if(rc == RED_GREATER)
            {
                //Assign other colors according to red order
                for(int i=start; i<endof; i++)
                {
                    int xPos = toIn.red[i].X;
                    int yPos = toIn.red[i].Y;
                    toIn.blue[i].c = img.atXY(xPos,yPos,0,0);
                    toIn.green[i].c =  img.atXY(xPos,yPos,0,1);
                    toIn.blue[i].X = toIn.green[i].X = xPos;
                    toIn.blue[i].Y = toIn.green[i].Y = yPos;
                }
            }
            c = ~((~c)|1<<e); //zera o bit ja lido
        }

        unsigned int R = 0, G =0, B =0;
        rgb[mapb].sizeselect = endof-start;
        rgb[mapb].sX = new unsigned int[endof-start];
        rgb[mapb].sY = new unsigned int[endof-start];
        for(unsigned int i=start,e=0; i<endof; i++,e++)
        {
            rgb[mapb].sX[e] = toIn.blue[i].X;
            rgb[mapb].sY[e] = toIn.blue[i].Y;
            B+=toIn.blue[i].c;
            G+=toIn.green[i].c;
            R+=toIn.red[i].c;
        }
        rgb[mapb].B = B/(endof-start);
        rgb[mapb].G = G/(endof-start);
        rgb[mapb].R = R/(endof-start);
        toIn = s;//original set of pixels
    }
    return rgb;

}
int main(int argc,char *argv[])
{

    CImg <unsigned char> image(FILE_INPUT);
    PixelSort ps;
    unsigned int height = image.height();
    unsigned int width = image.width();

    #ifdef DEBUG
        cout<<"Image height:"<<height<<endl;
        cout<<"Image width:"<<width<<endl;
        cout<<"Array size (height*width):"<<height*width<<endl;
    #endif // DEBUG

    for(unsigned int i=0; i<height; i++)
    {
        for(unsigned int e=0; e<width; e++)
        {
            range_colors(_return_to_rgb(image,e,i),ps);
        }
    }
    RGB *rgb = create_bucket(image,N_COLORS,ps);
    CImg <unsigned char> im(width,height,1,3,0);
    for(int i=0;i<N_COLORS;i++){
         for(int e=0;e<rgb[i].sizeselect;e++){
            im(rgb[i].sX[e],rgb[i].sY[e],0) = rgb[i].B;
            im(rgb[i].sX[e],rgb[i].sY[e],1) = rgb[i].G;
            im(rgb[i].sX[e],rgb[i].sY[e],2) = rgb[i].R;
         }
    }
    im.save_bmp(FILE_OUTPUT);
    return 0;
}











