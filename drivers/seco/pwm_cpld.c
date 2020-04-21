
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <asm/byteorder.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/pwm.h>

#include <linux/seco_cpld.h>


#define CPWM_INFO(fmt, arg...) printk(KERN_INFO "SecoCPWM: " fmt "\n" , ## arg)
#define CPWM_ERR(fmt, arg...)  printk(KERN_ERR "%s: " fmt "\n" , __func__ , ## arg)
#define CPWM_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n" , __func__ , ## arg)

#define DRV_VERSION     "1.0"

#define PWM_LPC_REG1   CPLD_REG_6
#define PWM_LPC_REG2   CPLD_REG_7
#define PWM_LPC_REG3   CPLD_REG_8

#define PWM_GPIO_REG1   CPLD_REG_8
#define PWM_GPIO_REG2   CPLD_REG_9
#define PWM_GPIO_REG3   CPLD_REG_10


#define PWM_MASK_EN         0x8000
#define PWM_MASK_POL        0x4000
#define PWM_MASK_PRESCALE   0x3FFF
#define PWM_MASK_PERIOD     0xFFFF
#define PWM_MASK_DUTY       0xFFFF

#define MAIN_CLK      33000000    // (Hz)

struct cpwm_data {
	bool polarity_supported;
	const struct pwm_ops *ops;
};

struct register_map {
	u8  PWM_REG_ENABLE;
	u8  PWM_REG_POLARITY;
	u8  PWM_REG_PRESCALE;
	u8  PWM_REG_PERIOD;
	u8  PWM_REG_DUTY;
};

struct cpwm_chip {
	struct pwm_chip	    chip;
    struct register_map reg_map;
};


#define to_cpwm_chip(chip)	container_of(chip, struct cpwm_chip, chip)


/* __________________________________________________________________________
* |                                                                          |
* |                              W/R BASIC FUNCTION                          |
* |__________________________________________________________________________|
*/
static void cpwm_writeb (unsigned int reg, uint16_t value) {
	cpld_reg_write (reg, value);
}


static void cpwm_readb (unsigned int reg, uint16_t *data) {
	uint16_t value;
	cpld_reg_read (reg, &value);
	*data = value;
}

/* __________________________________________________________________________
* |__________________________________________________________________________|
*/

/* __________________________________________________________________________
* |                                                                          |
* |                              PWM BASIC FUNCTION                          |
* |__________________________________________________________________________|
*/
static void cpwm_enable ( struct pwm_chip *chip ) {
    struct cpwm_chip *cpwm = to_cpwm_chip( chip );
    uint16_t reg_value;

	cpwm_readb( cpwm->reg_map.PWM_REG_ENABLE, &reg_value );
	cpwm_writeb( cpwm->reg_map.PWM_REG_ENABLE, reg_value | PWM_MASK_EN) ;
}


static void cpwm_disable ( struct pwm_chip *chip ) {
    struct cpwm_chip *cpwm = to_cpwm_chip( chip );
    uint16_t reg_value;

    cpwm_readb( cpwm->reg_map.PWM_REG_ENABLE, &reg_value );
	cpwm_writeb( cpwm->reg_map.PWM_REG_ENABLE, reg_value & ~PWM_MASK_EN) ;
}


static void cpwm_pol_inv ( struct pwm_chip *chip ) {
    struct cpwm_chip *cpwm = to_cpwm_chip( chip );
    uint16_t reg_value;

	cpwm_readb( cpwm->reg_map.PWM_REG_POLARITY, &reg_value );
	cpwm_writeb( cpwm->reg_map.PWM_REG_POLARITY, reg_value & ~PWM_MASK_POL);
}


static void cpwm_pol_dir ( struct pwm_chip *chip ) {
    struct cpwm_chip *cpwm = to_cpwm_chip( chip );
    uint16_t reg_value;

	cpwm_readb( cpwm->reg_map.PWM_REG_POLARITY, &reg_value );
	cpwm_writeb( cpwm->reg_map.PWM_REG_POLARITY, reg_value | PWM_MASK_POL);
}


static int cpwm_apply( struct pwm_chip *chip, struct pwm_device *pwm,
			    struct pwm_state *state )
{
    unsigned long long period_cycles, duty_cycles, prescale;
    struct cpwm_chip *cpwm = to_cpwm_chip( chip );
    struct pwm_state cstate;
    unsigned long long clk1;
	unsigned long long nclk;
	uint16_t reg_value;

