// Font generated by stb_font_inl_generator.c (4/1 bpp)
//
// Following instructions show how to use the only included font, whatever it is, in
// a generic way so you can replace it with any other font by changing the include.
// To use multiple fonts, replace STB_SOMEFONT_* below with STB_FONT_courier_bold_6_latin1_*,
// and separately install each font. Note that the CREATE function call has a
// totally different name; it's just 'stb_font_courier_bold_6_latin1'.
//
/* // Example usage:

static stb_fontchar fontdata[STB_SOMEFONT_NUM_CHARS];

static void init(void)
{
    // optionally replace both STB_SOMEFONT_BITMAP_HEIGHT with STB_SOMEFONT_BITMAP_HEIGHT_POW2
    static unsigned char fontpixels[STB_SOMEFONT_BITMAP_HEIGHT][STB_SOMEFONT_BITMAP_WIDTH];
    STB_SOMEFONT_CREATE(fontdata, fontpixels, STB_SOMEFONT_BITMAP_HEIGHT);
    ... create texture ...
    // for best results rendering 1:1 pixels texels, use nearest-neighbor sampling
    // if allowed to scale up, use bilerp
}

// This function positions characters on integer coordinates, and assumes 1:1 texels to pixels
// Appropriate if nearest-neighbor sampling is used
static void draw_string_integer(int x, int y, char *str) // draw with top-left point x,y
{
    ... use texture ...
    ... turn on alpha blending and gamma-correct alpha blending ...
    glBegin(GL_QUADS);
    while (*str) {
        int char_codepoint = *str++;
        stb_fontchar *cd = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];
        glTexCoord2f(cd->s0, cd->t0); glVertex2i(x + cd->x0, y + cd->y0);
        glTexCoord2f(cd->s1, cd->t0); glVertex2i(x + cd->x1, y + cd->y0);
        glTexCoord2f(cd->s1, cd->t1); glVertex2i(x + cd->x1, y + cd->y1);
        glTexCoord2f(cd->s0, cd->t1); glVertex2i(x + cd->x0, y + cd->y1);
        // if bilerping, in D3D9 you'll need a half-pixel offset here for 1:1 to behave correct
        x += cd->advance_int;
    }
    glEnd();
}

// This function positions characters on float coordinates, and doesn't require 1:1 texels to pixels
// Appropriate if bilinear filtering is used
static void draw_string_float(float x, float y, char *str) // draw with top-left point x,y
{
    ... use texture ...
    ... turn on alpha blending and gamma-correct alpha blending ...
    glBegin(GL_QUADS);
    while (*str) {
        int char_codepoint = *str++;
        stb_fontchar *cd = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];
        glTexCoord2f(cd->s0f, cd->t0f); glVertex2f(x + cd->x0f, y + cd->y0f);
        glTexCoord2f(cd->s1f, cd->t0f); glVertex2f(x + cd->x1f, y + cd->y0f);
        glTexCoord2f(cd->s1f, cd->t1f); glVertex2f(x + cd->x1f, y + cd->y1f);
        glTexCoord2f(cd->s0f, cd->t1f); glVertex2f(x + cd->x0f, y + cd->y1f);
        // if bilerping, in D3D9 you'll need a half-pixel offset here for 1:1 to behave correct
        x += cd->advance;
    }
    glEnd();
}
*/

#ifndef STB_FONTCHAR__TYPEDEF
#define STB_FONTCHAR__TYPEDEF
typedef struct
{
    // coordinates if using integer positioning
    float s0,t0,s1,t1;
    signed short x0,y0,x1,y1;
    int   advance_int;
    // coordinates if using floating positioning
    float s0f,t0f,s1f,t1f;
    float x0f,y0f,x1f,y1f;
    float advance;
} stb_fontchar;
#endif

#define STB_FONT_courier_bold_6_latin1_BITMAP_WIDTH         256
#define STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT         24
#define STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT_POW2    32

#define STB_FONT_courier_bold_6_latin1_FIRST_CHAR            32
#define STB_FONT_courier_bold_6_latin1_NUM_CHARS            224

#define STB_FONT_courier_bold_6_latin1_LINE_SPACING           3

