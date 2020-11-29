#define NUM_LEDS            114     // Total number of led in strip (width + height) * 2
#define LED_PIN             13      // Led strip pin
#define CURRENT_LIMIT       2000    // Electric current limit (milliamps). 0 - disable
#define START_FLASHES       500     // Start flashes delay (milliseconds). 0 - disable
#define SERIAL_RATE         57600   // Rate connection

#define PR_PIN                 4    // Photo resistor pin
#define PR_UPPER_BOUND         100  // Maximum required external bright level. 0 - always on
#define PR_CHECK_INTERVAL      100  // Check brightness (milliseconds)
#define PR_SEQUENCE_LEN        10   // Number of equal measurements. Change state delay PR_CHECK_INTERVAL * PR_SERIAL_LEN milliseconds

#include <FastLED.h>

CRGB strip[NUM_LEDS];

uint8_t wait_read_u8()
{
    while (!Serial.available()) {}
    return Serial.read();
}

void wait_magic_world()
{
    static const uint8_t prefix[] = { 'A', 'd', 'a' };
    for (uint8_t i = 0; i < sizeof(prefix); ++i)
    {
        if (prefix[i] != wait_read_u8())
        {
            i = 0;
        }
    }
}

bool check_sum()
{
    const uint8_t hi = wait_read_u8();
    const uint8_t lo = wait_read_u8();
    const uint8_t checksum = wait_read_u8();
    return (checksum == (hi ^ lo ^ 0x55));
}

void wait_handshake()
{
    bool done = false;
    while (!done)
    {
        wait_magic_world();
        done = check_sum();
    }
}

void show_color(const CRGB& color, uint32_t sleep = START_FLASHES)
{
    LEDS.showColor(color);
    delay(sleep);
}

void setup()
{
    FastLED.addLeds<WS2812, LED_PIN, GRB> (strip, NUM_LEDS);
    if (CURRENT_LIMIT > 0)
    {
        FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
    }

    if (START_FLASHES > 0)
    {
        show_color(CRGB(255, 0, 0));
        show_color(CRGB(0, 255, 0));
        show_color(CRGB(0, 0, 255));
        show_color(CRGB(0, 0, 0));
    }

    Serial.begin(SERIAL_RATE);
    Serial.print("Ada\n");
}

bool is_enabled()
{
    static bool enabled = true;
    if (PR_UPPER_BOUND > 0)
    {
        const uint32_t now = millis();
        static uint32_t pr_last_check_time = now - PR_CHECK_INTERVAL;
        if (now - pr_last_check_time >= PR_CHECK_INTERVAL)
        {
            pr_last_check_time = now;
            const uint32_t pr_value = analogRead(PR_PIN);
            const bool curr_enabled = pr_value < PR_UPPER_BOUND;
            static uint8_t sequence_len = 0;
            if (curr_enabled != enabled)
            {
                ++sequence_len;
                if (sequence_len == PR_SEQUENCE_LEN)
                {
                    if (enabled)
                    {
                        FastLED.clear();
                        FastLED.show();
                    }
                    enabled = curr_enabled;
                    sequence_len = 0;
                }
            }
            else
            {
                sequence_len = 0;
            }
        }
    }

    return enabled;
}

void loop()
{
    if (!is_enabled())
    {
        return;
    }

    wait_handshake();

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        strip[i].r = wait_read_u8();
        strip[i].g = wait_read_u8();
        strip[i].b = wait_read_u8();
    }

    FastLED.show();
}