    pwm_get_state(pwm, &cstate);
    if (state->enabled) {
        prescale = 0;
		
	    do {
		    clk1 = (MAIN_CLK >> 1);
		    do_div (clk1, (prescale + 1));
		    nclk = state->period * clk1;	
		    do_div (nclk, 1000000000);	
		    nclk--;
		    prescale++;
	    } while (nclk & ~((unsigned long long)0xFFFF));
	    prescale--;
		
	    period_cycles = nclk;
	    duty_cycles = nclk * state->duty_cycle;
	    do_div (duty_cycles, 100);

        cpwm_writeb( cpwm->reg_map.PWM_REG_PERIOD, period_cycles  & PWM_MASK_PERIOD);
	    cpwm_writeb( cpwm->reg_map.PWM_REG_DUTY, duty_cycles & PWM_MASK_DUTY);
        if ( state->polarity != PWM_POLARITY_INVERSED ) {
            cpwm_pol_dir( chip );
        } else {
            cpwm_pol_inv( chip );
        }
cpwm_pol_inv( chip );
		cpwm_readb( cpwm->reg_map.PWM_REG_PRESCALE, &reg_value );
		cpwm_writeb( cpwm->reg_map.PWM_REG_PRESCALE, reg_value | (PWM_MASK_PRESCALE & (~PWM_MASK_PRESCALE | prescale)));

        if (cstate.enabled) {
			// do nothing
		} else {
			cpwm_enable( chip );
		}
		
    } else if ( cstate.enabled ) {
		cpwm_disable( chip );
	}
    return 0;
}
/* __________________________________________________________________________
* |__________________________________________________________________________|
*/


static const struct pwm_ops cpwm_ops = {
	.apply = cpwm_apply,
	.owner = THIS_MODULE,
};


static struct cpwm_data cpwm_data = {
	.polarity_supported = true,
	.ops = &cpwm_ops,
};



static int cpwm_probe (struct platform_device *pdev) {
	struct cpwm_chip *cpwm;
	int ret, err;

	cpwm = kzalloc (sizeof (struct cpwm_chip), GFP_KERNEL);
	if ( !cpwm ) {
		CPWM_ERR ("cannot allocate memory for structure data");
		err = -ENOMEM;
		goto err_data_allocate;
	}

    /* check that the CPLD firmware is a GPIO expander */
	if ( cpld_is_gpio () ) {
	    cpwm->reg_map.PWM_REG_ENABLE   = PWM_GPIO_REG1;
		cpwm->reg_map.PWM_REG_POLARITY = PWM_GPIO_REG1;
		cpwm->reg_map.PWM_REG_PRESCALE = PWM_GPIO_REG1;
		cpwm->reg_map.PWM_REG_PERIOD   = PWM_GPIO_REG2;
		cpwm->reg_map.PWM_REG_DUTY     = PWM_GPIO_REG3;
	} else {
        cpwm->reg_map.PWM_REG_ENABLE   = PWM_LPC_REG1;
		cpwm->reg_map.PWM_REG_POLARITY = PWM_LPC_REG1;
		cpwm->reg_map.PWM_REG_PRESCALE = PWM_LPC_REG1;
		cpwm->reg_map.PWM_REG_PERIOD   = PWM_LPC_REG2;
		cpwm->reg_map.PWM_REG_DUTY     = PWM_LPC_REG3;
    }

    platform_set_drvdata( pdev, cpwm );

    cpwm->chip.ops = cpwm_data.ops;
	cpwm->chip.dev = &pdev->dev;
	cpwm->chip.base = -1;
	cpwm->chip.npwm = 1;

    if ( cpwm_data.polarity_supported ) {
		cpwm->chip.of_xlate = of_pwm_xlate_with_flags;
		cpwm->chip.of_pwm_n_cells = 3;
	}

    ret = pwmchip_add(&cpwm->chip);
	if ( ret < 0 ) {
		err = ret;
		goto err_pwmchip_add;
    }

    CPWM_INFO ("cpwm driver ver %s probed!!!", DRV_VERSION);
    return 0;
err_pwmchip_add:
    kfree( cpwm );
err_data_allocate:
	return err;
}


static int cpwm_remove (struct platform_device *pdev) {
    return 0;
}



static const struct of_device_id seco_cpwm_match[] = {
	{ .compatible = "seco,cpwm" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, seco_cpwm_match);


static struct platform_driver cpwm_driver = {
	.driver = {
		.name		    = "cpwm",
		.owner          = THIS_MODULE,
		.of_match_table	= seco_cpwm_match,
	},
	.probe  = cpwm_probe,
	.remove = cpwm_remove,
};



static int __init cpwm_init(void) {
	return platform_driver_register (&cpwm_driver);
}

subsys_initcall(cpwm_init);


static void __exit cpwm_exit (void) {
	return platform_driver_unregister (&cpwm_driver);
}

module_exit(cpwm_exit);



MODULE_AUTHOR("Davide Cardillo, SECO srl");
MODULE_DESCRIPTION("SECO PWM over CPLD logic");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
