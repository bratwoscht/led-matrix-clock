const uint8_t pixel5pt7bBitmaps[] PROGMEM = {
   0x69, 0x99, 0x60, // 0
   0x26, 0x22, 0x20, // 1
   0x69, 0x24, 0xF0, // 2
   0xE1, 0x61, 0xE0, // 3
   0x99, 0x71, 0x10, // 4
   0xF8, 0xE1, 0xE0, // 5
   0x78, 0xE9, 0x60, // 6
   0xF1, 0x24, 0x40, // 7
   0x69, 0x69, 0x60, // 8
   0x69, 0x71, 0xE0  // 9
};

const GFXglyph pixel5pt7bGlyphs[] PROGMEM = {
  {     0,   4,   5,   5,    0,   0 },   // 0x30 '0'
  {     3,   4,   5,   5,    0,   0 },   // 0x31 '1'
  {     6,   4,   5,   5,    0,   0 },   // 0x32 '2'
  {     9,   4,   5,   5,    0,   0 },   // 0x33 '3'
  {    12,   4,   5,   5,    0,   0 },   // 0x34 '4'
  {    15,   4,   5,   5,    0,   0 },   // 0x35 '5'
  {    18,   4,   5,   5,    0,   0 },   // 0x36 '6'
  {    21,   4,   5,   5,    0,   0 },   // 0x37 '7'
  {    24,   4,   5,   5,    0,   0 },   // 0x38 '8'
  {    27,   4,   5,   5,    0,   0 }    // 0x39 '9'
};

const GFXfont pixel5pt7b PROGMEM = {
  (uint8_t  *)pixel5pt7bBitmaps,
  (GFXglyph *)pixel5pt7bGlyphs,
  0x30, 0x39, 6 };

// Approx. 867 bytes
