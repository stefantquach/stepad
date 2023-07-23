#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "led_matrix.h"
#include "utility_colors.h"

// Hold time for each iterations of the LED process loop 
#define LED_COL_HOLD_TIME_TICKS 3

// Struct defs
typedef struct pwm_slice_chan
{
    uint slice;
    uint channel;
} pwm_ref_t;

// Pin definitions
enum
{
    LED_RED,
    LED_GREEN,
    LED_BLUE
} led_row_table_index;

// Row table                        R                 G                 B
const int row_pins[NUM_ROWS][3] = {{STEPAD_LED_ROW_R, STEPAD_LED_ROW_G, STEPAD_LED_ROW_B}};
const pwm_ref_t row_pwm_slice[NUM_ROWS][3] = {{{0, PWM_CHAN_A}, {0, PWM_CHAN_B}, {1, PWM_CHAN_A}}};
// Column table
const int col_pins[NUM_COLS] = {STEPAD_LED_COL1, STEPAD_LED_COL2, STEPAD_LED_COL3, STEPAD_LED_COL4};


// Static variables
static rgb_t led_color[NUM_ROWS][NUM_COLS] = {0};
static int irq_counter;
static uint curr_led_col;

// IRQ
static void pwm_wrap_irq(void);

void initialize_led_pwms()
{
    int i,j;

    // ------ Initialize columns ------
    // set the drive strength for the LED matrix higher.
    // The columns source current so we set only their drive strength;
    for(i=0; i < NUM_COLS; ++i)
    {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_OUT);
        gpio_set_drive_strength(col_pins[i], GPIO_DRIVE_STRENGTH_8MA);
        gpio_set_slew_rate(col_pins[i], GPIO_SLEW_RATE_FAST);
        gpio_pull_down(col_pins[i]); // enable pull down to prevent ghosting
        gpio_put(col_pins[i], false);
    }

    // ------ Initialize rows ------
    // set the following configs
    pwm_config config = pwm_get_default_config();
    // pwm_config_set_clkdiv(&config, 4.f);
    pwm_config_set_output_polarity(&config, true, true); // invert outputs of both channels

    // Set pin functions to PWM
    for(i=0; i < NUM_ROWS; ++i)
    {
        for(j=0; j < 3; ++j)
        {
            gpio_set_function(row_pins[i][j], GPIO_FUNC_PWM);
            gpio_set_slew_rate(row_pins[i][j], GPIO_SLEW_RATE_FAST);
            gpio_pull_up(row_pins[i][j]); // enable pull up to prevent ghosting
            pwm_init(row_pwm_slice[i][j].slice, &config, false);
            pwm_set_gpio_level(row_pins[i][j], 0);
        }
    }

    // Setup PWM IRQ when the counter wraps around.
    // Since all the PWMs are running at the same speed and same clock, just use the first row's pins to generate the IRQ
    pwm_clear_irq(row_pwm_slice[0][0].slice);
    pwm_set_irq_enabled(row_pwm_slice[0][0].slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_irq);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    irq_counter = 0;
    curr_led_col = 0;
}

/**
 * Enables all the PWM modules (that are being used for the matrix).
*/
void start_led_process()
{
    // Enable the PWMs
    // Generate the mask
    uint32_t pwm_mask = 0;
    int i;
    int j;
    for(i=0; i < NUM_ROWS; ++i)
    {
        for(j=0; j < 3; ++j)
        {
            pwm_mask |= (1 << row_pwm_slice[i][j].slice);
        }
    }
    pwm_set_mask_enabled(pwm_mask);
}

/**
 * IRQ process to deal with the RGB LED matrix
*/
void pwm_wrap_irq()
{    
    int i=0;
    
    // Clear the interrupt flag
    pwm_clear_irq(row_pwm_slice[0][0].slice);

    if(irq_counter == 1)
    {
        gpio_put(col_pins[curr_led_col], true);
    }

    if(irq_counter == 0)
    {
        // Turn off the previous column
        gpio_put(col_pins[curr_led_col], false);

        // increment to next col
        if(++curr_led_col >= NUM_COLS)
        {
            curr_led_col = 0;
        }
        
        // Set PWM cycle of the row to match color
        for(i=0; i<NUM_ROWS; ++i)
        {
            // write the square of the color value to make the brightness more linear
            uint16_t red = led_color[i][curr_led_col].red * led_color[i][curr_led_col].red;
            uint16_t green = led_color[i][curr_led_col].green * led_color[i][curr_led_col].green;
            uint16_t blue = led_color[i][curr_led_col].blue * led_color[i][curr_led_col].blue;
            pwm_set_gpio_level(row_pins[i][LED_RED], red);
            pwm_set_gpio_level(row_pins[i][LED_GREEN], green);
            pwm_set_gpio_level(row_pins[i][LED_BLUE], blue);
        }
    }

    if(++irq_counter >= 3)
    {
        irq_counter = 0;
    }
}


/**
 * Set a given LED RGB value
*/
void set_led(int row, int col, uint8_t r, uint8_t g, uint8_t b)
{
    led_color[row][col].red = r;
    led_color[row][col].green = g;
    led_color[row][col].blue = b;
}