static unsigned int stb__courier_bold_6_latin1_pixels[]={
    0x03018111,0x20202001,0x08080800,0x41861100,0x3060c081,0x060c1830,
    0x40881826,0x81818180,0x10102221,0x10d4c081,0x28660443,0x505442a2,
    0x18801415,0x40081020,0xa8982040,0x33056428,0x204c0a85,0x17104c42,
    0x41a882e2,0x31104c42,0x37158544,0x21c88666,0x14d42a0b,0x1705c371,
    0x22790666,0xc982a20c,0x0b886e21,0x51ba82e2,0x5104c431,0x4e6614c1,
    0x4cd750b8,0x2ea7353d,0x1ce47310,0xa41a9851,0x54351198,0x88cc42a0,
    0x2a21511a,0xc8917190,0xba922e49,0x3ba8eea3,0x52989672,0x752ce477,
    0xb392ce47,0x9374a4dc,0x2e93724e,0x24e93749,0x65732b99,0x4ab9275c,
    0x31e4c793,0x931e4c79,0x7571e4c7,0x64b5724e,0x3303a65c,0x31e5cf2a,
    0x3a89ce7b,0x1c3cc8ea,0x366716b6,0x47663b31,0x329791d9,0x2f29794b,
    0x33206c74,0x38e706c3,0x41ce39c7,0x9c738e72,0xa8e71ce3,0x54eea33b,
    0x5d4eea5b,0x96ea7753,0x2a7753ba,0x5d4b753b,0x2ddcb772,0x72ddcb77,
    0x792ddcb7,0x41b16ea7,0x70f2a3ba,0x23cc9e44,0x265932d9,0x0ec7732b,
    0x3897714c,0x71c3870e,0x5cb72dcb,0x0ec2d4e5,0x5457516a,0xba8aea2b,
    0x15d46e62,0x515d4575,0x0015d457,0x00000000,0x00000000,0x3b100000,
    0x000d9d80,0x00013101,0x2cec3004,0x6ccb6691,0x4b665b32,0x32b795bc,
    0x26eb795b,0x00880808,0x00080000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x06044000,
    0x00806000,0x02010101,0x0840850a,0x0c4c6628,0x886a0420,0x0882182a,
    0x1020982a,0x5131a808,0x08608808,0x11022082,0x1a80c110,0x41082204,
    0x060c4c09,0x440830d4,0x21830540,0x0cc144ab,0x02b205cc,0x2a0c140a,
    0x131dd42c,0x314caea5,0x77198e23,0x439324e5,0xb966e59c,0x2e725449,
    0x26a7354a,0x4eaa7554,0x5c2aa3ba,0x470eea48,0x231c72c8,0x306d44aa,
    0x370ca19d,0x7590dcc5,0x21a896ae,0x65432893,0x30654532,0x228a4dc5,
    0x03a60e28,0x40e981d3,0x2074c49c,0x950e60e9,0xa84c5c13,0x3225712b,
    0xb98e4872,0x05c4b772,0x2a5ba8e7,0x71926e4b,0x4b913da8,0x1ce5711a,
    0x2706c575,0x8e317243,0x18e6e90c,0x2ea3da87,0x44b8eea4,0x543d13ad,
    0x7716ea4c,0x248eea17,0x7752439b,0x23ba9224,0x21e543ca,0x21dec3ca,
    0x51e543ca,0x2a1dd457,0x65462a2c,0x2705caa2,0x90395428,0x36215d47,
    0x75d47710,0x5d4eea70,0x770c3713,0x3988aea7,0x1c3ca8b5,0xbac71c36,
    0xbb0aee4b,0x50e2c439,0xe89dec7b,0x19002c41,0x2ce4b660,0x4b6605b3,
    0xb3b0e2d9,0x3b06cec1,0x1b3b001b,0x00006cec,0x5100c150,0x01540c66,
    0x80081000,0x20000002,0xc88110a9,0x0e280543,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x43100000,0x06185501,0x41506214,0x110d4199,
    0x10440a88,0x84220201,0x10888440,0x81081082,0x1110a810,0x44088222,
    0x40881108,0x04410810,0x00c41042,0x44018072,0x14c45130,0x0ccc0306,
    0x898a5011,0x5d46a201,0x866a4862,0xab8a25cb,0x1ec4b6a0,0x86f5cb99,
    0x98baa29b,0x21d6403c,0x647660e9,0x3bb96459,0x2255cb57,0x86c6c5ad,
    0x2ebb938a,0xbc8eb24b,0x1d649754,0x0e8e8b5b,0x824bc895,0x25b11c49,
    0x4cea0c3a,0x6c49372a,0x8e44b261,0x4c7753ca,0xb326643b,0x1c995545,
    0x23854735,0x90f720da,0xc8921cdc,0xcec8eea0,0x254c7750,0x4cb80b77,
    0xbb8e1e54,0x20c8aea4,0x3ae0740d,0x46e1d4c4,0x5c1b30b8,0x3c996e3c,
    0xc89771ce,0xdb82a25d,0x9ed470a0,0x229665d9,0xb506c03c,0x33000843,
    0x32a0d4c3,0x2a896f24,0x98aa67b5,0x49772d98,0xb392d980,0x02d99e54,
    0x803d5000,0x89790888,0x25b30d9d,0x5cf2259c,0x4e43b14c,0x0742cdc5,
    0x5c7625b9,0xbc864e44,0x23bd8955,0x86cec59d,0x092725db,0x2a038b66,
    0x0001d403,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x40a98000,0x9820c1aa,
    0x30a04118,0x4050440c,0x2e0b8862,0x0dcc224b,0x00001777,0x00000000,
    0x00000000,0x00000000,0x00000000,0x20000000,0x2266439b,0x3a277519,
    0x544b6671,0x5ddc08a1,0x408a3940,0x0004380b,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0xb1672000,0x9860c199,0x80722118,
    0x00000009,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,
};

