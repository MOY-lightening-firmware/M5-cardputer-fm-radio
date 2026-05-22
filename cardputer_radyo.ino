#include <M5Cardputer.h>
#include <Wire.h>

#define TEA5767_ADDR 0x60

#define FREQ_MIN 87.5
#define FREQ_MAX 108.0
#define FREQ_STEP_LARGE 1.0
#define FREQ_STEP_SMALL 0.1

#define BG_COLOR        0x0000
#define PRIMARY_COLOR   0xFEA0
#define SECONDARY_COLOR 0xFD20
#define TEXT_COLOR      0xFFFF
#define DIM_COLOR       0x8C60
#define SIGNAL_COLOR    0xFEA0
#define WARN_COLOR      0xF800

float currentFreq = 100.0;
bool isMuted      = false;
bool isStereo     = false;
int  signalLevel  = 0;

void setFrequency(float freq) {
    if (freq < FREQ_MIN) freq = FREQ_MIN;
    if (freq > FREQ_MAX) freq = FREQ_MAX;
    currentFreq = freq;

    uint32_t freqB = (uint32_t)((freq * 1000000.0 + 225000.0) / 8192.0);

    uint8_t buf[5];
    buf[0] = (freqB >> 8) & 0x3F;
    buf[1] =  freqB & 0xFF;
    buf[2] = 0xB0;
    buf[3] = 0x10;
    buf[4] = 0x00;

    if (isMuted) buf[0] |= 0x80;

    Wire.beginTransmission(TEA5767_ADDR);
    Wire.write(buf, 5);
    Wire.endTransmission();
}

void readStatus() {
    Wire.requestFrom(TEA5767_ADDR, 5);
    if (Wire.available() >= 5) {
        uint8_t buf[5];
        for (int i = 0; i < 5; i++) buf[i] = Wire.read();
        signalLevel = (buf[3] >> 4) & 0x0F;
        isStereo    = (buf[2] & 0x80) != 0;
    }
}

void drawBackground() {
    M5Cardputer.Display.fillScreen(BG_COLOR);
    for (int i = 0; i < 3; i++) {
        M5Cardputer.Display.drawLine(
            0, 15 + i * 2, 240, 15 + i * 2,
            M5Cardputer.Display.color565(40, 30, 0)
        );
    }
}

void drawHeader() {
    M5Cardputer.Display.fillRect(0, 0, 240, 18,
        M5Cardputer.Display.color565(20, 15, 0));

    M5Cardputer.Display.setTextColor(PRIMARY_COLOR);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(8, 5);
    M5Cardputer.Display.print("FM RADIO");

    static bool      dotState = false;
    static unsigned long lastDot = 0;
    if (millis() - lastDot > 500) {
        dotState = !dotState;
        lastDot  = millis();
    }
    uint16_t dotCol = dotState
        ? SECONDARY_COLOR
        : M5Cardputer.Display.color565(20, 15, 0);
    M5Cardputer.Display.fillCircle(75, 9, 3, dotCol);

    M5Cardputer.Display.setTextColor(DIM_COLOR);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(190, 5);
    M5Cardputer.Display.print("by MOY");
}

void drawFrequencyDisplay() {
    M5Cardputer.Display.fillRoundRect(20, 25, 200, 55, 8,
        M5Cardputer.Display.color565(10, 8, 0));
    M5Cardputer.Display.drawRoundRect(20, 25, 200, 55, 8, PRIMARY_COLOR);
    M5Cardputer.Display.drawRoundRect(21, 26, 198, 53, 7,
        M5Cardputer.Display.color565(80, 60, 0));

    M5Cardputer.Display.setTextColor(DIM_COLOR);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(28, 30);
    M5Cardputer.Display.print("MHz");

    char freqStr[10];
    sprintf(freqStr, "%.1f", currentFreq);

    M5Cardputer.Display.setTextColor(PRIMARY_COLOR);
    M5Cardputer.Display.setTextSize(4);

    int textWidth = strlen(freqStr) * 24;
    int startX    = (240 - textWidth) / 2;
    M5Cardputer.Display.setCursor(startX - 5, 35);
    M5Cardputer.Display.print(freqStr);
}

void drawFreqBar() {
    const int barX = 20;
    const int barY = 95;
    const int barW = 200;
    const int barH = 3;

    M5Cardputer.Display.fillRect(barX, barY, barW, barH, DIM_COLOR);

    float ratio = (currentFreq - FREQ_MIN) / (FREQ_MAX - FREQ_MIN);
    int   posX  = barX + (int)(ratio * barW);

    M5Cardputer.Display.fillRect(barX, barY, posX - barX, barH, PRIMARY_COLOR);
    M5Cardputer.Display.fillCircle(posX, barY + 1, 4, PRIMARY_COLOR);
    M5Cardputer.Display.drawCircle(posX, barY + 1, 4, TEXT_COLOR);

    M5Cardputer.Display.setTextColor(DIM_COLOR);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(barX, barY - 9);
    M5Cardputer.Display.print("87.5");
    M5Cardputer.Display.setCursor(barX + barW - 18, barY - 9);
    M5Cardputer.Display.print("108");
}

