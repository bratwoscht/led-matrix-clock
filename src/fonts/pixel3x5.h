const uint8_t pixel3x5Bitmaps[] PROGMEM = {
   0x75, 0x55, 0x70, // 0
   0x26, 0x22, 0x70, // 1
   0x71, 0x74, 0x70, // 2
   0x71, 0x71, 0x70, // 3
   0x55, 0x71, 0x10, // 4
   0x74, 0x71, 0x70, // 5
   0x74, 0x75, 0x70, // 6
   0x71, 0x11, 0x10, // 7
   0x75, 0x75, 0x70, // 8
   0x75, 0x71, 0x70  // 9
};

const GFXglyph pixel3x5Glyphs[] PROGMEM = {
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

const GFXfont pixel3x5 PROGMEM = {
  (uint8_t  *)pixel3x5Bitmaps,
  (GFXglyph *)pixel3x5Glyphs,
  0x30, 0x39, 6 };

// Approx. 867 bytes