static signed short stb__courier_bold_6_latin1_x[224]={ 0,1,0,0,0,0,0,1,1,0,0,0,1,0,
1,0,0,0,0,0,0,0,0,0,0,0,1,1,-1,0,0,0,0,-1,0,0,0,0,0,0,0,0,0,0,
0,-1,-1,0,0,0,0,0,0,0,-1,-1,0,0,0,1,0,0,0,-1,1,0,-1,0,0,0,0,0,0,0,
0,0,0,-1,0,0,-1,0,0,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
1,0,0,-1,0,0,-1,0,-1,-1,0,0,0,0,1,0,0,1,1,0,0,0,0,-1,0,0,-1,-1,-1,-1,
-1,-1,-1,0,0,0,0,0,0,0,0,0,-1,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,0,
 };
static signed short stb__courier_bold_6_latin1_y[224]={ 4,0,0,0,0,0,1,0,0,0,0,0,3,2,
3,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,1,0,1,0,1,0,1,0,0,
0,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,1,0,0,1,0,
0,0,0,0,0,1,2,2,0,-1,0,0,0,0,0,1,0,1,3,0,0,1,0,0,0,1,-1,-1,-1,-1,
-1,-1,0,0,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,-1,-1,-1,-1,-1,1,0,-1,-1,-1,-1,-1,0,0,0,0,
0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,
 };
static unsigned short stb__courier_bold_6_latin1_w[224]={ 0,1,3,3,3,3,3,1,2,2,3,3,2,3,
1,3,3,3,3,3,3,3,3,3,3,3,1,2,4,4,4,3,3,5,4,3,3,3,4,4,4,3,4,4,
4,5,5,4,4,4,4,3,3,4,5,5,4,4,3,2,3,2,3,5,2,4,5,4,4,3,4,4,4,3,
3,4,3,5,4,3,5,4,4,3,4,4,4,5,4,4,3,3,1,3,3,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0,1,3,3,3,4,
1,3,3,5,3,3,4,3,5,5,3,3,3,3,2,4,3,1,2,3,3,4,4,5,4,3,5,5,5,5,
5,5,5,3,3,3,3,3,3,3,3,3,4,5,4,4,4,4,4,3,4,4,4,4,4,4,4,3,4,4,
4,4,4,4,5,4,3,3,3,3,3,3,3,3,3,4,3,3,3,3,3,3,4,4,4,4,4,4,5,4,
 };
