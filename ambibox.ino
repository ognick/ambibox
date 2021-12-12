#define NUM_LEDS            114     // Total number of led in strip (width + height) * 2
#define LED_PIN             13      // Led strip pin
#define CURRENT_LIMIT       3200    // Electric current limit (milliamps). 0 - disable
#define START_FLASHES       500     // Start flashes delay (milliseconds). 0 - disable
#define SERIAL_RATE         115200  // Rate connection

#define PR_PIN              4       // Photo resistor pin
#define PR_UPPER_BOUND      120     // Maximum required external bright level. 0 - always on
#define PR_LOWER_BOUND      100     // Minimum required external bright level
#define PR_CHECK_INTERVAL   100     // Check brightness (milliseconds)
#define PR_SEQUENCE_LEN     20      // Number of equal measurements. Change state delay PR_CHECK_INTERVAL * PR_SERIAL_LEN milliseconds
#define OFF_TIMEOUT         8000    // ms to switch off after no data was received, set 0 to deactivate


#include <FastLED.h>

CRGB strip[NUM_LEDS];

uint8_t wait_read_u8(bool& interrupted)
{
    const uint32_t off_timer = millis() + OFF_TIMEOUT;
    while (!Serial.available()) 
    {
      if ( OFF_TIMEOUT > 0 && millis() > off_timer) {
        interrupted = true;
        return 0;
      }
    }
    interrupted = false;
    return Serial.read();
}

void wait_magic_world()
{
    static const uint8_t prefix[] = { 'A', 'd', 'a' };
    for (uint8_t i = 0; i < sizeof(prefix); ++i)
    {
        bool interrupted;
        if (prefix[i] != wait_read_u8(interrupted) || interrupted)
        {
            i = 0;
        }
    }
}

uint32_t wait_header(bool& interrupted)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    bool done = false;
    while (!done)
    {
        wait_magic_world();
        hi = wait_read_u8(interrupted); if (interrupted) continue;
        lo = wait_read_u8(interrupted); if (interrupted) continue;
        const uint8_t checksum = wait_read_u8(interrupted); if (interrupted) continue;
        done = (checksum == (hi ^ lo ^ 0x55));
    }
    return (hi<<8) + lo + 1;
}

void show_color(const CRGB& color, uint32_t sleep = START_FLASHES)
{
    LEDS.showColor(color);
    delay(sleep);
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
            static uint8_t sequence_len = 0;
            if ((!enabled && pr_value < PR_LOWER_BOUND) || (enabled && pr_value > PR_UPPER_BOUND))
            {
                ++sequence_len;
                if (sequence_len == PR_SEQUENCE_LEN)
                {
                    if (enabled)
                    {
                        FastLED.clear();
                        FastLED.show();
                    }
                    enabled = !enabled;
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

void flash_frame_data()
{
  while(Serial.available())
  {
    Serial.read();
  }
}

uint32_t ada_timer_ms = 0;

void tick()
{
  uint32_t now_ms = millis();
  if (now_ms > ada_timer_ms)
  {
      ada_timer_ms = now_ms + OFF_TIMEOUT;  
      Serial.print("Ada\n");
  }
  
  if (is_enabled())
  {

    bool inetrrupted = true;
    const uint32_t num_leds = wait_header(inetrrupted); if (inetrrupted) return;
    if (num_leds != NUM_LEDS) return;
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        strip[i].r = wait_read_u8(inetrrupted); if (inetrrupted) break;
        strip[i].g = wait_read_u8(inetrrupted); if (inetrrupted) break;
        strip[i].b = wait_read_u8(inetrrupted); if (inetrrupted) break;   
    } 
  
    if (!inetrrupted)
    {
      FastLED.show();
    }
  }
  
  // FastLED.show() disables interrupts while writing out WS2812 data. It leads to getting corrupted frames.
  flash_frame_data();
}

void fast_loop()
{
    for (;;)
    {
      tick();
    }
}

void setup()
{
    Serial.begin(SERIAL_RATE);
    
    FastLED.addLeds<WS2812B, LED_PIN, GRB> (strip, NUM_LEDS);
    if (CURRENT_LIMIT > 0)
    {
        FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
    }

    if (START_FLASHES > 0)
    {
        show_color(CRGB(255, 255, 255));
        show_color(CRGB(255, 0, 0));
        show_color(CRGB(255, 255, 255));
        show_color(CRGB(0, 0, 0));
    }

    delay(1000);   
    fast_loop();
}

void loop()
{
//  tick();
}