void drawSignalBar() {
    M5Cardputer.Display.setTextColor(DIM_COLOR);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 104);
    M5Cardputer.Display.print("SIG");

    const int barWidth  = 9;
    const int barGap    = 2;
    const int startX    = 45;
    const int maxBars   = 10;

    for (int i = 0; i < maxBars; i++) {
        int barH = 3 + i;
        int x    = startX + i * (barWidth + barGap);
        int y    = 116 - barH;

        uint16_t color;
        if (i < signalLevel) {
            if      (i < 3) color = WARN_COLOR;
            else if (i < 7) color = SECONDARY_COLOR;
            else            color = SIGNAL_COLOR;
        } else {
            color = DIM_COLOR;
        }
        M5Cardputer.Display.fillRect(x, y, barWidth, barH, color);
    }

    M5Cardputer.Display.fillRoundRect(185, 103, 42, 14, 4,
        isStereo
            ? M5Cardputer.Display.color565(30, 22, 0)
            : M5Cardputer.Display.color565(20, 20, 20));
    M5Cardputer.Display.drawRoundRect(185, 103, 42, 14, 4,
        isStereo ? PRIMARY_COLOR : DIM_COLOR);
    M5Cardputer.Display.setTextColor(isStereo ? PRIMARY_COLOR : DIM_COLOR);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(189, 107);
    M5Cardputer.Display.print(isStereo ? "STEREO" : " MONO ");
}

void drawControls() {
    M5Cardputer.Display.fillRect(0, 120, 240, 15,
        M5Cardputer.Display.color565(15, 12, 0));
    M5Cardputer.Display.drawLine(0, 120, 240, 120,
        M5Cardputer.Display.color565(60, 45, 0));

    if (isMuted) {
        M5Cardputer.Display.fillRect(0, 120, 240, 15,
            M5Cardputer.Display.color565(50, 0, 0));
        M5Cardputer.Display.drawLine(0, 120, 240, 120, WARN_COLOR);
        M5Cardputer.Display.setTextColor(WARN_COLOR);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(35, 124);
        M5Cardputer.Display.print("** MUTED - SPACE to unmute **");
        return;
    }

    
    struct Hint {
        const char* key;
        const char* label;
        int x;
    };

    Hint hints[] = {
        { ",/",   "0.1M",  2  },   
        { ";/.",  "1M",    52 },   
        { "SPC",  "MUTE",  95 },
        { "ENT",  "SCAN", 138 }
    };

    for (auto& h : hints) {
        int kw = strlen(h.key) * 6 + 4;
        M5Cardputer.Display.fillRoundRect(h.x, 122, kw, 10, 2,
            M5Cardputer.Display.color565(50, 38, 0));
        M5Cardputer.Display.setTextColor(PRIMARY_COLOR);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(h.x + 2, 124);
        M5Cardputer.Display.print(h.key);

        M5Cardputer.Display.setTextColor(DIM_COLOR);
        M5Cardputer.Display.setCursor(h.x + kw + 2, 124);
        M5Cardputer.Display.print(h.label);
    }
}

void drawFullUI() {
    drawBackground();
    drawHeader();
    drawFrequencyDisplay();
    drawFreqBar();
    drawSignalBar();
    drawControls();
}

void scanFrequency(bool up) {
    float startFreq = currentFreq;
    float scanFreq  = currentFreq;

    for (int step = 0; step < 210; step++) {
        if (up) {
            scanFreq += 0.1f;
            if (scanFreq > FREQ_MAX) scanFreq = FREQ_MIN;
        } else {
            scanFreq -= 0.1f;
            if (scanFreq < FREQ_MIN) scanFreq = FREQ_MAX;
        }

        if (step > 5 && fabs(scanFreq - startFreq) < 0.05f) break;

        setFrequency(scanFreq);
        delay(30);
        readStatus();

        if (signalLevel > 7) break;

        if (step % 5 == 0) {
            M5Cardputer.Display.fillRoundRect(20, 25, 200, 55, 8,
                M5Cardputer.Display.color565(10, 8, 0));
            M5Cardputer.Display.drawRoundRect(20, 25, 200, 55, 8,
                SECONDARY_COLOR);

            char freqStr[10];
            sprintf(freqStr, "%.1f", scanFreq);
            M5Cardputer.Display.setTextColor(SECONDARY_COLOR);
            M5Cardputer.Display.setTextSize(4);
            int tw = strlen(freqStr) * 24;
            M5Cardputer.Display.setCursor((240 - tw) / 2 - 5, 35);
            M5Cardputer.Display.print(freqStr);

            M5Cardputer.Display.setTextColor(DIM_COLOR);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(85, 70);
            M5Cardputer.Display.print("SCANNING...");
        }
    }
    drawFullUI();
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(150);

    Wire.begin(2, 1);
    delay(100);

    setFrequency(currentFreq);
    delay(200);
    drawFullUI();
}

void loop() {
    M5Cardputer.update();

    bool needsUpdate = false;

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {

        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        for (char key : status.word) {
            switch (key) {

                
                case ',':
                    setFrequency(currentFreq - FREQ_STEP_SMALL);
                    needsUpdate = true;
                    break;

                
                case '/':
                    setFrequency(currentFreq + FREQ_STEP_SMALL);
                    needsUpdate = true;
                    break;

                
                case ';':
                    setFrequency(currentFreq - FREQ_STEP_LARGE);
                    needsUpdate = true;
                    break;

                
                case '.':
                    setFrequency(currentFreq + FREQ_STEP_LARGE);
                    needsUpdate = true;
                    break;

                
                case ' ':
                    isMuted = !isMuted;
                    setFrequency(currentFreq);
                    needsUpdate = true;
                    break;

                
                case '\n':
                case '\r':
                    scanFrequency(true);
                    needsUpdate = false;
                    break;

                default:
                    break;
            }
        }
    }

    static unsigned long lastRead = 0;
    if (millis() - lastRead > 600) {
        int  prevSig    = signalLevel;
        bool prevStereo = isStereo;
        readStatus();
        lastRead = millis();

        if (signalLevel != prevSig || isStereo != prevStereo) {
            needsUpdate = true;
        }
        drawHeader();
    }

    if (needsUpdate) {
        drawFullUI();
    }

    delay(40);
}
