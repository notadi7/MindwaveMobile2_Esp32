/************************************************************
   OLED_DISPLAY.INO
   Small helpers for writing to the SH1106 OLED.
************************************************************/

/************************************************************
   🖥️ SIMPLE TEXT DISPLAY
************************************************************/
void showText(String line1, String line2, String line3) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 10);
  display.println(line1);

  display.setCursor(0, 30);
  display.println(line2);

  display.setCursor(0, 50);
  display.println(line3);

  display.display();
}

/************************************************************
   🖥️ CENTER TEXT FUNCTION
************************************************************/
void drawCenteredText(String text, int y) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - w) / 2;

  display.setCursor(x, y);
  display.print(text);
}
