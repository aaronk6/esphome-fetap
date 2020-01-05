#include "esphome.h"

#define PIN 27
#define LOOP_INTERVAL 1
#define COOLDOWN 200
#define DIAL_PLATE_DELAY 100
#define DEFAULT_DIAL_STATE -1

class FeTAp : public Component, public Sensor {

    public:

        Sensor *cradle_sensor = new Sensor();
        Sensor *dial_sensor = new Sensor();

    private:

        bool input = LOW;
        bool input_previous = LOW;
        bool dialing = false;
        byte impulse_counter = 0;

        unsigned long time_now = 0;
        unsigned long signal_stop_timestamp = 0;
        unsigned long no_signal_period = 0;

    void setup() override {
        dial_sensor->publish_state(DEFAULT_DIAL_STATE);
    }

    void loop() override {

        // limit how often this loop is being entered
        if(millis() <= time_now + LOOP_INTERVAL) return; 
        time_now = millis();

        input = digitalRead(PIN);

        // react to pin changes
        if (input != input_previous) {
            input_previous = input;

            if (!dialing && input == HIGH) {
                // Pin changed to high, but we aren't in dialing mode yet.
                // This the the cradle has been picked up.
                
                dialing = true;
                cradle_sensor->publish_state(true);

            } else if (dialing && input == LOW) {
                // We are in dialing mode and the pin changed to LOW. This can
                // either mean that we're seeing the beginning of an impulse, or
                // that the the cradle was hung up. We will find out later, therefore
                // storing the timestamp.

                signal_stop_timestamp = millis();

            } else if (dialing && input == HIGH) {
                // We are already in dialing mode and the pin changed to HIGH. This
                // means we saw an impulse, so let's count it.
                impulse_counter++;      
            }
        }

        // Determine the length of the period in which we didn't see the pin go to LOW.
        no_signal_period = millis() - signal_stop_timestamp;

        if (dialing && no_signal_period > DIAL_PLATE_DELAY && impulse_counter > 0) {

            // note: 10 impulses mean 0
            dial_sensor->publish_state(impulse_counter == 10 ? 0 : impulse_counter);
            dial_sensor->publish_state(DEFAULT_DIAL_STATE);

            // reset impulse counter, but stay in dialing mode
            impulse_counter = 0;
        }

        // Detect hung up (signal doesn't go up again after cooldown period)
        if (!input && dialing && no_signal_period > COOLDOWN) {
            cradle_sensor->publish_state(false);

            // disable dialing mode and reset impulse counter
            dialing = false;
            impulse_counter = 0;
        }
    }
};