static unsigned short stb__courier_bold_6_latin1_h[224]={ 0,5,3,5,5,5,4,3,5,5,3,4,2,1,
2,5,5,4,4,5,4,5,5,5,5,5,4,4,4,3,4,5,5,4,4,5,4,4,4,5,4,4,5,4,
4,4,4,5,4,5,4,5,4,5,4,4,4,4,4,5,5,5,3,1,2,4,5,4,5,4,4,5,4,4,
6,4,4,3,3,4,5,5,3,4,5,4,3,3,3,5,3,5,5,5,3,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0,5,5,4,3,4,
5,5,2,5,3,3,2,1,5,2,2,4,3,3,2,5,5,2,2,3,3,3,4,4,4,5,5,5,5,5,
5,5,4,5,5,5,5,5,5,5,5,5,4,5,6,6,6,6,6,3,5,6,6,6,6,5,4,5,5,5,
5,5,5,5,4,4,5,5,5,5,4,4,4,4,5,4,5,5,5,5,5,3,4,5,5,5,5,6,6,6,
 };
static unsigned short stb__courier_bold_6_latin1_s[224]={ 66,70,193,186,61,65,177,24,69,72,26,
173,54,73,47,75,190,169,113,198,83,194,177,173,119,123,186,166,188,219,181,
127,57,68,63,115,140,131,121,170,78,74,175,161,156,150,144,135,135,149,126,
131,117,93,107,101,96,91,87,253,208,218,207,77,57,53,23,43,108,33,28,
103,19,15,37,5,1,6,1,243,202,98,202,226,88,215,197,229,235,144,244,
154,162,164,215,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,
58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,66,168,158,
196,248,205,113,140,69,51,211,240,64,73,45,41,60,24,252,12,38,83,79,
49,51,34,30,224,186,220,210,41,35,29,167,17,11,5,180,1,249,245,241,
237,233,229,225,221,200,212,27,51,17,41,22,20,181,56,32,46,61,158,48,
149,144,139,134,129,124,119,37,191,106,102,98,163,251,247,230,239,115,234,89,
85,72,81,66,16,10,76,110,93,153,12,6,1, };
static unsigned short stb__courier_bold_6_latin1_t[224]={ 7,1,14,1,8,8,14,19,8,8,19,
14,19,19,19,8,1,14,14,1,14,1,1,1,8,8,14,14,14,14,14,
8,8,14,14,8,14,14,14,8,14,14,8,14,14,14,14,8,14,8,14,
8,14,8,14,14,14,14,14,1,1,1,14,19,19,14,8,14,8,14,14,
8,14,14,1,14,14,19,19,8,1,8,14,8,8,8,14,14,14,8,14,
8,8,8,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,7,8,8,
8,14,8,8,8,19,8,14,14,19,19,8,19,19,14,14,19,19,8,8,
19,19,19,19,14,8,8,8,8,8,8,1,8,8,8,8,8,1,1,1,
1,1,1,1,1,8,1,1,1,1,1,1,19,1,1,1,1,1,1,14,
1,1,1,1,1,1,1,14,8,1,1,1,1,8,8,8,8,1,8,1,
1,1,1,1,19,14,1,1,1,1,1,1,1, };
static unsigned short stb__courier_bold_6_latin1_a[224]={ 51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,
51,51,51,51,51,51,51,51, };

// Call this function with
//    font: NULL or array length
//    data: NULL or specified size
//    height: STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT or STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT_POW2
//    return value: spacing between lines
static void stb_font_courier_bold_6_latin1(stb_fontchar font[STB_FONT_courier_bold_6_latin1_NUM_CHARS],
                unsigned char data[STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT][STB_FONT_courier_bold_6_latin1_BITMAP_WIDTH],
                int height)
{
    int i,j;
    if (data != 0) {
        unsigned int *bits = stb__courier_bold_6_latin1_pixels;
        unsigned int bitpack = *bits++, numbits = 32;
        for (i=0; i < STB_FONT_courier_bold_6_latin1_BITMAP_WIDTH*height; ++i)
            data[0][i] = 0;  // zero entire bitmap
        for (j=1; j < STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT-1; ++j) {
            for (i=1; i < STB_FONT_courier_bold_6_latin1_BITMAP_WIDTH-1; ++i) {
                unsigned int value;
                if (numbits==0) bitpack = *bits++, numbits=32;
                value = bitpack & 1;
                bitpack >>= 1, --numbits;
                if (value) {
                    if (numbits < 3) bitpack = *bits++, numbits = 32;
                    data[j][i] = (bitpack & 7) * 0x20 + 0x1f;
                    bitpack >>= 3, numbits -= 3;
                } else {
                    data[j][i] = 0;
                }
            }
        }
    }

    // build font description
    if (font != 0) {
        float recip_width = 1.0f / STB_FONT_courier_bold_6_latin1_BITMAP_WIDTH;
        float recip_height = 1.0f / height;
        for (i=0; i < STB_FONT_courier_bold_6_latin1_NUM_CHARS; ++i) {
            // pad characters so they bilerp from empty space around each character
            font[i].s0 = (stb__courier_bold_6_latin1_s[i]) * recip_width;
            font[i].t0 = (stb__courier_bold_6_latin1_t[i]) * recip_height;
            font[i].s1 = (stb__courier_bold_6_latin1_s[i] + stb__courier_bold_6_latin1_w[i]) * recip_width;
            font[i].t1 = (stb__courier_bold_6_latin1_t[i] + stb__courier_bold_6_latin1_h[i]) * recip_height;
            font[i].x0 = stb__courier_bold_6_latin1_x[i];
            font[i].y0 = stb__courier_bold_6_latin1_y[i];
            font[i].x1 = stb__courier_bold_6_latin1_x[i] + stb__courier_bold_6_latin1_w[i];
            font[i].y1 = stb__courier_bold_6_latin1_y[i] + stb__courier_bold_6_latin1_h[i];
            font[i].advance_int = (stb__courier_bold_6_latin1_a[i]+8)>>4;
            font[i].s0f = (stb__courier_bold_6_latin1_s[i] - 0.5f) * recip_width;
            font[i].t0f = (stb__courier_bold_6_latin1_t[i] - 0.5f) * recip_height;
            font[i].s1f = (stb__courier_bold_6_latin1_s[i] + stb__courier_bold_6_latin1_w[i] + 0.5f) * recip_width;
            font[i].t1f = (stb__courier_bold_6_latin1_t[i] + stb__courier_bold_6_latin1_h[i] + 0.5f) * recip_height;
            font[i].x0f = stb__courier_bold_6_latin1_x[i] - 0.5f;
            font[i].y0f = stb__courier_bold_6_latin1_y[i] - 0.5f;
            font[i].x1f = stb__courier_bold_6_latin1_x[i] + stb__courier_bold_6_latin1_w[i] + 0.5f;
            font[i].y1f = stb__courier_bold_6_latin1_y[i] + stb__courier_bold_6_latin1_h[i] + 0.5f;
            font[i].advance = stb__courier_bold_6_latin1_a[i]/16.0f;
        }
    }
}

#ifndef STB_SOMEFONT_CREATE
#define STB_SOMEFONT_CREATE              stb_font_courier_bold_6_latin1
#define STB_SOMEFONT_BITMAP_WIDTH        STB_FONT_courier_bold_6_latin1_BITMAP_WIDTH
#define STB_SOMEFONT_BITMAP_HEIGHT       STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT
#define STB_SOMEFONT_BITMAP_HEIGHT_POW2  STB_FONT_courier_bold_6_latin1_BITMAP_HEIGHT_POW2
#define STB_SOMEFONT_FIRST_CHAR          STB_FONT_courier_bold_6_latin1_FIRST_CHAR
#define STB_SOMEFONT_NUM_CHARS           STB_FONT_courier_bold_6_latin1_NUM_CHARS
#define STB_SOMEFONT_LINE_SPACING        STB_FONT_courier_bold_6_latin1_LINE_SPACING
#endif
